#include "itemType.h"
#include "area.h"
#include "craft.h"
#include "deserializationMemo.h"
#include "fluidType.h"
#include "materialType.h"
#include "simulation.h"
#include "types.h"
#include "util.h"

#include <iostream>
#include <numbers>
ItemType::ItemType(std::string n, const Shape& s, const MoveType& mt, Volume vol, Volume ivol, uint32_t val, bool i, bool g, bool chf) : 
	name(n), shape(s), moveType(mt), volume(vol), internalVolume(ivol), value(val), installable(i), generic(g), canHoldFluids(chf) { }
AttackType* ItemType::getRangedAttackType() const
{
	for(const AttackType& attackType : weaponData->attackTypes)
		if(attackType.projectileItemType != nullptr)
			return const_cast<AttackType*>(&attackType);
	return nullptr;
}
bool ItemType::hasRangedAttack() const { return getRangedAttackType() != nullptr; }
bool ItemType::hasMeleeAttack() const
{
	for(const AttackType& attackType : weaponData->attackTypes)
		if(attackType.projectileItemType == nullptr)
			return true;
	return false;
}
BlockIndex ItemType::getCraftLocation(Blocks& blocks, const BlockIndex location, Facing facing) const
{
	assert(craftLocationStepTypeCategory);
	auto [x, y, z] = util::rotateOffsetToFacing(craftLocationOffset, facing);
	return blocks.offset(location, x, y, z);
}
// Static methods.
const ItemType& ItemType::byName(std::string name)
{
	auto found = std::ranges::find(itemTypeDataStore, name, &ItemType::name);
	assert(found != itemTypeDataStore.end());
	return *found;
}
ItemType& ItemType::byNameNonConst(std::string name)
{
	return const_cast<ItemType&>(byName(name));
}
