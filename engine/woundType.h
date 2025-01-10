#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
struct Hit;
class BodyPartType;
enum class WoundType { Pierce, Cut, Bludgeon };

inline WoundType woundTypeByName(std::wstring name)
{
	if(name == L"Pierce")
		return WoundType::Pierce;
	if(name == L"Cut")
		return WoundType::Cut;
	assert(name == L"Bludgeon");
	return WoundType::Bludgeon;
}
inline std::wstring getWoundTypeName(const WoundType& woundType)
{
	if(woundType == WoundType::Pierce)
		return L"Pierce";
	if(woundType == WoundType::Cut)
		return L"Cut";
	assert(woundType == WoundType::Bludgeon);
	return L"Bludgeon";
}
namespace WoundCalculations
{
	Step getStepsTillHealed(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale);
	uint32_t getBleedVolumeRate(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale);
	Percent getPercentTemporaryImpairment(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale);
	Percent getPercentPermanentImpairment(const Hit& hit, const BodyPartTypeId& bodyPartType, uint32_t scale);
	WoundType byName(std::wstring name);
}
