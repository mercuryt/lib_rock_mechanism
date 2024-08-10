#include "itemType.h"
#include "area.h"
#include "attackType.h"
#include "craft.h"
#include "deserializationMemo.h"
#include "fluidType.h"
#include "materialType.h"
#include "simulation.h"
#include "types.h"
#include "util.h"

#include <iostream>
#include <numbers>
AttackTypeId ItemType::getRangedAttackType(ItemTypeId id)
{
	for(AttackTypeId attackType : data.m_attackTypes[id])
		if(AttackType::getProjectileItemType(attackType).exists())
			return attackType;
	return AttackTypeId::null();
}
bool ItemType::hasRangedAttack(ItemTypeId id) { return getRangedAttackType(id).exists(); }
bool ItemType::hasMeleeAttack(ItemTypeId id)
{
	for(AttackTypeId attackType : data.m_attackTypes[id])
		if(AttackType::getProjectileItemType(attackType).empty())
			return true;
	return false;
}
BlockIndex ItemType::getCraftLocation(ItemTypeId id, Blocks& blocks, BlockIndex location, Facing facing)
{
	assert(data.m_craftLocationStepTypeCategory[id] != CraftStepTypeCategoryId::null());
	auto [x, y, z] = util::rotateOffsetToFacing(data.m_craftLocationOffset[id], facing);
	return blocks.offset(location, x, y, z);
}
// Static methods.
const ItemTypeId ItemType::byName(std::string name)
{
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return ItemTypeId::create(found - data.m_name.begin());
}
void ItemType::create(ItemTypeParamaters& p)
{
	data.m_materialTypeCategories.add(p.materialTypeCategories);
	data.m_name.add(p.name);
	data.m_craftLocationOffset.add(p.craftLocationOffset);
	data.m_shape.add(p.shape);
	data.m_moveType.add(p.moveType);
	data.m_edibleForDrinkersOf.add(p.edibleForDrinkersOf);
	data.m_craftLocationStepTypeCategory.add(p.craftLocationStepTypeCategory);
	data.m_volume.add(p.volume);
	data.m_internalVolume.add(p.internalVolume);
	data.m_value.add(p.value);
	data.m_installable.add(p.installable);
	data.m_generic.add(p.generic);
	data.m_canHoldFluids.add(p.canHoldFluids);
	data.m_attackTypes.add(p.attackTypes);
	data.m_combatSkill.add(p.combatSkill);
	data.m_attackCoolDownBase.add(p.attackCoolDownBase);
	data.m_wearable_defenseScore.add(p.wearable_defenseScore);
	data.m_wearable_layer.add(p.wearable_layer);
	data.m_wearable_bodyTypeScale.add(p.wearable_bodyTypeScale);
	data.m_wearable_forceAbsorbedUnpiercedModifier.add(p.wearable_forceAbsorbedUnpiercedModifier);
	data.m_wearable_forceAbsorbedPiercedModifier.add(p.wearable_forceAbsorbedPiercedModifier);
	data.m_wearable_percentCoverage.add(p.wearable_percentCoverage);
	data.m_wearable_rigid.add(p.wearable_rigid);
}
