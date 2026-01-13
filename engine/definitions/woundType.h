#pragma once

#include "../numericTypes/types.h"

#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
struct Hit;
class BodyPartType;
enum class WoundType { Pierce, Cut, Bludgeon, Null };

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
	Step getStepsTillHealed(const Hit& hit, const BodyPartTypeId& bodyPartType, int scale);
	int getBleedVolumeRate(const Hit& hit, const BodyPartTypeId& bodyPartType, int scale);
	Percent getPercentTemporaryImpairment(const Hit& hit, const BodyPartTypeId& bodyPartType, int scale);
	Percent getPercentPermanentImpairment(const Hit& hit, const BodyPartTypeId& bodyPartType, int scale);
	WoundType byName(std::string name);
}
