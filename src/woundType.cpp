#include "woundType.h"
#include "body.h"
#include "hit.h"
#include "config.h"
uint32_t WoundCalculations::getStepsTillHealed(WoundType woundType, const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale)
{
	uint32_t hitVolume = hit.depth * hit.area;
	uint32_t bodyPartVolume = bodyPartType.volume * scale;
	float ratio = (float)hitVolume / (float)bodyPartVolume;
	switch (woundType)
	{
		case WoundType::Pierce:
			return ratio * Config::pierceStepsTillHealedModifier;
		case WoundType::Cut:
			return ratio * Config::cutStepsTillHealedModifier;
		case WoundType::Bludgeon:
			return ratio * Config::bludgeonStepsTillHealedModifier;
		default:
			assert(false);
	}
}
uint32_t WoundCalculations::getBleedVolumeRate(WoundType woundType, const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale)
{
	uint32_t hitVolume = hit.depth * hit.area;
	uint32_t bodyPartVolume = bodyPartType.volume * scale;
	float ratio = (float)hitVolume / (float)bodyPartVolume;
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
uint32_t WoundCalculations::getPercentTemporaryImpairment(WoundType woundType, const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale)
{
	uint32_t hitVolume = hit.depth * hit.area;
	uint32_t bodyPartVolume = bodyPartType.volume * scale;
	float ratio = (float)hitVolume / (float)bodyPartVolume;
	switch (woundType)
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
uint32_t WoundCalculations::getPercentPermanentImpairment(WoundType woundType, const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale)
{
	uint32_t hitVolume = hit.depth * hit.area;
	uint32_t bodyPartVolume = bodyPartType.volume * scale;
	float ratio = (float)hitVolume / (float)bodyPartVolume;
	uint32_t output;
	switch (woundType)
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
