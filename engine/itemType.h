#pragma once

#include "attackType.h"
#include "dataVector.h"
#include "eventSchedule.hpp"
#include "types.h"
#include "items/constructed.h"
#include "geometry/offsetCuboidSet.h"

#include <ratio>
#include <string>
#include <vector>

class Area;
class Blocks;

struct ItemTypeParamaters final
{
	//TODO: remove craftLocationOffset?
	std::unique_ptr<ConstructedShape> constructedShape = nullptr;
	std::vector<MaterialCategoryTypeId> materialTypeCategories = {};
	std::string name;
	std::array<int32_t, 3> craftLocationOffset = {};
	ShapeId shape;
	MoveTypeId moveType;
	Force motiveForce = Force::create(0);
	FluidTypeId edibleForDrinkersOf = FluidTypeId::null();
	CraftStepTypeCategoryId craftLocationStepTypeCategory = CraftStepTypeCategoryId::null();
	FullDisplacement volume;
	FullDisplacement internalVolume;
	uint32_t value;
	OffsetCuboidSet decks = {};
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
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ItemTypeParamaters,
		constructedShape, materialTypeCategories, name, craftLocationOffset, shape, moveType, motiveForce, edibleForDrinkersOf, craftLocationStepTypeCategory,
		volume, internalVolume, value, decks, installable, generic, canHoldFluids, attackTypes, combatSkill, attackCoolDownBase, wearable_defenseScore,
		wearable_layer, wearable_bodyTypeScale, wearable_forceAbsorbedUnpiercedModifier, wearable_forceAbsorbedPiercedModifier, wearable_percentCoverage,
		wearable_rigid, wearable_bodyPartsCovered
	);
};
class ItemType final
{
	//TODO: remove craftLocationOffset?
	StrongVector<std::unique_ptr<ConstructedShape>, ItemTypeId> m_constructedShape;
	StrongVector<std::vector<MaterialCategoryTypeId>, ItemTypeId> m_materialTypeCategories;
	StrongVector<std::string, ItemTypeId> m_name;
	StrongVector<std::array<int32_t, 3>, ItemTypeId> m_craftLocationOffset;
	StrongVector<ShapeId, ItemTypeId> m_shape;
	StrongVector<MoveTypeId, ItemTypeId> m_moveType;
	StrongVector<FluidTypeId, ItemTypeId> m_edibleForDrinkersOf;
	StrongVector<CraftStepTypeCategoryId, ItemTypeId> m_craftLocationStepTypeCategory;
	StrongVector<FullDisplacement, ItemTypeId> m_fullDisplacement;
	StrongVector<FullDisplacement, ItemTypeId> m_internalVolume;
	StrongVector<uint32_t, ItemTypeId> m_value;
	StrongBitSet<ItemTypeId> m_installable;
	StrongBitSet<ItemTypeId> m_generic;
	StrongBitSet<ItemTypeId> m_canHoldFluids;
	StrongVector<std::vector<AttackTypeId>, ItemTypeId> m_attackTypes;
	StrongVector<SkillTypeId, ItemTypeId> m_combatSkill;
	StrongVector<Step, ItemTypeId> m_attackCoolDownBase;
	StrongVector<OffsetCuboidSet, ItemTypeId> m_decks;
	StrongVector<Force, ItemTypeId> m_motiveForce;
	StrongVector<uint32_t, ItemTypeId> m_wearable_defenseScore;
	StrongVector<uint32_t, ItemTypeId> m_wearable_layer;
	StrongVector<uint32_t, ItemTypeId> m_wearable_bodyTypeScale;
	StrongVector<uint32_t, ItemTypeId> m_wearable_forceAbsorbedUnpiercedModifier;
	StrongVector<uint32_t, ItemTypeId> m_wearable_forceAbsorbedPiercedModifier;
	StrongVector<Percent, ItemTypeId> m_wearable_percentCoverage;
	StrongBitSet<ItemTypeId> m_wearable_rigid;
	StrongVector<std::vector<BodyPartTypeId>, ItemTypeId> m_wearable_bodyPartsCovered;
public:
	static void create(const ItemTypeId& id, const ItemTypeParamaters& p);
	static ItemTypeId create(const ItemTypeParamaters& p);
	[[nodiscard]] static ItemTypeId size();
	[[nodiscard]] static AttackTypeId getRangedAttackType(const ItemTypeId& id);
	[[nodiscard]] static BlockIndex getCraftLocation(const ItemTypeId& id, Blocks& blocks, const BlockIndex& location, const Facing4& facing);
	[[nodiscard]] static bool hasRangedAttack(const ItemTypeId& id);
	[[nodiscard]] static bool hasMeleeAttack(const ItemTypeId& id);
	[[nodiscard]] static const ItemTypeId byName(std::string name);
	[[nodiscard]] static const std::unique_ptr<ConstructedShape>& getConstructedShape(const ItemTypeId& id);
	[[nodiscard]] static std::vector<MaterialCategoryTypeId>& getMaterialTypeCategories(const ItemTypeId& id);
	[[nodiscard]] static std::string& getName(const ItemTypeId& id);
	[[nodiscard]] static std::array<int32_t, 3>& getCraftLocationOffset(const ItemTypeId& id);
	[[nodiscard]] static ShapeId getShape(const ItemTypeId& id);
	[[nodiscard]] static MoveTypeId getMoveType(const ItemTypeId& id);
	[[nodiscard]] static FluidTypeId getEdibleForDrinkersOf(const ItemTypeId& id);
	[[nodiscard]] static CraftStepTypeCategoryId getCraftLocationStepTypeCategory(const ItemTypeId& id);
	[[nodiscard]] static FullDisplacement getFullDisplacement(const ItemTypeId& id);
	[[nodiscard]] static FullDisplacement getInternalVolume(const ItemTypeId& id);
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
	[[nodiscard]] static const OffsetCuboidSet& getDecks(const ItemTypeId& id);
	[[nodiscard]] static const Force getMotiveForce(const ItemTypeId& id);
	template<typename Action>
	void forEachVector(Action action);
};
inline ItemType itemTypeData;

template<typename Action>
void ItemType::forEachVector(Action action)
{
		action(itemTypeData.m_constructedShape);
		action(itemTypeData.m_materialTypeCategories);
		action(itemTypeData.m_name);
		action(itemTypeData.m_craftLocationOffset);
		action(itemTypeData.m_shape);
		action(itemTypeData.m_moveType);
		action(itemTypeData.m_edibleForDrinkersOf);
		action(itemTypeData.m_craftLocationStepTypeCategory);
		action(itemTypeData.m_fullDisplacement);
		action(itemTypeData.m_internalVolume);
		action(itemTypeData.m_value);
		action(itemTypeData.m_installable);
		action(itemTypeData.m_generic);
		action(itemTypeData.m_canHoldFluids);
		action(itemTypeData.m_attackTypes);
		action(itemTypeData.m_combatSkill);
		action(itemTypeData.m_attackCoolDownBase);
		action(itemTypeData.m_decks);
		action(itemTypeData.m_motiveForce);
		action(itemTypeData.m_wearable_defenseScore);
		action(itemTypeData.m_wearable_layer);
		action(itemTypeData.m_wearable_bodyTypeScale);
		action(itemTypeData.m_wearable_forceAbsorbedUnpiercedModifier);
		action(itemTypeData.m_wearable_forceAbsorbedPiercedModifier);
		action(itemTypeData.m_wearable_percentCoverage);
		action(itemTypeData.m_wearable_rigid);
		action(itemTypeData.m_wearable_bodyPartsCovered);
}