#include "woundType.h"
#include "body.h"
#include "bodyType.h"
#include "hit.h"
#include "config.h"
#include "types.h"
#include <algorithm>
Step WoundCalculations::getStepsTillHealed(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale)
{
	assert(hit.depth !=0 );
	Volume hitVolume = Volume::create(hit.depth * hit.area * Config::hitScaleModifier);
	Volume bodyPartVolume = BodyPartType::getVolume(bodyPartType) * scale;
	float ratio = (float)hitVolume.get() / (float)bodyPartVolume.get();
	Step baseDelay = Step::create(Config::baseHealDelaySteps.get() * ratio);
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
			return Step::create(0);
	}
}
uint32_t WoundCalculations::getBleedVolumeRate(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale)
{
	if(hit.depth == 0)
		return 0;
	Volume hitVolume = Volume::create(hit.depth * hit.area * Config::hitScaleModifier);
	Volume bodyPartVolume = BodyPartType::getVolume(bodyPartType) * scale;
	float ratio = (float)hitVolume.get() / (float)bodyPartVolume.get();
	WoundType woundType = hit.woundType;
	switch (woundType)
	{
		case WoundType::Pierce:
			return std::max(1u, (uint32_t)(ratio * Config::pierceBleedVoumeRateModifier));
		case WoundType::Cut:
			return std::max(1u, (uint32_t)(ratio * Config::cutBleedVoumeRateModifier));
		case WoundType::Bludgeon:
			return std::max(1u, (uint32_t)(ratio * Config::bludgeonBleedVoumeRateModifier));
		default:
			assert(false);
			return 0;
	}
}
Percent WoundCalculations::getPercentTemporaryImpairment(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale)
{
	Volume hitVolume = Volume::create(hit.depth * hit.area * Config::hitScaleModifier);
	Volume bodyPartVolume = BodyPartType::getVolume(bodyPartType) * scale;
	float ratio = (float)hitVolume.get() / (float)bodyPartVolume.get();
	switch (hit.woundType)
	{
		case WoundType::Pierce:
			return Percent::create(Config::piercePercentTemporaryImparmentModifier * ratio);
		case WoundType::Cut:
			return Percent::create(Config::cutPercentTemporaryImparmentModifier * ratio);
		case WoundType::Bludgeon:
			return Percent::create(Config::bludgeonPercentTemporaryImparmentModifier * ratio);
		default:
			assert(false);
			return Percent::create(0);
	}
}
Percent WoundCalculations::getPercentPermanentImpairment(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale)
{
	Volume hitVolume = Volume::create(hit.depth * hit.area * Config::hitScaleModifier);
	Volume bodyPartVolume = BodyPartType::getVolume(bodyPartType) * scale;
	float ratio = (float)hitVolume.get() / (float)bodyPartVolume.get();
	Percent output;
	switch (hit.woundType)
	{
		case WoundType::Pierce:
			output = Percent::create(Config::piercePercentPermanantImparmentModifier * ratio);
			if(output < Config::percentPermanantImparmentMinimum)
				return Percent::create(0);
			return output - Config::percentPermanantImparmentMinimum;
		case WoundType::Cut:
			output = Percent::create(ratio * Config::cutPercentPermanantImparmentModifier);
			if(output < Config::percentPermanantImparmentMinimum)
				return Percent::create(0);
			return output - Config::percentPermanantImparmentMinimum;
		case WoundType::Bludgeon:
			output = Percent::create(ratio * Config::bludgeonPercentPermanantImparmentModifier);
			if(output < Config::percentPermanantImparmentMinimum)
				return Percent::create(0);
			return output - Config::percentPermanantImparmentMinimum;
		default:
			assert(false);
			return Percent::create(0);
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
	{
		assert(false);
		return WoundType::Pierce;
	}
}
