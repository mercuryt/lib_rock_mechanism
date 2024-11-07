#include "blocks.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../area.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "../types.h"
#include "../itemType.h"
#include <iterator>
void Blocks::item_record(const BlockIndex& index, const ItemIndex& item, const CollisionVolume& volume)
{
	m_itemVolume[index].emplace_back(item, volume);
	Items& items = m_area.getItems();
	m_items[index].add(item);
	if(items.isStatic(item))
		m_staticVolume[index] += volume;
	else
		m_dynamicVolume[index] += volume;
}
void Blocks::item_erase(const BlockIndex& index, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	auto& blockItems = m_items[index];
	auto& blockItemVolume = m_itemVolume[index];
	auto iter = blockItems.find(item);
	uint16_t offset = (iter - blockItems.begin()).getIndex();
	auto iter2 = m_itemVolume[index].begin() + offset;
	if(items.isStatic(item))
		m_staticVolume[index] -= iter2->second;
	else
		m_dynamicVolume[index] -= iter2->second;
	blockItems.remove(iter);
	(*iter2) = blockItemVolume.back();
	blockItemVolume.pop_back();
}
void Blocks::item_setTemperature(const BlockIndex& index, const Temperature& temperature)
{
	Items& items = m_area.getItems();
	for(const ItemIndex& item : m_items[index])
		items.setTemperature(item, temperature);
}
void Blocks::item_disperseAll(const BlockIndex& index)
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
void Blocks::item_updateIndex(const BlockIndex& index, const ItemIndex& oldIndex, const ItemIndex& newIndex)
{
	auto found = m_items[index].find(oldIndex);
	assert(found != m_items[index].end());
	(*found) = newIndex; 
	auto found2 = std::ranges::find(m_itemVolume[index], oldIndex, [](const auto& pair) { return pair.first; });
	assert(found2 != m_itemVolume[index].end());
	(*found2).first = newIndex; 
}
ItemIndex Blocks::item_addGeneric(const BlockIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
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
			.location=index,
			.quantity=quantity,
		});
		return item;
	}
	else
	{
		ItemIndex item = *found;
		assert(item.exists());
		items.addQuantity(item, quantity);
		return item;
	}
}
Quantity Blocks::item_getCount(const BlockIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType) const
{
	auto& itemsInBlock = m_items[index];
	Items& items = m_area.getItems();
	const auto found = itemsInBlock.find_if([&](const ItemIndex& item)
	{
		return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType;
	});
	if(found == itemsInBlock.end())
		return Quantity::create(0);
	else
		return items.getQuantity(*found);
}
ItemIndex Blocks::item_getGeneric(const BlockIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType) const
{
	auto& itemsInBlock = m_items[index];
	Items& items = m_area.getItems();
	const auto found = itemsInBlock.find_if([&](const ItemIndex& item) { 
		return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType;
	});
	if(found == itemsInBlock.end())
		return ItemIndex::null();
	return *found;
}
// TODO: buggy
bool Blocks::item_hasInstalledType(const BlockIndex& index, const ItemTypeId& itemType) const
{
	auto& itemsInBlock = m_itemVolume[index];
	Items& items = m_area.getItems();
	auto found = std::ranges::find_if(itemsInBlock, [&](auto pair) { 
		ItemIndex item = pair.first;
		return items.getItemType(item) == itemType;
	});
	return found != itemsInBlock.end() && items.isInstalled(found->first);
}
bool Blocks::item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(const BlockIndex& index, const ActorIndex& actor) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	for(const ItemIndex& item : m_items[index])
	{
		ItemTypeId itemType = items.getItemType(item);
		//TODO: account for container weight when full, needs to have fluid type passed in.
		if(ItemType::getInternalVolume(itemType) != 0 && ItemType::getCanHoldFluids(itemType) && actors.canPickUp_anyWithMass(actor, items.getMass(item)))
			return true;
	}
	return false;
}
bool Blocks::item_hasContainerContainingFluidTypeCarryableBy(const BlockIndex& index, const ActorIndex& actor, const FluidTypeId& fluidType) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	for(const ItemIndex& item : m_items[index])
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
bool Blocks::item_empty(const BlockIndex& index) const
{
	return m_itemVolume[index].empty();
}
bool Blocks::item_contains(const BlockIndex& index, const ItemIndex& item) const
{
	return m_items[index].contains(item);
}
VerySmallSet<ItemIndex, 3>& Blocks::item_getAll(const BlockIndex& index)
{
	return m_items[index];
}
const VerySmallSet<ItemIndex, 3>& Blocks::item_getAll(const BlockIndex& index) const
{
	return const_cast<Blocks*>(this)->item_getAll(index);
}
