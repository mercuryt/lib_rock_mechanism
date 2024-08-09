#include "blocks.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../area.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "../types.h"
#include "../itemType.h"
#include <iterator>
void Blocks::item_record(BlockIndex index, ItemIndex item, CollisionVolume volume)
{
	m_itemVolume.at(index).emplace_back(item, volume);
	Items& items = m_area.getItems();
	m_items.at(index).add(item);
	if(items.isStatic(item))
		m_staticVolume.at(index) += volume;
	else
		m_dynamicVolume.at(index) += volume;
}
void Blocks::item_erase(BlockIndex index, ItemIndex item)
{
	Items& items = m_area.getItems();
	auto& blockItems = m_items.at(index);
	auto& blockItemVolume = m_itemVolume.at(index);
	auto iter = std::ranges::find(blockItems, item);
	auto iter2 = m_itemVolume.at(index).begin() + (std::distance(iter, blockItems.begin()));
	if(items.isStatic(item))
		m_staticVolume.at(index) -= iter2->second;
	else
		m_dynamicVolume.at(index) -= iter2->second;
	blockItems.remove(iter);
	(*iter2) = blockItemVolume.back();
	blockItemVolume.pop_back();
}
void Blocks::item_setTemperature(BlockIndex index, Temperature temperature)
{
	Items& items = m_area.getItems();
	for(auto [item, volume] : m_itemVolume.at(index))
		items.setTemperature(item, temperature);
}
void Blocks::item_disperseAll(BlockIndex index)
{
	auto& itemsInBlock = m_itemVolume.at(index);
	if(itemsInBlock.empty())
		return;
	BlockIndices blocks;
	for(BlockIndex otherIndex : getAdjacentOnSameZLevelOnly(index))
		if(!solid_is(otherIndex))
			blocks.add(otherIndex);
	auto copy = itemsInBlock;
	Items& items = m_area.getItems();
	for(auto [item, volume] : copy)
	{
		//TODO: split up stacks of generics, prefer blocks with more empty space.
		BlockIndex block = blocks.random(m_area.m_simulation);
		items.setLocation(item, block);
	}
}
bool Blocks::item_canEnterCurrentlyFrom(BlockIndex index, ItemIndex item, BlockIndex block) const
{
	Facing facing = facingToSetWhenEnteringFrom(index, block);
	return item_canEnterCurrentlyWithFacing(index, item, facing);
}
bool Blocks::item_canEnterCurrentlyWithFacing(BlockIndex index, ItemIndex item, Facing facing) const
{
	Items& items = m_area.getItems();
	ShapeId shape = items.getShape(item);
	// For multi block shapes assume that volume is the same for each block.
	CollisionVolume volume = Shape::getCollisionVolumeAtLocationBlock(shape);
	for(BlockIndex block : Shape::getBlocksOccupiedAt(shape, *this, index, facing))
		if(!item_contains(block, item) && (/*m_items.at(block).full() || */m_dynamicVolume.at(block) + volume > Config::maxBlockVolume))
				return false;
	return true;
}
bool Blocks::item_canEnterEverOrCurrentlyWithFacing(BlockIndex index, ItemIndex item, Facing facing) const
{
	assert(shape_anythingCanEnterEver(index));
	const Items& items = m_area.getItems();
	MoveTypeId moveType = items.getMoveType(item);
	for(auto& [x, y, z, v] : Shape::positionsWithFacing(items.getShape(item), facing))
	{
		BlockIndex block = offset(index,x, y, z);
		if(m_items.at(block).contains(item))
			continue;
		if(block.empty() || !shape_anythingCanEnterEver(block) ||
			m_dynamicVolume.at(block) + v > Config::maxBlockVolume || 
			!shape_moveTypeCanEnter(block, moveType)
		)
			return false;
	}
	return true;
}
bool Blocks::item_canEnterCurrentlyWithAnyFacing(BlockIndex index, ItemIndex item) const
{
	for(Facing i = Facing::create(0); i < 4; ++i)
		if(item_canEnterCurrentlyWithFacing(index, item, i))
			return true;
	return false;
}
void Blocks::item_updateIndex(BlockIndex index, ItemIndex oldIndex, ItemIndex newIndex)
{
	auto found = std::ranges::find(m_items.at(index), oldIndex);
	assert(found != m_items.at(index).end());
	(*found) = newIndex; 
}
Quantity Blocks::item_getCount(BlockIndex index, ItemTypeId itemType, MaterialTypeId materialType) const
{
	auto& itemsInBlock = m_itemVolume.at(index);
	Items& items = m_area.getItems();
	auto found = std::ranges::find_if(itemsInBlock, [&](auto pair)
	{
		ItemIndex item = pair.first;
		return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType;
	});
	if(found == itemsInBlock.end())
		return Quantity::create(0);
	else
		return items.getQuantity(found->first);
}
ItemIndex Blocks::item_getGeneric(BlockIndex index, ItemTypeId itemType, MaterialTypeId materialType) const
{
	auto& itemsInBlock = m_itemVolume.at(index);
	Items& items = m_area.getItems();
	auto found = std::ranges::find_if(itemsInBlock, [&](auto pair) { 
		ItemIndex item = pair.first;
		return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType;
	});
	assert(found != itemsInBlock.end());
	return found->first;
}
// TODO: buggy
bool Blocks::item_hasInstalledType(BlockIndex index, ItemTypeId itemType) const
{
	auto& itemsInBlock = m_itemVolume.at(index);
	Items& items = m_area.getItems();
	auto found = std::ranges::find_if(itemsInBlock, [&](auto pair) { 
		ItemIndex item = pair.first;
		return items.getItemType(item) == itemType;
	});
	return found != itemsInBlock.end() && items.isInstalled(found->first);
}
bool Blocks::item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(BlockIndex index, const ActorIndex actor) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	for(auto [item, volume] : m_itemVolume.at(index))
	{
		ItemTypeId itemType = items.getItemType(item);
		//TODO: account for container weight when full, needs to have fluid type passed in.
		if(ItemType::getInternalVolume(itemType) != 0 && ItemType::getCanHoldFluids(itemType) && actors.canPickUp_anyWithMass(actor, items.getMass(item)))
			return true;
	}
	return false;
}
bool Blocks::item_hasContainerContainingFluidTypeCarryableBy(BlockIndex index, const ActorIndex actor, FluidTypeId fluidType) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	for(auto [item, volume]  : m_itemVolume.at(index))
	{
		ItemTypeId itemType = items.getItemType(item);
		if(ItemType::getInternalVolume(itemType) != 0 && ItemType::getCanHoldFluids(itemType) &&
			actors.canPickUp_anyWithMass(actor, items.getMass(item)) &&
			items.cargo_getFluidType(item) == fluidType
		)
			return true;
	}
	return false;
}
bool Blocks::item_empty(BlockIndex index) const
{
	return m_itemVolume.at(index).empty();
}
bool Blocks::item_contains(BlockIndex index, ItemIndex item) const
{
	return m_items.at(index).contains(item);
}
ItemIndicesForBlock& Blocks::item_getAll(BlockIndex index)
{
	return m_items.at(index);
}
const ItemIndicesForBlock& Blocks::item_getAll(BlockIndex index) const
{
	return const_cast<Blocks*>(this)->item_getAll(index);
}
