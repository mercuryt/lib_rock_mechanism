#include "woundType.h"
#include "body.h"
#include "hit.h"
#include "config.h"
Step WoundCalculations::getStepsTillHealed(const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale)
{
	assert(hit.depth !=0 );
	uint32_t hitVolume = hit.depth * hit.area * Config::hitScaleModifier;
	uint32_t bodyPartVolume = bodyPartType.volume * scale;
	float ratio = (float)hitVolume / (float)bodyPartVolume;
	Step baseDelay = ratio * Config::baseHealDelaySteps;
	switch (hit.woundType)
	{
		case WoundType::Pierce:
			return baseDelay * Config::pierceStepsTillHealedModifier;
		case WoundType::Cut:
			return baseDelay * Config::cutStepsTillHealedModifier;
		case WoundType::Bludgeon:
			return baseDelay * Config::bludgeonStepsTillHealedModifier;
		default:
			assert(false);
	}
}
uint32_t WoundCalculations::getBleedVolumeRate(const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale)
{
	uint32_t hitVolume = hit.depth * hit.area * Config::hitScaleModifier;
	uint32_t bodyPartVolume = bodyPartType.volume * scale;
	float ratio = (float)hitVolume / (float)bodyPartVolume;
	WoundType woundType = hit.woundType;
	switch (woundType)
	{
		case WoundType::Pierce:
			return ratio * Config::pierceBleedVoumeRateModifier;
		case WoundType::Cut:
			return ratio * Config::cutBleedVoumeRateModifier;
		case WoundType::Bludgeon:
			return ratio * Config::bludgeonBleedVoumeRateModifier;
		default:
			assert(false);
	}
}
uint32_t WoundCalculations::getPercentTemporaryImpairment(const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale)
{
	uint32_t hitVolume = hit.depth * hit.area * Config::hitScaleModifier;
	uint32_t bodyPartVolume = bodyPartType.volume * scale;
	float ratio = (float)hitVolume / (float)bodyPartVolume;
	switch (hit.woundType)
	{
		case WoundType::Pierce:
			return ratio * Config::piercePercentTemporaryImparmentModifier;
		case WoundType::Cut:
			return ratio * Config::cutPercentTemporaryImparmentModifier;
		case WoundType::Bludgeon:
			return ratio * Config::bludgeonPercentTemporaryImparmentModifier;
		default:
			assert(false);
	}
}
uint32_t WoundCalculations::getPercentPermanentImpairment(const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale)
{
	uint32_t hitVolume = hit.depth * hit.area * Config::hitScaleModifier;
	uint32_t bodyPartVolume = bodyPartType.volume * scale;
	float ratio = (float)hitVolume / (float)bodyPartVolume;
	uint32_t output;
	switch (hit.woundType)
	{
		case WoundType::Pierce:
			output = ratio * Config::piercePercentPermanantImparmentModifier;
			if(output < Config::percentPermanantImparmentMinimum)
				return 0;
			return output - Config::percentPermanantImparmentMinimum;
		case WoundType::Cut:
			output = ratio * Config::cutPercentPermanantImparmentModifier;
			if(output < Config::percentPermanantImparmentMinimum)
				return 0;
			return output - Config::percentPermanantImparmentMinimum;
		case WoundType::Bludgeon:
			output = ratio * Config::bludgeonPercentPermanantImparmentModifier;
			if(output < Config::percentPermanantImparmentMinimum)
				return 0;
			return output - Config::percentPermanantImparmentMinimum;
		default:
			assert(false);
	}
}
WoundType WoundCalculations::byName(std::string name)
{
	if(name == "Pierce")
		return WoundType::Pierce;
	if(name == "Cut")
		return WoundType::Cut;
	if(name == "Bludgeon")
		return WoundType::Bludgeon;
	else
		assert(false);
}
