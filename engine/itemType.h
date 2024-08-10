#pragma once

#include "attackType.h"
#include "blocks/blocks.h"
#include "dataVector.h"
#include "eventSchedule.hpp"
#include "types.h"

#include <ratio>
#include <string>
#include <vector>
#include <unordered_map>

class Area;

struct ItemTypeParamaters final
{
	//TODO: remove craftLocationOffset?
	std::vector<MaterialCategoryTypeId> materialTypeCategories = {};
	std::string name;
	std::array<int32_t, 3> craftLocationOffset = {};
	ShapeId shape;
	MoveTypeId moveType;
	FluidTypeId edibleForDrinkersOf = FluidTypeId::null();
	CraftStepTypeCategoryId craftLocationStepTypeCategory = CraftStepTypeCategoryId::null();
	Volume volume;
	Volume internalVolume;
	uint32_t value;
	bool installable;
	bool generic;
	bool canHoldFluids;
	std::vector<AttackTypeId> attackTypes = {};
	SkillTypeId combatSkill = SkillTypeId::null();
	Step attackCoolDownBase = Step::null();
	uint32_t wearable_defenseScore = 0;
	uint32_t wearable_layer = 0;
	uint32_t wearable_bodyTypeScale = 0;
	uint32_t wearable_forceAbsorbedUnpiercedModifier = 0;
	uint32_t wearable_forceAbsorbedPiercedModifier = 0;
	Percent wearable_percentCoverage = Percent::null();
	bool wearable_rigid = false;
	std::vector<BodyPartTypeId> wearable_bodyPartsCovered = {};
};
class ItemType final
{
	static ItemType data;
	//TODO: remove craftLocationOffset?
	DataVector<std::vector<MaterialCategoryTypeId>, ItemTypeId> m_materialTypeCategories;
	DataVector<std::string, ItemTypeId> m_name;
	DataVector<std::array<int32_t, 3>, ItemTypeId> m_craftLocationOffset;
	DataVector<ShapeId, ItemTypeId> m_shape;
	DataVector<MoveTypeId, ItemTypeId> m_moveType;
	DataVector<FluidTypeId, ItemTypeId> m_edibleForDrinkersOf;
	DataVector<CraftStepTypeCategoryId, ItemTypeId> m_craftLocationStepTypeCategory;
	DataVector<Volume, ItemTypeId> m_volume;
	DataVector<Volume, ItemTypeId> m_internalVolume;
	DataVector<uint32_t, ItemTypeId> m_value;
	DataBitSet<ItemTypeId> m_installable;
	DataBitSet<ItemTypeId> m_generic;
	DataBitSet<ItemTypeId> m_canHoldFluids;
	DataVector<std::vector<AttackTypeId>, ItemTypeId> m_attackTypes;
	DataVector<SkillTypeId, ItemTypeId> m_combatSkill;
	DataVector<Step, ItemTypeId> m_attackCoolDownBase;
	DataVector<uint32_t, ItemTypeId> m_wearable_defenseScore;
	DataVector<uint32_t, ItemTypeId> m_wearable_layer;
	DataVector<uint32_t, ItemTypeId> m_wearable_bodyTypeScale;
	DataVector<uint32_t, ItemTypeId> m_wearable_forceAbsorbedUnpiercedModifier;
	DataVector<uint32_t, ItemTypeId> m_wearable_forceAbsorbedPiercedModifier;
	DataVector<Percent, ItemTypeId> m_wearable_percentCoverage;
	DataBitSet<ItemTypeId> m_wearable_rigid;
	DataVector<std::vector<BodyPartTypeId>, ItemTypeId> m_wearable_bodyPartsCovered;
public:
	static void create(ItemTypeParamaters& p);
	static AttackTypeId getRangedAttackType(ItemTypeId id);
	static BlockIndex getCraftLocation(ItemTypeId id, Blocks& blocks, BlockIndex location, Facing facing);
	static bool hasRangedAttack(ItemTypeId id);
	static bool hasMeleeAttack(ItemTypeId id);
	static const ItemTypeId byName(std::string name);
	[[nodiscard]] static std::vector<MaterialCategoryTypeId>& getMaterialTypeCategories(ItemTypeId id) { return data.m_materialTypeCategories[id]; }
	[[nodiscard]] static std::string& getName(ItemTypeId id) { return data.m_name[id]; }
	[[nodiscard]] static std::array<int32_t, 3>& getCraftLocationOffset(ItemTypeId id) { return data.m_craftLocationOffset[id]; }
	[[nodiscard]] static ShapeId getShape(ItemTypeId id) { return data.m_shape[id]; }
	[[nodiscard]] static MoveTypeId getMoveType(ItemTypeId id) { return data.m_moveType[id]; }
	[[nodiscard]] static FluidTypeId getEdibleForDrinkersOf(ItemTypeId id) { return data.m_edibleForDrinkersOf[id]; }
	[[nodiscard]] static CraftStepTypeCategoryId getCraftLocationStepTypeCategory(ItemTypeId id) { return data.m_craftLocationStepTypeCategory[id]; }
	[[nodiscard]] static Volume getVolume(ItemTypeId id) { return data.m_volume[id]; }
	[[nodiscard]] static Volume getInternalVolume(ItemTypeId id) { return data.m_internalVolume[id]; }
	[[nodiscard]] static uint32_t getValue(ItemTypeId id) { return data.m_value[id]; }
	[[nodiscard]] static bool getInstallable(ItemTypeId id) { return data.m_installable[id]; }
	[[nodiscard]] static bool getGeneric(ItemTypeId id) { return data.m_generic[id]; }
	[[nodiscard]] static bool getCanHoldFluids(ItemTypeId id) { return data.m_canHoldFluids[id]; }
	[[nodiscard]] static std::vector<AttackTypeId> getAttackTypes(ItemTypeId id) { return data.m_attackTypes[id]; }
	[[nodiscard]] static SkillTypeId getCombatSkill(ItemTypeId id) { return data.m_combatSkill[id]; }
	[[nodiscard]] static Step getAttackCoolDownBase(ItemTypeId id) { return data.m_attackCoolDownBase[id]; }
	[[nodiscard]] static bool getIsWeapon(ItemTypeId id) { return !data.m_attackTypes[id].empty(); }
	[[nodiscard]] static bool getIsGeneric(ItemTypeId id) { return !data.m_generic[id]; }
	[[nodiscard]] static uint32_t getWearable_defenseScore(ItemTypeId id) { return data.m_wearable_defenseScore[id]; }
	[[nodiscard]] static uint32_t getWearable_layer(ItemTypeId id) { return data.m_wearable_layer[id]; }
	[[nodiscard]] static uint32_t getWearable_bodyTypeScale(ItemTypeId id) { return data.m_wearable_bodyTypeScale[id]; }
	[[nodiscard]] static uint32_t getWearable_forceAbsorbedUnpiercedModifier(ItemTypeId id) { return data.m_wearable_forceAbsorbedUnpiercedModifier[id]; }
	[[nodiscard]] static uint32_t getWearable_forceAbsorbedPiercedModifier(ItemTypeId id) { return data.m_wearable_forceAbsorbedPiercedModifier[id]; }
	[[nodiscard]] static Percent getWearable_percentCoverage(ItemTypeId id) { return data.m_wearable_percentCoverage[id]; }
	[[nodiscard]] static bool getWearable_rigid(ItemTypeId id) { return data.m_wearable_rigid[id]; }
	[[nodiscard]] static std::vector<BodyPartTypeId>& getWearable_bodyPartsCovered(ItemTypeId id) { return data.m_wearable_bodyPartsCovered[id]; }
	[[nodiscard]] static bool getIsWearable(ItemTypeId id) { return !data.m_wearable_bodyPartsCovered[id].empty(); }
};
