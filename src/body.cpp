#include "body.h"
#include "actor.h"
#include "config.h"
#include "randomUtil.h"
#include "util.h"
#include "simulation.h"
Wound::Wound(Actor& a, const WoundType wt, BodyPart& bp, Hit h, uint32_t bvr, uint32_t ph) : woundType(wt), bodyPart(bp), hit(h), bleedVolumeRate(bvr), percentHealed(ph), healEvent(a.getEventSchedule()) { }
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
Body::Body(Actor& a) :  m_actor(a), m_totalVolume(0), m_impairMovePercent(0), m_impairManipulationPercent(0), m_bleedVolumeRate(0), m_bleedEvent(a.getEventSchedule()), m_woundsCloseEvent(a.getEventSchedule())
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
		uint32_t forceDelta = bodyPart.materialType.hardness * hit.area * Config::skinPierceForceCost;
		hit.force = hit.force < forceDelta ? 0 : hit.force - forceDelta;
		if(piercesFat(hit, bodyPart))
		{
			++hit.depth;
			forceDelta = bodyPart.materialType.hardness * hit.area * Config::skinPierceForceCost;
			hit.force = hit.force < forceDelta ? 0 : hit.force - forceDelta;
			if(piercesMuscle(hit, bodyPart))
			{
				++hit.depth;
				forceDelta = bodyPart.materialType.hardness * hit.area * Config::skinPierceForceCost;
				hit.force = hit.force < forceDelta ? 0 : hit.force - forceDelta;
				if(piercesBone(hit, bodyPart))
					++hit.depth;
			}
		}
	}
}
Wound& Body::addWound(BodyPart& bodyPart, Hit& hit)
{
	assert(hit.depth != 0);
	uint32_t scale = m_actor.m_species.bodyScale;
	uint32_t bleedVolumeRate = WoundCalculations::getBleedVolumeRate(hit, bodyPart.bodyPartType, scale);
	Wound& wound = bodyPart.wounds.emplace_back(m_actor, hit.woundType, bodyPart, hit, bleedVolumeRate);
	Step delayTillHeal = WoundCalculations::getStepsTillHealed(hit, bodyPart.bodyPartType, scale);
	wound.healEvent.schedule(delayTillHeal, wound, *this);
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
	m_volumeOfBlood --;
	float ratio = m_volumeOfBlood / healthyBloodVolume();
	if(ratio <= Config::bleedToDeathRatio)
		m_actor.die(CauseOfDeath::bloodLoss);
	else 
	{
		if (m_actor.m_mustSleep.isAwake() && ratio < Config::bleedToUnconciousessRatio)
			m_actor.passout(Config::bleedPassOutDuration);
		recalculateBleedAndImpairment();
	}
}
void Body::recalculateBleedAndImpairment()
{
	m_bleedVolumeRate = 0;
	m_impairManipulationPercent = 0;
	m_impairMovePercent = 0;
	for(BodyPart& bodyPart : m_bodyParts)
	{
		for(Wound& wound : bodyPart.wounds)
		{
			m_bleedVolumeRate += wound.bleedVolumeRate; // when wound closes set to 0.
			if(bodyPart.bodyPartType.doesLocamotion)
				m_impairMovePercent += wound.impairPercent();
			if(bodyPart.bodyPartType.doesManipulation)
				m_impairManipulationPercent += wound.impairPercent();
		}
		m_impairManipulationPercent = std::min(100u, m_impairManipulationPercent);
		m_impairMovePercent = std::min(100u, m_impairMovePercent);
	}
	if(m_bleedVolumeRate > 0)
	{
		// Calculate bleed event frequency from bleed volume rate.
		Step baseFrequency = Config::bleedEventBaseFrequency / m_bleedVolumeRate;
		Step baseWoundsCloseDelay = m_bleedVolumeRate * Config::ratioWoundsCloseDelayToBleedVolume;
		// Apply already elapsed progress from existing event ( if any ).
		if(m_bleedEvent.exists())
		{
			// Already bleeding, reschedule bleed event and wounds close event.
			assert(m_woundsCloseEvent.exists());
			Step adjustedFrequency = util::scaleByInversePercent(baseFrequency, m_bleedEvent.percentComplete());
			Step toScheduleStep = m_actor.getSimulation().m_step + adjustedFrequency;
			if(toScheduleStep != m_bleedEvent.getStep())
			{
				m_bleedEvent.unschedule();
				m_bleedEvent.schedule(adjustedFrequency, *this);
			}
			Step adjustedWoundsCloseDelay = util::scaleByInversePercent(baseWoundsCloseDelay, m_woundsCloseEvent.percentComplete());
			toScheduleStep = m_actor.getSimulation().m_step + adjustedWoundsCloseDelay;
			if(!m_woundsCloseEvent.exists() || toScheduleStep != m_woundsCloseEvent.getStep())
			{
				m_woundsCloseEvent.maybeUnschedule();
				m_woundsCloseEvent.schedule(adjustedWoundsCloseDelay, *this);
			}

		}
		else
		{
			// Start bleeding.
			m_bleedEvent.schedule(baseFrequency, *this);
			m_woundsCloseEvent.schedule(baseWoundsCloseDelay, *this);
		}
	}
	else
	{
		// Stop bleeding if bleeding.
		m_woundsCloseEvent.maybeUnschedule();
		m_bleedEvent.maybeUnschedule();
	}
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
	return m_volumeOfBlood * (Config::bleedEventBaseFrequency / m_bleedVolumeRate);
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
