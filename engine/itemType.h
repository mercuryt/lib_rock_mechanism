#pragma once

#include "attackType.h"
#include "blocks/blocks.h"
#include "dataVector.h"
#include "eventSchedule.hpp"
#include "types.h"

#include <ratio>
#include <string>
#include <vector>

class Area;

struct ItemTypeParamaters final
{
	//TODO: remove craftLocationOffset?
	std::vector<MaterialCategoryTypeId> materialTypeCategories = {};
	std::wstring name;
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
	//TODO: remove craftLocationOffset?
	DataVector<std::vector<MaterialCategoryTypeId>, ItemTypeId> m_materialTypeCategories;
	DataVector<std::wstring, ItemTypeId> m_name;
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
	[[nodiscard]] static ItemTypeId size();
	[[nodiscard]] static AttackTypeId getRangedAttackType(const ItemTypeId& id);
	[[nodiscard]] static BlockIndex getCraftLocation(const ItemTypeId& id, Blocks& blocks, const BlockIndex& location, const Facing& facing);
	[[nodiscard]] static bool hasRangedAttack(const ItemTypeId& id);
	[[nodiscard]] static bool hasMeleeAttack(const ItemTypeId& id);
	[[nodiscard]] static const ItemTypeId byName(std::wstring name);
	[[nodiscard]] static std::vector<MaterialCategoryTypeId>& getMaterialTypeCategories(const ItemTypeId& id);
	[[nodiscard]] static std::wstring& getName(const ItemTypeId& id);
	[[nodiscard]] static std::array<int32_t, 3>& getCraftLocationOffset(const ItemTypeId& id);
	[[nodiscard]] static ShapeId getShape(const ItemTypeId& id);
	[[nodiscard]] static MoveTypeId getMoveType(const ItemTypeId& id);
	[[nodiscard]] static FluidTypeId getEdibleForDrinkersOf(const ItemTypeId& id);
	[[nodiscard]] static CraftStepTypeCategoryId getCraftLocationStepTypeCategory(const ItemTypeId& id);
	[[nodiscard]] static Volume getVolume(const ItemTypeId& id);
	[[nodiscard]] static Volume getInternalVolume(const ItemTypeId& id);
	[[nodiscard]] static uint32_t getValue(const ItemTypeId& id);
	[[nodiscard]] static bool getInstallable(const ItemTypeId& id);
	[[nodiscard]] static bool getGeneric(const ItemTypeId& id);
	[[nodiscard]] static bool getCanHoldFluids(const ItemTypeId& id);
	[[nodiscard]] static std::vector<AttackTypeId> getAttackTypes(const ItemTypeId& id);
	[[nodiscard]] static SkillTypeId getCombatSkill(const ItemTypeId& id);
	[[nodiscard]] static Step getAttackCoolDownBase(const ItemTypeId& id);
	[[nodiscard]] static bool getIsWeapon(const ItemTypeId& id);
	[[nodiscard]] static bool getIsGeneric(const ItemTypeId& id);
	[[nodiscard]] static uint32_t getWearable_defenseScore(const ItemTypeId& id);
	[[nodiscard]] static uint32_t getWearable_layer(const ItemTypeId& id);
	[[nodiscard]] static uint32_t getWearable_bodyTypeScale(const ItemTypeId& id);
	[[nodiscard]] static uint32_t getWearable_forceAbsorbedUnpiercedModifier(const ItemTypeId& id);
	[[nodiscard]] static uint32_t getWearable_forceAbsorbedPiercedModifier(const ItemTypeId& id);
	[[nodiscard]] static Percent getWearable_percentCoverage(const ItemTypeId& id);
	[[nodiscard]] static bool getWearable_rigid(const ItemTypeId& id);
	[[nodiscard]] static std::vector<BodyPartTypeId>& getWearable_bodyPartsCovered(const ItemTypeId& id);
	[[nodiscard]] static bool getIsWearable(const ItemTypeId& id);
};
inline ItemType itemTypeData;
