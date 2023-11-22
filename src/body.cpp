#include "body.h"
#include "actor.h"
#include "config.h"
#include "random.h"
#include "util.h"
#include "simulation.h"
Wound::Wound(Actor& a, const WoundType wt, BodyPart& bp, Hit h, uint32_t bvr, Percent ph) : woundType(wt), bodyPart(bp), hit(h), bleedVolumeRate(bvr), percentHealed(ph), healEvent(a.getEventSchedule()) 
{ 
	maxPercentTemporaryImpairment = WoundCalculations::getPercentTemporaryImpairment(hit, bodyPart.bodyPartType, a.m_species.bodyScale);
	maxPercentPermanantImpairment = WoundCalculations::getPercentPermanentImpairment(hit, bodyPart.bodyPartType, a.m_species.bodyScale);
}
Percent Wound::getPercentHealed() const
{
	Percent output = percentHealed;
	if(healEvent.exists())
	{
		if(output != 0)
			output += (healEvent.percentComplete() * (100u - output)) / 100u;
		else
			output = healEvent.percentComplete();
	}
	return output;
}
Percent Wound::impairPercent() const
{
	return util::scaleByInversePercent(maxPercentTemporaryImpairment, getPercentHealed()) + maxPercentPermanantImpairment;
}
Body::Body(Actor& a) :  m_actor(a), m_totalVolume(0), m_impairMovePercent(0), m_impairManipulationPercent(0), m_isBleeding(false), m_bleedEvent(a.getEventSchedule()), m_woundsCloseEvent(a.getEventSchedule())
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
	Random& random = m_actor.getSimulation().m_random;
	uint32_t roll = random.getInRange(0u, m_totalVolume);
	for(BodyPart& bodyPart : m_bodyParts)
	{
		if(bodyPart.bodyPartType.volume >= roll)
			return bodyPart;
		else
			roll -= bodyPart.bodyPartType.volume;
	}
	assert(false);
}
BodyPart& Body::pickABodyPartByType(const BodyPartType& bodyPartType)
{
	for(BodyPart& bodyPart : m_bodyParts)
		if(bodyPart.bodyPartType == bodyPartType)
				return bodyPart;
	assert(false);
}
// Armor has already been applied, calculate hit depth.
void Body::getHitDepth(Hit& hit, const BodyPart& bodyPart)
{
	assert(hit.depth == 0);
	if(piercesSkin(hit, bodyPart))
	{
		++hit.depth;
		uint32_t hardnessDelta = std::max(1, (int32_t)bodyPart.materialType.hardness - (int32_t)hit.materialType.hardness);
		Force forceDelta = hardnessDelta * hit.area * Config::skinPierceForceCost;
		hit.force = hit.force < forceDelta ? 0 : hit.force - forceDelta;
		if(piercesFat(hit, bodyPart))
		{
			++hit.depth;
			forceDelta = hardnessDelta * hit.area * Config::fatPierceForceCost;
			hit.force = hit.force < forceDelta ? 0 : hit.force - forceDelta;
			if(piercesMuscle(hit, bodyPart))
			{
				++hit.depth;
				forceDelta = hardnessDelta * hit.area * Config::musclePierceForceCost;
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
	float ratio = (float)hit.area / bodyPart.bodyPartType.volume * scale;
	if(bodyPart.bodyPartType.vital && hit.depth == 4 && ratio >= Config::hitAreaToBodyPartVolumeRatioForFatalStrikeToVitalArea)
	{
		m_actor.die(CauseOfDeath::wound);
		return wound;
	}
	else if(hit.woundType == WoundType::Cut && hit.depth == 4 && ((float)hit.area / (float)bodyPart.bodyPartType.volume) > Config::ratioOfHitAreaToBodyPartVolumeForSever)
	{
		sever(bodyPart, wound);
		if(bodyPart.bodyPartType.vital)
		{
			m_actor.die(CauseOfDeath::wound);
			return wound;
		}
	}
	else
	{
		Step delayTillHeal = WoundCalculations::getStepsTillHealed(hit, bodyPart.bodyPartType, scale);
		wound.healEvent.schedule(delayTillHeal, wound, *this);
	}
	recalculateBleedAndImpairment();
	return wound;
}
void Body::healWound(Wound& wound)
{
	wound.bodyPart.wounds.remove(wound);
	recalculateBleedAndImpairment();
}
void Body::doctorWound(Wound& wound, Percent healSpeedPercentageChange)
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
	m_volumeOfBlood--;
	float ratio = (float)m_volumeOfBlood / (float)healthyBloodVolume();
	if(ratio <= Config::bleedToDeathRatio)
		m_actor.die(CauseOfDeath::bloodLoss);
	else 
	{
		if (m_actor.m_mustSleep.isAwake() && ratio < Config::bleedToUnconciousessRatio)
			m_actor.passout(Config::bleedPassOutDuration);
		recalculateBleedAndImpairment();
	}
}
void Body::sever(BodyPart& bodyPart, Wound& wound)
{
	wound.maxPercentPermanantImpairment = 100u;
	wound.maxPercentTemporaryImpairment = 0u;
	bodyPart.severed = true;
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
	}
	if(m_impairManipulationPercent > 100)
		m_impairManipulationPercent = 100u;
	m_impairMovePercent = std::min(Percent(100u), m_impairMovePercent);
	if(bleedVolumeRate > 0)
	{
		// Calculate bleed event frequency from bleed volume rate.
		Step baseFrequency = Config::bleedEventBaseFrequency / bleedVolumeRate;
		Step baseWoundsCloseDelay = bleedVolumeRate * Config::ratioWoundsCloseDelayToBleedVolume;
		// Apply already elapsed progress from existing event ( if any ).
		if(m_isBleeding)
		{
			// Already bleeding, reschedule bleed event and wounds close event.
			assert(m_woundsCloseEvent.exists());
			Step adjustedFrequency = m_bleedEvent.exists() ? 
				util::scaleByInversePercent(baseFrequency, m_bleedEvent.percentComplete()) :
				baseFrequency;
			Step toScheduleStep = m_actor.getSimulation().m_step + adjustedFrequency;
			if(!m_bleedEvent.exists() || toScheduleStep != m_bleedEvent.getStep())
			{
				m_bleedEvent.maybeUnschedule();
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
			m_isBleeding = true;
			m_bleedEvent.schedule(baseFrequency, *this);
			m_woundsCloseEvent.schedule(baseWoundsCloseDelay, *this);
		}
	}
	else
	{
		// Stop bleeding if bleeding.
		m_isBleeding = false;
		m_woundsCloseEvent.maybeUnschedule();
		m_bleedEvent.maybeUnschedule();
	}
	// Update move speed for current move impairment level.
	m_actor.m_canMove.updateIndividualSpeed();
	// Update combat score for current manipulation impairment level.
	m_actor.m_canFight.update();
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
Step Body::getStepsTillBleedToDeath() const
{
	assert(m_bleedEvent.exists());
	Step output = m_bleedEvent.remainingSteps();
	if(m_volumeOfBlood > 1)
	{
		auto volume = m_volumeOfBlood - 1;
		while(true)
		{
			output += m_bleedEvent.duration();
			if(volume == 0 || (float)volume / healthyBloodVolume() <= Config::bleedToDeathRatio)
				return output;
			volume--;
		}
	}
	else
		return output;
}
bool Body::piercesSkin(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (uint32_t)((float)hit.force / (float)hit.area) * hit.materialType.hardness * Config::pierceSkinModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesFat(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (uint32_t)((float)hit.force / (float)hit.area) * hit.materialType.hardness * Config::pierceFatModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesMuscle(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (uint32_t)((float)hit.force / (float)hit.area) * hit.materialType.hardness * Config::pierceMuscelModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesBone(Hit hit, const BodyPart& bodyPart) const
{
	uint32_t pierceScore = (uint32_t)((float)hit.force / (float)hit.area) * hit.materialType.hardness * Config::pierceBoneModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.bodyPartType.volume * bodyPart.materialType.hardness;
	return pierceScore > defenseScore;
}
Volume Body::healthyBloodVolume() const
{
	return m_totalVolume * Config::ratioOfTotalBodyVolumeWhichIsBlood;
}
std::vector<Attack> Body::getMeleeAttacks() const
{
	std::vector<Attack> output;
	for(const BodyPart& bodyPart : m_bodyParts)
		for(auto& [attackType, materialType] : bodyPart.bodyPartType.attackTypesAndMaterials)
			output.emplace_back(&attackType, materialType, nullptr);
	return output;
}
Volume Body::getVolume() const
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
bool Body::hasBodyPart(const BodyPartType& bodyPartType) const 
{
	for(const BodyPart& bodyPart : m_bodyParts)
		if(bodyPart.bodyPartType == bodyPartType)
			return true;
	return false;
}
uint32_t Body::getImpairPercentFor(const BodyPartType& bodyPartType) const
{
	uint32_t output = 0;
	for(const BodyPart& bodyPart : m_bodyParts)
		if(bodyPart.bodyPartType == bodyPartType)
			for(const Wound& wound : bodyPart.wounds)
				output += wound.impairPercent();
	return output;
}
WoundHealEvent::WoundHealEvent(const Step delay, Wound& w, Body& b) : ScheduledEventWithPercent(b.m_actor.getSimulation(), delay), m_wound(w), m_body(b) {}
BleedEvent::BleedEvent(const Step delay, Body& b) : ScheduledEventWithPercent(b.m_actor.getSimulation(), delay), m_body(b) {}
WoundsCloseEvent::WoundsCloseEvent(const Step delay, Body& b) : ScheduledEventWithPercent(b.m_actor.getSimulation(), delay), m_body(b) {}
