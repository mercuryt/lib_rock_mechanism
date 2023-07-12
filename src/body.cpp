#include "body.h"
#include "actor.h"
#include "config.h"
#include "randomUtil.h"
uint32_t Wound::getPercentHealed() const
{
	uint32_t output = percentHealed;
	if(healEvent.exists())
		output += util::scaleByInversePercent(percentHealed, healEvent.percentComplete());
	return output;
}
uint32_t Wound::impairPercent() const
{
	return util::scaleByInversePercent(maxPercentTemporaryImpairment, getPercentHealed()) + maxPercentPermanantImpairment;
}
Body::Body(Actor& a, const BodyType& bodyType) :  m_actor(a), m_totalVolume(0), m_impairMovePercent(0), m_impairManipulationPercent(0)
{
	for(auto& [bodyPartType, materialType] : bodyType.bodyParts)
	{
		m_bodyParts.emplace_back(*bodyPartType, *materialType);
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
void Body::getHitDepth(Hit& hit, const BodyPart& bodyPart, const MaterialType& materialType)
{
	assert(hit.depth == 0);
	if(piercesSkin(hit, bodyPart))
	{
		++hit.depth;
		hit.force -= materialType.hardness * hit.area * Config::skinPierceForceCost;
		if(piercesFat(hit, bodyPart))
		{
			++hit.depth;
			hit.force -= materialType.hardness * hit.area * Config::fatPierceForceCost;
			if(piercesMuscle(hit, bodyPart))
			{
				++hit.depth;
				hit.force -= materialType.hardness * hit.area * Config::musclePierceForceCost;
				if(piercesBone(hit, bodyPart))
					++hit.depth;
			}
		}
	}
}
Wound& Body::addWound(const WoundType& woundType, BodyPart& bodyPart, const Hit& hit)
{
	uint32_t bleedVolumeRate = woundType.getBleedVolumeRate(hit, bodyPart.bodyPartType);
	Wound& wound = bodyPart.wounds.emplace_back(woundType, bodyPart, hit, bleedVolumeRate);
	wound.healEvent.schedule(wound, *this);
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
	uint32_t remaningSteps = wound.healEvent.remaningSteps();
	remaningSteps = util::scaleByPercent(remaningSteps, healSpeedPercentageChange);
	wound.healEvent.cancel();
	wound.healEvent.schedule(remaningSteps, wound);
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
		m_woundsCloseEvent.cancel();
	if(bleedVolumeRate > 0 && !m_woundsCloseEvent.exists())
	{
		uint32_t delay = bleedVolumeRate * Config::ratioWoundsCloseDelayToBleedVolume;
		m_woundsCloseEvent.schedule(delay, *this);
	}
	else if(bleedVolumeRate == 0 && m_woundsCloseEvent.exists())
		m_woundsCloseEvent.cancel();
	m_actor.m_attributes.generate();
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
		for(auto& [materialType, attackType] : bodyPart.bodyPartType.attackTypes)
			output.emplace_back(attackType, *materialType, nullptr);
	return output;
}
