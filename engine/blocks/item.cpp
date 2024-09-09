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
	m_itemVolume[index].emplace_back(item, volume);
	Items& items = m_area.getItems();
	m_items[index].add(item);
	if(items.isStatic(item))
		m_staticVolume[index] += volume;
	else
		m_dynamicVolume[index] += volume;
}
void Blocks::item_erase(BlockIndex index, ItemIndex item)
{
	Items& items = m_area.getItems();
	auto& blockItems = m_items[index];
	auto& blockItemVolume = m_itemVolume[index];
	auto iter = std::ranges::find(blockItems, item);
	auto iter2 = m_itemVolume[index].begin() + (std::distance(blockItems.begin(), iter));
	if(items.isStatic(item))
		m_staticVolume[index] -= iter2->second;
	else
		m_dynamicVolume[index] -= iter2->second;
	blockItems.remove(iter);
	(*iter2) = blockItemVolume.back();
	blockItemVolume.pop_back();
}
void Blocks::item_setTemperature(BlockIndex index, Temperature temperature)
{
	Items& items = m_area.getItems();
	for(auto [item, volume] : m_itemVolume[index])
		items.setTemperature(item, temperature);
}
void Blocks::item_disperseAll(BlockIndex index)
{
	auto& itemsInBlock = m_itemVolume[index];
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
void Blocks::item_updateIndex(BlockIndex index, ItemIndex oldIndex, ItemIndex newIndex)
{
	auto found = std::ranges::find(m_items[index], oldIndex);
	assert(found != m_items[index].end());
	(*found) = newIndex; 
}
ItemIndex Blocks::item_addGeneric(BlockIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity)
{
	Items& items = m_area.getItems();
	CollisionVolume volume = Shape::getCollisionVolumeAtLocationBlock(ItemType::getShape(itemType)) * quantity;
	// Assume that generics added through this method are static.
	// Generics in transit ( being hauled by multiple workers ) should not use this method and should use setLocationAndFacing instead.
	m_staticVolume[index] += volume;
	auto& blockItems = m_items[index];
	auto found = blockItems.find_if([itemType, &items](const auto& item) { return items.getItemType(item) == itemType; });
	if(found == blockItems.end())
	{
		ItemIndex item = items.create(ItemParamaters{
			.itemType=itemType,
			.materialType=materialType,
			.quantity=quantity,
		});
		blockItems.add(item);
		return item;
	}
	else
	{
		items.addQuantity(*found, quantity);
		return *found;
	}
}
Quantity Blocks::item_getCount(BlockIndex index, ItemTypeId itemType, MaterialTypeId materialType) const
{
	auto& itemsInBlock = m_itemVolume[index];
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
	auto& itemsInBlock = m_itemVolume[index];
	Items& items = m_area.getItems();
	auto found = std::ranges::find_if(itemsInBlock, [&](auto pair) { 
		ItemIndex item = pair.first;
		return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType;
	});
	if(found == itemsInBlock.end())
		return ItemIndex::null();
	return found->first;
}
// TODO: buggy
bool Blocks::item_hasInstalledType(BlockIndex index, ItemTypeId itemType) const
{
	auto& itemsInBlock = m_itemVolume[index];
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
	for(auto [item, volume] : m_itemVolume[index])
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
	for(auto [item, volume]  : m_itemVolume[index])
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
	return m_itemVolume[index].empty();
}
bool Blocks::item_contains(BlockIndex index, ItemIndex item) const
{
	return m_items[index].contains(item);
}
ItemIndicesForBlock& Blocks::item_getAll(BlockIndex index)
{
	return m_items[index];
}
const ItemIndicesForBlock& Blocks::item_getAll(BlockIndex index) const
{
	return const_cast<Blocks*>(this)->item_getAll(index);
}
