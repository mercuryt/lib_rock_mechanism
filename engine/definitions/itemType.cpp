#include "definitions/itemType.h"
#include "area/area.h"
#include "space/space.h"
#include "definitions/attackType.h"
#include "craft.h"
#include "deserializationMemo.h"
#include "fluidType.h"
#include "definitions/materialType.h"
#include "simulation/simulation.h"
#include "numericTypes/types.h"
#include "util.h"

#include <iostream>
#include <numbers>
ItemTypeId ItemType::size() { return ItemTypeId::create(g_itemTypeData.m_name.size());}
AttackTypeId ItemType::getRangedAttackType(const ItemTypeId& id)
{
	for(AttackTypeId attackType : g_itemTypeData.m_attackTypes[id])
		if(AttackType::getProjectileItemType(attackType).exists())
			return attackType;
	return AttackTypeId::null();
}
bool ItemType::hasRangedAttack(const ItemTypeId& id) { return getRangedAttackType(id).exists(); }
bool ItemType::hasMeleeAttack(const ItemTypeId& id)
{
	for(AttackTypeId attackType : g_itemTypeData.m_attackTypes[id])
		if(AttackType::getProjectileItemType(attackType).empty())
			return true;
	return false;
}
Point3D ItemType::getCraftLocation(const ItemTypeId& id, const Point3D& location, const Facing4& facing)
{
	assert(g_itemTypeData.m_craftLocationStepTypeCategory[id] != CraftStepTypeCategoryId::null());
	auto [x, y, z] = util::rotateOffsetToFacing(g_itemTypeData.m_craftLocationOffset[id], facing);
	const Offset3D offsetLocation = location.applyOffset({x, y, z});
	return Point3D::create(offsetLocation);
}
// Static methods.
const ItemTypeId ItemType::byName(std::string name)
{
	auto found = g_itemTypeData.m_name.find(name);
	assert(found != g_itemTypeData.m_name.end());
	return ItemTypeId::create(found - g_itemTypeData.m_name.begin());
}
const std::unique_ptr<ConstructedShape>& ItemType::getConstructedShape(const ItemTypeId& id) { return g_itemTypeData.m_constructedShape[id]; }
void ItemType::create(const ItemTypeId& id, const ItemTypeParamaters& p)
{
	if(id >= g_itemTypeData.size())
	{
		auto action = [&](auto& vector){ vector.resize(id); };
		g_itemTypeData.forEachVector(action);
	}
	if(p.constructedShape != nullptr)
		g_itemTypeData.m_constructedShape[id] = std::make_unique<ConstructedShape>(*p.constructedShape);
	g_itemTypeData.m_solidCategories[id] = p.materialTypeCategories;
	g_itemTypeData.m_name[id] = p.name;
	g_itemTypeData.m_craftLocationOffset[id] = p.craftLocationOffset;
	g_itemTypeData.m_shape[id] = p.shape;
	g_itemTypeData.m_moveType[id] = p.moveType;
	g_itemTypeData.m_edibleForDrinkersOf[id] = p.edibleForDrinkersOf;
	g_itemTypeData.m_craftLocationStepTypeCategory[id] = p.craftLocationStepTypeCategory;
	g_itemTypeData.m_fullDisplacement[id] = p.volume;
	g_itemTypeData.m_internalVolume[id] = p.internalVolume;
	g_itemTypeData.m_value[id] = p.value;
	g_itemTypeData.m_installable.set(id, p.installable);
	g_itemTypeData.m_generic.set(id, p.generic);
	g_itemTypeData.m_canHoldFluids.set(id, p.canHoldFluids);
	g_itemTypeData.m_attackTypes[id] = p.attackTypes;
	g_itemTypeData.m_combatSkill[id] = p.combatSkill;
	g_itemTypeData.m_attackCoolDownBase[id] = p.attackCoolDownBase;
	g_itemTypeData.m_wearable_defenseScore[id] = p.wearable_defenseScore;
	g_itemTypeData.m_wearable_layer[id] = p.wearable_layer;
	g_itemTypeData.m_wearable_bodyTypeScale[id] = p.wearable_bodyTypeScale;
	g_itemTypeData.m_wearable_forceAbsorbedUnpiercedModifier[id] = p.wearable_forceAbsorbedUnpiercedModifier;
	g_itemTypeData.m_wearable_forceAbsorbedPiercedModifier[id] = p.wearable_forceAbsorbedPiercedModifier;
	g_itemTypeData.m_wearable_percentCoverage[id] = p.wearable_percentCoverage;
	g_itemTypeData.m_wearable_rigid.set(id, p.wearable_rigid);
	g_itemTypeData.m_wearable_bodyPartsCovered[id] = p.wearable_bodyPartsCovered;
	g_itemTypeData.m_decks[id] = p.decks;
	g_itemTypeData.m_motiveForce[id] = p.motiveForce;
}
ItemTypeId ItemType::create(const ItemTypeParamaters& p)
{
	const ConstructedShape* constructedShape = p.constructedShape.get();
	if(constructedShape == nullptr)
		g_itemTypeData.m_constructedShape.resize(g_itemTypeData.m_name.size() + 1);
	else
		g_itemTypeData.m_constructedShape.add(std::make_unique<ConstructedShape>(*constructedShape));
	g_itemTypeData.m_solidCategories.add(p.materialTypeCategories);
	g_itemTypeData.m_name.add(p.name);
	g_itemTypeData.m_craftLocationOffset.add(p.craftLocationOffset);
	g_itemTypeData.m_shape.add(p.shape);
	g_itemTypeData.m_moveType.add(p.moveType);
	g_itemTypeData.m_edibleForDrinkersOf.add(p.edibleForDrinkersOf);
	g_itemTypeData.m_craftLocationStepTypeCategory.add(p.craftLocationStepTypeCategory);
	g_itemTypeData.m_fullDisplacement.add(p.volume);
	g_itemTypeData.m_internalVolume.add(p.internalVolume);
	g_itemTypeData.m_value.add(p.value);
	g_itemTypeData.m_installable.add(p.installable);
	g_itemTypeData.m_generic.add(p.generic);
	g_itemTypeData.m_canHoldFluids.add(p.canHoldFluids);
	g_itemTypeData.m_attackTypes.add(p.attackTypes);
	g_itemTypeData.m_combatSkill.add(p.combatSkill);
	g_itemTypeData.m_attackCoolDownBase.add(p.attackCoolDownBase);
	g_itemTypeData.m_wearable_defenseScore.add(p.wearable_defenseScore);
	g_itemTypeData.m_wearable_layer.add(p.wearable_layer);
	g_itemTypeData.m_wearable_bodyTypeScale.add(p.wearable_bodyTypeScale);
	g_itemTypeData.m_wearable_forceAbsorbedUnpiercedModifier.add(p.wearable_forceAbsorbedUnpiercedModifier);
	g_itemTypeData.m_wearable_forceAbsorbedPiercedModifier.add(p.wearable_forceAbsorbedPiercedModifier);
	g_itemTypeData.m_wearable_percentCoverage.add(p.wearable_percentCoverage);
	g_itemTypeData.m_wearable_rigid.add(p.wearable_rigid);
	g_itemTypeData.m_wearable_bodyPartsCovered.add(p.wearable_bodyPartsCovered);
	g_itemTypeData.m_decks.add(p.decks);
	g_itemTypeData.m_motiveForce.add(p.motiveForce);
	return ItemTypeId::create(g_itemTypeData.m_solidCategories.size() - 1);
}
std::vector<MaterialCategoryTypeId>& ItemType::getMaterialTypeCategories(const ItemTypeId& id) { return g_itemTypeData.m_solidCategories[id]; }
std::string& ItemType::getName(const ItemTypeId& id) { return g_itemTypeData.m_name[id]; }
std::array<int32_t, 3>& ItemType::getCraftLocationOffset(const ItemTypeId& id) { return g_itemTypeData.m_craftLocationOffset[id]; }
ShapeId ItemType::getShape(const ItemTypeId& id) { return g_itemTypeData.m_shape[id]; }
MoveTypeId ItemType::getMoveType(const ItemTypeId& id) { return g_itemTypeData.m_moveType[id]; }
FluidTypeId ItemType::getEdibleForDrinkersOf(const ItemTypeId& id) { return g_itemTypeData.m_edibleForDrinkersOf[id]; }
CraftStepTypeCategoryId ItemType::getCraftLocationStepTypeCategory(const ItemTypeId& id) { return g_itemTypeData.m_craftLocationStepTypeCategory[id]; }
FullDisplacement ItemType::getFullDisplacement(const ItemTypeId& id) { return g_itemTypeData.m_fullDisplacement[id]; }
FullDisplacement ItemType::getInternalVolume(const ItemTypeId& id) { return g_itemTypeData.m_internalVolume[id]; }
int32_t ItemType::getValue(const ItemTypeId& id) { return g_itemTypeData.m_value[id]; }
bool ItemType::getInstallable(const ItemTypeId& id) { return g_itemTypeData.m_installable[id]; }
bool ItemType::getGeneric(const ItemTypeId& id) { return g_itemTypeData.m_generic[id]; }
bool ItemType::getCanHoldFluids(const ItemTypeId& id) { return g_itemTypeData.m_canHoldFluids[id]; }
std::vector<AttackTypeId> ItemType::getAttackTypes(const ItemTypeId& id) { return g_itemTypeData.m_attackTypes[id]; }
SkillTypeId ItemType::getCombatSkill(const ItemTypeId& id) { return g_itemTypeData.m_combatSkill[id]; }
Step ItemType::getAttackCoolDownBase(const ItemTypeId& id) { return g_itemTypeData.m_attackCoolDownBase[id]; }
bool ItemType::getIsWeapon(const ItemTypeId& id) { return !g_itemTypeData.m_attackTypes[id].empty(); }
bool ItemType::getIsGeneric(const ItemTypeId& id) { return g_itemTypeData.m_generic[id]; }
int32_t ItemType::getWearable_defenseScore(const ItemTypeId& id) { return g_itemTypeData.m_wearable_defenseScore[id]; }
int32_t ItemType::getWearable_layer(const ItemTypeId& id) { return g_itemTypeData.m_wearable_layer[id]; }
int32_t ItemType::getWearable_bodyTypeScale(const ItemTypeId& id) { return g_itemTypeData.m_wearable_bodyTypeScale[id]; }
int32_t ItemType::getWearable_forceAbsorbedUnpiercedModifier(const ItemTypeId& id) { return g_itemTypeData.m_wearable_forceAbsorbedUnpiercedModifier[id]; }
int32_t ItemType::getWearable_forceAbsorbedPiercedModifier(const ItemTypeId& id) { return g_itemTypeData.m_wearable_forceAbsorbedPiercedModifier[id]; }
Percent ItemType::getWearable_percentCoverage(const ItemTypeId& id) { return g_itemTypeData.m_wearable_percentCoverage[id]; }
bool ItemType::getWearable_rigid(const ItemTypeId& id) { return g_itemTypeData.m_wearable_rigid[id]; }
std::vector<BodyPartTypeId>& ItemType::getWearable_bodyPartsCovered(const ItemTypeId& id) { return g_itemTypeData.m_wearable_bodyPartsCovered[id]; }
bool ItemType::getIsWearable(const ItemTypeId& id) { return !g_itemTypeData.m_wearable_bodyPartsCovered[id].empty(); }
const OffsetCuboidSet& ItemType::getDecks(const ItemTypeId& id) { return g_itemTypeData.m_decks[id]; }
const Force ItemType::getMotiveForce(const ItemTypeId& id) { return g_itemTypeData.m_motiveForce[id]; }
