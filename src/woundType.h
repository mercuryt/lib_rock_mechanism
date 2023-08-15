#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
struct Hit;
struct BodyPartType;
enum class WoundType { Pierce, Cut, Bludgeon };

namespace WoundCalculations
{
	Step getStepsTillHealed(WoundType woundType, const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale);
	uint32_t getBleedVolumeRate(WoundType woundType, const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale);
	uint32_t getPercentTemporaryImpairment(WoundType woundType, const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale);
	uint32_t getPercentPermanentImpairment(WoundType woundType, const Hit& hit, const BodyPartType& bodyPartType, uint32_t scale);
	WoundType byName(std::string name);
}
