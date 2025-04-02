#include "itemType.h"
#include "area/area.h"
#include "attackType.h"
#include "craft.h"
#include "deserializationMemo.h"
#include "fluidType.h"
#include "materialType.h"
#include "simulation/simulation.h"
#include "types.h"
#include "util.h"

#include <iostream>
#include <numbers>
ItemTypeId ItemType::size() { return ItemTypeId::create(itemTypeData.m_name.size());}
AttackTypeId ItemType::getRangedAttackType(const ItemTypeId& id)
{
	for(AttackTypeId attackType : itemTypeData.m_attackTypes[id])
		if(AttackType::getProjectileItemType(attackType).exists())
			return attackType;
	return AttackTypeId::null();
}
bool ItemType::hasRangedAttack(const ItemTypeId& id) { return getRangedAttackType(id).exists(); }
bool ItemType::hasMeleeAttack(const ItemTypeId& id)
{
	for(AttackTypeId attackType : itemTypeData.m_attackTypes[id])
		if(AttackType::getProjectileItemType(attackType).empty())
			return true;
	return false;
}
BlockIndex ItemType::getCraftLocation(const ItemTypeId& id, Blocks& blocks, const BlockIndex& location, const Facing4& facing)
{
	assert(itemTypeData.m_craftLocationStepTypeCategory[id] != CraftStepTypeCategoryId::null());
	auto [x, y, z] = util::rotateOffsetToFacing(itemTypeData.m_craftLocationOffset[id], facing);
	return blocks.offset(location, x, y, z);
}
// Static methods.
const ItemTypeId ItemType::byName(std::string name)
{
	auto found = itemTypeData.m_name.find(name);
	assert(found != itemTypeData.m_name.end());
	return ItemTypeId::create(found - itemTypeData.m_name.begin());
}
void ItemType::create(ItemTypeParamaters& p)
{
	itemTypeData.m_materialTypeCategories.add(p.materialTypeCategories);
	itemTypeData.m_name.add(p.name);
	itemTypeData.m_craftLocationOffset.add(p.craftLocationOffset);
	itemTypeData.m_shape.add(p.shape);
	itemTypeData.m_moveType.add(p.moveType);
	itemTypeData.m_edibleForDrinkersOf.add(p.edibleForDrinkersOf);
	itemTypeData.m_craftLocationStepTypeCategory.add(p.craftLocationStepTypeCategory);
	itemTypeData.m_fullDisplacement.add(p.volume);
	itemTypeData.m_internalVolume.add(p.internalVolume);
	itemTypeData.m_value.add(p.value);
	itemTypeData.m_installable.add(p.installable);
	itemTypeData.m_generic.add(p.generic);
	itemTypeData.m_canHoldFluids.add(p.canHoldFluids);
	itemTypeData.m_attackTypes.add(p.attackTypes);
	itemTypeData.m_combatSkill.add(p.combatSkill);
	itemTypeData.m_attackCoolDownBase.add(p.attackCoolDownBase);
	itemTypeData.m_wearable_defenseScore.add(p.wearable_defenseScore);
	itemTypeData.m_wearable_layer.add(p.wearable_layer);
	itemTypeData.m_wearable_bodyTypeScale.add(p.wearable_bodyTypeScale);
	itemTypeData.m_wearable_forceAbsorbedUnpiercedModifier.add(p.wearable_forceAbsorbedUnpiercedModifier);
	itemTypeData.m_wearable_forceAbsorbedPiercedModifier.add(p.wearable_forceAbsorbedPiercedModifier);
	itemTypeData.m_wearable_percentCoverage.add(p.wearable_percentCoverage);
	itemTypeData.m_wearable_rigid.add(p.wearable_rigid);
	itemTypeData.m_wearable_bodyPartsCovered.add(p.wearable_bodyPartsCovered);
	itemTypeData.m_decks.add(p.decks);
}
std::vector<MaterialCategoryTypeId>& ItemType::getMaterialTypeCategories(const ItemTypeId& id) { return itemTypeData.m_materialTypeCategories[id]; }
std::string& ItemType::getName(const ItemTypeId& id) { return itemTypeData.m_name[id]; }
std::array<int32_t, 3>& ItemType::getCraftLocationOffset(const ItemTypeId& id) { return itemTypeData.m_craftLocationOffset[id]; }
ShapeId ItemType::getShape(const ItemTypeId& id) { return itemTypeData.m_shape[id]; }
MoveTypeId ItemType::getMoveType(const ItemTypeId& id) { return itemTypeData.m_moveType[id]; }
FluidTypeId ItemType::getEdibleForDrinkersOf(const ItemTypeId& id) { return itemTypeData.m_edibleForDrinkersOf[id]; }
CraftStepTypeCategoryId ItemType::getCraftLocationStepTypeCategory(const ItemTypeId& id) { return itemTypeData.m_craftLocationStepTypeCategory[id]; }
FullDisplacement ItemType::getFullDisplacement(const ItemTypeId& id) { return itemTypeData.m_fullDisplacement[id]; }
FullDisplacement ItemType::getInternalVolume(const ItemTypeId& id) { return itemTypeData.m_internalVolume[id]; }
uint32_t ItemType::getValue(const ItemTypeId& id) { return itemTypeData.m_value[id]; }
bool ItemType::getInstallable(const ItemTypeId& id) { return itemTypeData.m_installable[id]; }
bool ItemType::getGeneric(const ItemTypeId& id) { return itemTypeData.m_generic[id]; }
bool ItemType::getCanHoldFluids(const ItemTypeId& id) { return itemTypeData.m_canHoldFluids[id]; }
std::vector<AttackTypeId> ItemType::getAttackTypes(const ItemTypeId& id) { return itemTypeData.m_attackTypes[id]; }
SkillTypeId ItemType::getCombatSkill(const ItemTypeId& id) { return itemTypeData.m_combatSkill[id]; }
Step ItemType::getAttackCoolDownBase(const ItemTypeId& id) { return itemTypeData.m_attackCoolDownBase[id]; }
bool ItemType::getIsWeapon(const ItemTypeId& id) { return !itemTypeData.m_attackTypes[id].empty(); }
bool ItemType::getIsGeneric(const ItemTypeId& id) { return itemTypeData.m_generic[id]; }
uint32_t ItemType::getWearable_defenseScore(const ItemTypeId& id) { return itemTypeData.m_wearable_defenseScore[id]; }
uint32_t ItemType::getWearable_layer(const ItemTypeId& id) { return itemTypeData.m_wearable_layer[id]; }
uint32_t ItemType::getWearable_bodyTypeScale(const ItemTypeId& id) { return itemTypeData.m_wearable_bodyTypeScale[id]; }
uint32_t ItemType::getWearable_forceAbsorbedUnpiercedModifier(const ItemTypeId& id) { return itemTypeData.m_wearable_forceAbsorbedUnpiercedModifier[id]; }
uint32_t ItemType::getWearable_forceAbsorbedPiercedModifier(const ItemTypeId& id) { return itemTypeData.m_wearable_forceAbsorbedPiercedModifier[id]; }
Percent ItemType::getWearable_percentCoverage(const ItemTypeId& id) { return itemTypeData.m_wearable_percentCoverage[id]; }
bool ItemType::getWearable_rigid(const ItemTypeId& id) { return itemTypeData.m_wearable_rigid[id]; }
std::vector<BodyPartTypeId>& ItemType::getWearable_bodyPartsCovered(const ItemTypeId& id) { return itemTypeData.m_wearable_bodyPartsCovered[id]; }
bool ItemType::getIsWearable(const ItemTypeId& id) { return !itemTypeData.m_wearable_bodyPartsCovered[id].empty(); }
const std::vector<std::pair<Offset3D, Offset3D>>& ItemType::getDecks(const ItemTypeId& id) { return itemTypeData.m_decks[id]; }
