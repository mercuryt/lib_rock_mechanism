#pragma once

#include "body.h"

#include <string>
#include <vector>
#include <unordered_set>


template<class WoundType>
struct AttackType
{
	std::string name;
	uint32_t area;
	uint32_t baseForce;
	const WoundType& woundType;
};
class WeaponType
{
	const std::vector<AttackType> attackTypes;
};
class WearableType
{
	const uint32_t percentCoverage;
	const std::unordered_set<BodyPartType*> bodyPartsCovered;
	const uint32_t defenseScore;
	const bool rigid;
	const uint32_t impactSpreadArea;
	const uint32_t forceAbsorbedPiercedModifier;
	const uint32_t forceAbsorbedUnpiercedModifier;

};
class ItemType
{
	const std::string name;
	const MaterialType& materialType;
	const uint32_t quality;
	const uint32_t wear;
	const WeaponType& weaponType;
       	const WearableType& wearableType;
	const GenericType& genericType;
};
