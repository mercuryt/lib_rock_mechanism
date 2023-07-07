#include "body.h"
Wound::Wound(const WoundType& wt, BodyPart& bp, uint32_t m_size, uint32_t ph = 0) : m_woundType(wt), m_bodyPart(bp), m_percentHealed(ph), m_healEvent(nullptr) {}
uint32_t Wound::getPercentHealed() const
{
	uint32_t output = m_percentHealed;
	if(m_healEvent != nullptr)
		output += util::scaleByPercent(100u - m_percentHealed, m_healEvent.percentComplete());
	return output;
}
~Wound::Wound()
{
	if(m_healEvent != nullptr)
		m_healEvent.cancel();
}
BodyPart::BodyPart(const BodyPartType& bpt, const MaterialType mt) : m_bodyPartType(bpt), m_materialType(mt) {}
Body::Body(const BodyType& bodyType) :  m_actor(a), m_totalVolume(0), m_impairMovePercent(0), m_impairManipulationPercent(0)
{
	for(auto& [bodyPartType, materialType] : bodyType.bodyParts)
	{
		m_bodyParts.emplace_back(bodyPartType, materialType);
		totalVolume += m_bodyParts.volume;
	}
	m_volumeOfBlood = healthyBloodVolume();
}
BodyPart& Body::pickABodyPartByVolume()
{
	uint32_t roll = randomUtil::getInRange(0, m_totalVolume);
	for(BodyPart& bodyPart : m_bodyParts)
	{
		if(bodyPart.volume >= roll)
			return bodyPart;
		else
			roll -= bodyPart.volume
	}
	assert(false);
}
// Armor has already been applied, calculate hit depth.
void Body::getHitDepth(Hit& hit, const BodyPart& bodyPart, const MaterialType& materialType)
{
	assert(hit.depth == 0);
	if(piercesSkin(hit))
	{
		++hit.depth;
		hit.m_force -= materialType.hardness * hit.m_area * Config::skinPierceForceCost;
		if(piercesFat(hit))
		{
			++hit.depth;
			hit.m_force -= materialType.hardness * hit.m_area * Config::fatPierceForceCost;
			if(piercesMuscle(hit))
			{
				++hit.depth;
				hit.m_force -= materialType.hardness * hit.m_area * Config::musclePierceForceCost;
				if(piercesBone(hit))
					++hit.depth;
			}
		}
	}
}
Wound& Body::addWound(const WoundType& woundType, const BodyPart& bodyPart, const Hit& hit)
{
	Wound& wound = bodyPart.m_wounds.emplace_back(woundType, bodyPart, hit);
	std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<WoundHealEvent>(wound, *this);
	wound.m_healEvent = event.get();
	m_actor.m_location->m_area->m_eventSchedule.schedule(event);
	recalculateBleedAndImpairment();
	return wound;
}
void Body::healWound(Wound& wound)
{
	wound.bodyPart.m_wounds.remove(wound);
	recalculateBleedAndImpairment();
}
void Body::doctorWound(Wound& wound, uint32_t healSpeedPercentageChange)
{
	if(wound.bleedVolumeRate != 0)
	{
		wound.bleedVolumeRate = 0;
		recalculateBleedAndImpairment();
	}
	assert(wound.m_healEvent != nullptr);
	uint32_t remaningSteps = wound.m_healEvent.remaningSteps();
	remaningSteps = util.scaleByPercent(remaningSteps, healSpeedPercentageChange);
	wound.m_healEvent.cancel();
	std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<WoundHealEvent>(remaningSteps, wound);
	wound.m_healEvent = event.get();
	m_actor.m_location->m_area->m_eventSchedule.scedule(event);
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
		m_actor.die();
	else if (ratio < Config::bleedToUnconciousessRatio)
		m_actor.passOut(Config::bleedPassOutDuration);
}
void Body::recalculateBleedAndImpairment()
{
	uint32_t bleedVolumeRate = 0;
	m_impairManipulationPercent = 0;
	m_impairMoveSpeedPercent = 0;
	for(BodyPart& bodyPart : m_bodyParts)
	{
		for(Wound& wound : bodyPart.wounds)
		{
			bleedVolumeRate += wound.bleedVolumeRate; // when wound closes set to 0.
			if(bodyPart.bodyPartType.doesLocamotion)
				m_impairMovePercent += wound.impairPercent;
			if(bodyPart.bodyPartType.doesManipulation)
				m_impairManipulationPercent += wound.impairPercent;
		}
		m_impairManipulationPercent = std::min(100, m_impairManipulationPercent);
		m_impairMovePercent = std::min(100, m_impairMovePercent);
	}
	if(bleedVolumeRate > 0 && m_bleedEvent == nullptr)
	{
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<BleedEvent>(Config::bleedEventFrequency, *this);
		m_bleedEvent = event.get();
		m_actor.m_location->m_area->m_eventSchedule.schedule(std::move(event));
	}
	else if(bleedVolumeRate == 0 && m_woundsCloseEvent != nullptr)
		m_bleedEvent.cancel();
	if(bleedVolumeRate > 0 && m_woundsCloseEvent == nullptr)
	{
		uint32_t delay = bleedVolumeRate * Config::ratioWoundsCloseDelayToBleedVolume;
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<WoundsCloseEvent>(delay, *this);
		m_woundsCloseEvent = event.get();
		m_actor.m_location->m_area->m_eventSchedule.schedule(std::move(event));
	}
	else if(bleedVolumeRate == 0 && m_woundsCloseEvent != nullptr)
		m_woundsCloseEvent.cancel();
	m_actor.statsNoLongerValid();
}
bool Body::piercesSkin(Hit hit, BodyPart& bodyPart)
{
	uint32_t pierceScore = (force / area) * materialType.hardness * Config::pierceSkinModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.m_bodyPartType.area * bodyPart.m_materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesFat(Hit hit, BodyPart& bodyPart)
{
	uint32_t pierceScore = (force / area) * materialType.hardness * Config::pierceFatModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.m_bodyPartType.area * bodyPart.m_materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesMuscle(Hit hit, BodyPart& bodyPart)
{
	uint32_t pierceScore = (force / area) * materialType.hardness * Config::pierceMuscelModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.m_bodyPartType.area * bodyPart.m_materialType.hardness;
	return pierceScore > defenseScore;
}
bool Body::piercesBone(Hit hit, BodyPart& bodyPart)
{
	uint32_t pierceScore = (force / area) * materialType.hardness * Config::pierceBoneModifier;
	uint32_t defenseScore = Config::bodyHardnessModifier * bodyPart.m_bodyPartType.area * bodyPart.m_materialType.hardness;
	return pierceScore > defenseScore;
}
uint32_t Body::healthyBloodVolume() const
{
	return m_totalVolume * Config::ratioOfTotalBodyVolumeWhichIsBlood;
}
std::vector<Attack> getAttacks()
{
	std::vector<Attack> output;
	for(BodyPart* bodyPart : m_bodyParts)
		for(AttackType* attackType : bodyPart.m_bodyPartType.attackTypes)
			output.emplace_back(attackType, equipment->m_materialType, equipment);
	return output;
}
