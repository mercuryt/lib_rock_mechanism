#pragma once

#include "numericTypes/types.h"

#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
struct Hit;
class BodyPartType;
enum class WoundType { Pierce, Cut, Bludgeon };

inline WoundType woundTypeByName(std::string name)
{
	if(name == "Pierce")
		return WoundType::Pierce;
	if(name == "Cut")
		return WoundType::Cut;
	assert(name == "Bludgeon");
	return WoundType::Bludgeon;
}
inline std::string getWoundTypeName(const WoundType& woundType)
{
	if(woundType == WoundType::Pierce)
		return "Pierce";
	if(woundType == WoundType::Cut)
		return "Cut";
	assert(woundType == WoundType::Bludgeon);
	return "Bludgeon";
}
namespace WoundCalculations
{
	Step getStepsTillHealed(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale);
	uint32_t getBleedVolumeRate(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale);
	Percent getPercentTemporaryImpairment(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale);
	Percent getPercentPermanentImpairment(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale);
	WoundType byName(std::string name);
}
