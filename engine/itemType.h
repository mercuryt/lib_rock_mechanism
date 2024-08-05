#pragma once

#include "attackType.h"
#include "blocks/blocks.h"
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
	std::vector<MaterialCategoryTypeId> materialTypeCategories;
	std::string name;
	std::array<int32_t, 3> craftLocationOffset;
	Shape* shape;
	MoveTypeId moveType;
	FluidTypeId edibleForDrinkersOf;
	CraftStepTypeCategoryId craftLocationStepTypeCategory;
	Volume volume;
	Volume internalVolume;
	uint32_t value;
	bool installable;
	bool generic;
	bool canHoldFluids;
	std::vector<AttackTypeId> attackTypes;
	SkillTypeId combatSkill;
	Step attackCoolDownBase;
	uint32_t wearable_defenseScore;
	uint32_t wearable_layer;
	uint32_t wearable_bodyTypeScale;
	uint32_t wearable_forceAbsorbedUnpiercedModifier;
	uint32_t wearable_forceAbsorbedPiercedModifier;
	Percent wearable_percentCoverage;
	bool wearable_rigid;
};
class ItemType final
{
	static ItemType data;
	//TODO: remove craftLocationOffset?
	DataVector<std::vector<MaterialCategoryTypeId>, ItemTypeId> m_materialTypeCategories;
	DataVector<std::string, ItemTypeId> m_name;
	DataVector<std::array<int32_t, 3>, ItemTypeId> m_craftLocationOffset;
	DataVector<Shape*, ItemTypeId> m_shape;
	DataVector<MoveTypeId, ItemTypeId> m_moveType;
	DataVector<FluidTypeId, ItemTypeId> m_edibleForDrinkersOf;
	DataVector<CraftStepTypeCategoryId, ItemTypeId> m_craftLocationStepTypeCategory;
	DataVector<Volume, ItemTypeId> m_volume;
	DataVector<Volume, ItemTypeId> m_internalVolume;
	DataVector<uint32_t, ItemTypeId> m_value;
	DataVector<bool, ItemTypeId> m_installable;
	DataVector<bool, ItemTypeId> m_generic;
	DataVector<bool, ItemTypeId> m_canHoldFluids;
	DataVector<std::vector<AttackTypeId>, ItemTypeId> m_attackTypes;
	DataVector<SkillTypeId, ItemTypeId> m_combatSkill;
	DataVector<Step, ItemTypeId> m_attackCoolDownBase;
	DataVector<uint32_t, ItemTypeId> m_wearable_defenseScore;
	DataVector<uint32_t, ItemTypeId> m_wearable_layer;
	DataVector<uint32_t, ItemTypeId> m_wearable_bodyTypeScale;
	DataVector<uint32_t, ItemTypeId> m_wearable_forceAbsorbedUnpiercedModifier;
	DataVector<uint32_t, ItemTypeId> m_wearable_forceAbsorbedPiercedModifier;
	DataVector<Percent, ItemTypeId> m_wearable_percentCoverage;
	DataVector<bool, ItemTypeId> m_wearable_rigid;
public:
	static void create(ItemTypeParamaters& p);
	static AttackTypeId getRangedAttackType(ItemTypeId id);
	static BlockIndex getCraftLocation(ItemTypeId id, Blocks& blocks, BlockIndex location, Facing facing);
	static bool hasRangedAttack(ItemTypeId id);
	static bool hasMeleeAttack(ItemTypeId id);
	static const ItemTypeId byName(std::string name);
	[[nodiscard]] static std::vector<MaterialCategoryTypeId>& getMaterialTypeCategories(ItemTypeId id) { return data.m_materialTypeCategories.at(id); }
	[[nodiscard]] static std::string& getName(ItemTypeId id) { return data.m_name.at(id); }
	[[nodiscard]] static std::array<int32_t, 3>& getCraftLocationOffset(ItemTypeId id) { return data.m_craftLocationOffset.at(id); }
	[[nodiscard]] static const Shape* getShape(ItemTypeId id) { return data.m_shape.at(id); }
	[[nodiscard]] static MoveTypeId getMoveType(ItemTypeId id) { return data.m_moveType.at(id); }
	[[nodiscard]] static FluidTypeId getEdibleForDrinkersOf(ItemTypeId id) { return data.m_edibleForDrinkersOf.at(id); }
	[[nodiscard]] static CraftStepTypeCategoryId getCraftLocationStepTypeCategory(ItemTypeId id) { return data.m_craftLocationStepTypeCategory.at(id); }
	[[nodiscard]] static Volume getVolume(ItemTypeId id) { return data.m_volume.at(id); }
	[[nodiscard]] static Volume getInternalVolume(ItemTypeId id) { return data.m_internalVolume.at(id); }
	[[nodiscard]] static uint32_t getValue(ItemTypeId id) { return data.m_value.at(id); }
	[[nodiscard]] static bool getInstallable(ItemTypeId id) { return data.m_installable.at(id); }
	[[nodiscard]] static bool getGeneric(ItemTypeId id) { return data.m_generic.at(id); }
	[[nodiscard]] static bool getCanHoldFluids(ItemTypeId id) { return data.m_canHoldFluids.at(id); }
	[[nodiscard]] static std::vector<AttackTypeId> getattackTypes(ItemTypeId id) { return data.m_attackTypes.at(id); }
	[[nodiscard]] static SkillTypeId getcombatSkill(ItemTypeId id) { return data.m_combatSkill.at(id); }
	[[nodiscard]] static Step getattackCoolDownBase(ItemTypeId id) { return data.m_attackCoolDownBase.at(id); }
	[[nodiscard]] static uint32_t getWearable_defenseScore(ItemTypeId id) { return data.m_wearable_defenseScore.at(id); }
	[[nodiscard]] static uint32_t getWearable_layer(ItemTypeId id) { return data.m_wearable_layer.at(id); }
	[[nodiscard]] static uint32_t getWearable_bodyTypeScale(ItemTypeId id) { return data.m_wearable_bodyTypeScale.at(id); }
	[[nodiscard]] static uint32_t getWearable_forceAbsorbedUnpiercedModifier(ItemTypeId id) { return data.m_wearable_forceAbsorbedUnpiercedModifier.at(id); }
	[[nodiscard]] static uint32_t getWearable_forceAbsorbedPiercedModifier(ItemTypeId id) { return data.m_wearable_forceAbsorbedPiercedModifier.at(id); }
	[[nodiscard]] static Percent getWearable_percentCoverage(ItemTypeId id) { return data.m_wearable_percentCoverage.at(id); }
	[[nodiscard]] static bool getWearable_rigid(ItemTypeId id) { return data.m_wearable_rigid.at(id); }
};
