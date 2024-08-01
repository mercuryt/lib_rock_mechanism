#pragma once

#include "attackType.h"
#include "blocks/blocks.h"
#include "eventSchedule.hpp"
#include "types.h"

#include <ratio>
#include <string>
#include <vector>
#include <unordered_map>

struct BodyPartType;
struct BodyTypeCategory;
class Area;
struct CraftJob;
struct CraftStepTypeCategory;
struct FluidType;
struct MaterialType;
struct DeserializationMemo;
struct MaterialTypeCategory;

struct WearableData final
{
	std::vector<const BodyPartType*> bodyPartsCovered;
	const uint32_t defenseScore = 0;
	const uint32_t layer = 0;
	const uint32_t bodyTypeScale = 0;
	const uint32_t forceAbsorbedUnpiercedModifier = 0;
	const uint32_t forceAbsorbedPiercedModifier = 0;
	const Percent percentCoverage = Percent::create(0);
	const bool rigid = false;
	WearableData(uint32_t ds, uint32_t l, uint32_t bts, uint32_t faum, uint32_t fapm, Percent pc, bool r) : 
		defenseScore(ds), layer(l), bodyTypeScale(bts), forceAbsorbedUnpiercedModifier(faum), forceAbsorbedPiercedModifier(fapm), 
		percentCoverage(pc), rigid(r) { }
};
inline std::deque<WearableData> wearableDataStore;
struct WeaponData final
{
	std::vector<AttackType> attackTypes;
	const SkillType* combatSkill;
	Step coolDown;
	WeaponData(const SkillType* cs, Step cd) : combatSkill(cs), coolDown(cd)  { }
	// Infastructure.
};
inline std::deque<WeaponData> weaponDataStore;
struct ItemType final
{
	std::vector<const MaterialTypeCategory*> materialTypeCategories;
	const std::string name;
	//TODO: remove craftLocationOffset?
	const std::array<int32_t, 3> craftLocationOffset = {0,0,0};
	const Shape& shape;
	const MoveType& moveType;
	const FluidType* edibleForDrinkersOf = nullptr;
       	const WearableData* wearableData = nullptr;
       	const WeaponData* weaponData = nullptr;
	const CraftStepTypeCategory* craftLocationStepTypeCategory = nullptr;
	const Volume volume = Volume::create(0);
	const Volume internalVolume = Volume::create(0);
	const uint32_t value = 0;
	const bool installable = false;
	const bool generic = false;
	const bool canHoldFluids = false;
	ItemType(std::string name, const Shape& shape, const MoveType& moveType, Volume volume, Volume internalVolume, uint32_t value, bool installable, bool generic, bool canHoldFluids);
	AttackType* getRangedAttackType() const;
	BlockIndex getCraftLocation(Blocks& blocks, BlockIndex location, Facing facing) const;
	bool hasRangedAttack() const;
	bool hasMeleeAttack() const;
	// Infastructure.
	bool operator==(const ItemType& itemType) const { return this == &itemType; }
	static const ItemType& byName(std::string name);
	static ItemType& byNameNonConst(std::string name);
};
inline std::vector<ItemType> itemTypeDataStore;
inline void to_json(Json& data, const ItemType* const& itemType){ data = itemType->name; }
inline void to_json(Json& data, const ItemType& itemType){ data = itemType.name; }
inline void from_json(const Json& data, const ItemType*& itemType){ itemType = &ItemType::byName(data.get<std::string>()); }
