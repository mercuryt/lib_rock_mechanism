#include "body.h"
#include "actor.h"
#include "config.h"
#include "randomUtil.h"
#include "util.h"
#include "simulation.h"
Wound::Wound(Actor& a, const WoundType& wt, BodyPart& bp, Hit h, uint32_t bvr, uint32_t ph) : woundType(wt), bodyPart(bp), hit(h), bleedVolumeRate(bvr), percentHealed(ph), healEvent(a.getEventSchedule()) { }
uint32_t Wound::getPercentHealed() const
{
	uint32_t output = percentHealed;
	if(healEvent.exists())
	{
		if(output != 0)
			output += (healEvent.percentComplete() * (100u - output)) / 100u;
		else
			output = healEvent.percentComplete();
	}
	return output;
}
uint32_t Wound::impairPercent() const
{
	return util::scaleByInversePercent(maxPercentTemporaryImpairment, getPercentHealed()) + maxPercentPermanantImpairment;
}
Body::Body(Actor& a) :  m_actor(a), m_totalVolume(0), m_impairMovePercent(0), m_impairManipulationPercent(0), m_bleedEvent(a.getEventSchedule()), m_woundsCloseEvent(a.getEventSchedule())
{
	for(const BodyPartType* bodyPartType : m_actor.m_species.bodyType.bodyPartTypes)
	{
		m_bodyParts.emplace_back(*bodyPartType, m_actor.m_species.materialType);
		m_totalVolume += bodyPartType->volume;
	}
	m_volumeOfBlood = healthyBloodVolume();
}
BodyPart& Body::pickABodyPartByVolume()
{
	uint32_t roll = randomUtil::getInRange(0u, m_totalVolume);
	for(BodyPart& bodyPart : m_bodyParts)
	{
		if(bodyPart.bodyPartType.volume >= roll)
			return bodyPart;
		else
			roll -= bodyPart.bodyPartType.volume;
	}
	assert(false);
}
// Armor has already been applied, calculate hit depth.
void Body::getHitDepth(Hit& hit, const BodyPart& bodyPart)
{
	assert(hit.depth == 0);
	if(piercesSkin(hit, bodyPart))
	{
		++hit.depth;
		hit.force -= bodyPart.materialType.hardness * hit.area * Config::skinPierceForceCost;
		if(piercesFat(hit, bodyPart))
		{
			++hit.depth;
			hit.force -= bodyPart.materialType.hardness * hit.area * Config::fatPierceForceCost;
			if(piercesMuscle(hit, bodyPart))
			{
				++hit.depth;
				hit.force -= bodyPart.materialType.hardness * hit.area * Config::musclePierceForceCost;
				if(piercesBone(hit, bodyPart))
					++hit.depth;
			}
		}
	}
}
Wound& Body::addWound(const WoundType& woundType, BodyPart& bodyPart, const Hit& hit)
{
	uint32_t scale = m_actor.m_species.bodyScale;
	uint32_t bleedVolumeRate = WoundCalculations::getBleedVolumeRate(woundType, hit, bodyPart.bodyPartType, scale);
	Wound& wound = bodyPart.wounds.emplace_back(m_actor, woundType, bodyPart, hit, bleedVolumeRate);
	Step delayTilHeal = WoundCalculations::getStepsTillHealed(woundType, hit, bodyPart.bodyPartType, scale);
	wound.healEvent.schedule(delayTilHeal, wound, *this);
	recalculateBleedAndImpairment();
	return wound;
}
void Body::healWound(Wound& wound)
{
	wound.bodyPart.wounds.remove(wound);
	recalculateBleedAndImpairment();
}
void Body::doctorWound(Wound& wound, uint32_t healSpeedPercentageChange)
{
	assert(wound.healEvent.exists());
	if(wound.bleedVolumeRate != 0)
	{
		wound.bleedVolumeRate = 0;
		recalculateBleedAndImpairment();
	}
	Step remaningSteps = wound.healEvent.remainingSteps();
	remaningSteps = util::scaleByPercent(remaningSteps, healSpeedPercentageChange);
	wound.healEvent.unschedule();
	wound.healEvent.schedule(remaningSteps, wound, *this);
}
void Body::woundsClose()
{
	for(BodyPart& bodyPart : m_bodyParts)
		for(Wound& wound : bodyPart.wounds)
			wound.bleedVolumeRate = 0;
	recalculateBleedAndImpairment();
}
void Body::bleed()
{
	m_volumeOfBlood -= m_bleedVolumeRate;
	float ratio = m_bleedVolumeRate / healthyBloodVolume();
	if(ratio <= Config::bleedToDeathRatio)
		m_actor.die(CauseOfDeath::bloodLoss);
	else if (ratio < Config::bleedToUnconciousessRatio)
		m_actor.passout(Config::bleedPassOutDuration);
}
void Body::recalculateBleedAndImpairment()
{
	uint32_t bleedVolumeRate = 0;
	m_impairManipulationPercent = 0;
	m_impairMovePercent = 0;
	for(BodyPart& bodyPart : m_bodyParts)
	{
		for(Wound& wound : bodyPart.wounds)
		{
			bleedVolumeRate += wound.bleedVolumeRate; // when wound closes set to 0.
			if(bodyPart.bodyPartType.doesLocamotion)
				m_impairMovePercent += wound.impairPercent();
			if(bodyPart.bodyPartType.doesManipulation)
				m_impairManipulationPercent += wound.impairPercent();
		}
		m_impairManipulationPercent = std::min(100u, m_impairManipulationPercent);
		m_impairMovePercent = std::min(100u, m_impairMovePercent);
	}
	if(bleedVolumeRate > 0 && !m_bleedEvent.exists())
		m_bleedEvent.schedule(Config::bleedEventFrequency, *this);
	else if(bleedVolumeRate == 0 && m_woundsCloseEvent.exists())
		m_woundsCloseEvent.unschedule();
	if(bleedVolumeRate > 0 && !m_woundsCloseEvent.exists())
	{
		uint32_t delay = bleedVolumeRate * Config::ratioWoundsCloseDelayToBleedVolume;
		m_woundsCloseEvent.schedule(delay, *this);
	}
	else if(bleedVolumeRate == 0 && m_woundsCloseEvent.exists())
		m_woundsCloseEvent.unschedule();
	m_actor.m_attributes.generate();
}
Wound& Body::getWoundWhichIsBleedingTheMost()
{
	Wound* output = nullptr;
	uint32_t outputBleedRate = 0;
	for(BodyPart& bodyPart : m_bodyParts)
		for(Wound& wound : bodyPart.wounds)
			if(wound.bleedVolumeRate >= outputBleedRate)
			{
				outputBleedRate = wound.bleedVolumeRate;
				output = &wound;
			}
	assert(output != nullptr);
	return *output;
}
uint32_t Body::getStepsTillBleedToDeath() const
{
	return (m_volumeOfBlood / m_bleedVolumeRate) / Config::bleedEventFrequency;
}
bool Body::piercesSkin(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (hit.force / hit.area) * hit.materialType.hardness * Config::pierceSkinModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesFat(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (hit.force / hit.area) * hit.materialType.hardness * Config::pierceFatModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesMuscle(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (hit.force / hit.area) * hit.materialType.hardness * Config::pierceMuscelModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesBone(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (hit.force / hit.area) * hit.materialType.hardness * Config::pierceBoneModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
uint32_t Body::healthyBloodVolume() const
{
	return m_totalVolume * Config::ratioOfTotalBodyVolumeWhichIsBlood;
}
std::vector<Attack> Body::getAttacks() const
{
	std::vector<Attack> output;
	for(const BodyPart& bodyPart : m_bodyParts)
		for(auto& [attackType, materialType] : bodyPart.bodyPartType.attackTypesAndMaterials)
			output.emplace_back(&attackType, materialType, nullptr);
	return output;
}
uint32_t Body::getVolume() const
{
	return util::scaleByPercent(m_totalVolume, m_actor.m_canGrow.growthPercent());
}
bool Body::isInjured() const
{
	for(const BodyPart& bodyPart : m_bodyParts)
		if(!bodyPart.wounds.empty())
			return true;
	return false;
}
WoundHealEvent::WoundHealEvent(const Step delay, Wound& w, Body& b) : ScheduledEventWithPercent(b.m_actor.getSimulation(), delay), m_wound(w), m_body(b) {}
BleedEvent::BleedEvent(const Step delay, Body& b) : ScheduledEventWithPercent(b.m_actor.getSimulation(), delay), m_body(b) {}
WoundsCloseEvent::WoundsCloseEvent(const Step delay, Body& b) : ScheduledEventWithPercent(b.m_actor.getSimulation(), delay), m_body(b) {}
