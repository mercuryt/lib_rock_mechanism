#include "blocks.h"
#include "../item.h"
#include "../actor.h"
#include "../area.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "../types.h"
void Blocks::item_add(BlockIndex index, Item& item)
{
	//assert(m_block.m_hasShapes.canEnterEverWithAnyFacing(item));
	auto& items = m_items.at(index);
	assert(std::ranges::find(items, &item) == items.end());
	if(item.m_itemType.generic)
	{
		auto found = std::ranges::find_if(items, [&](Item* otherItem) { return otherItem->m_itemType == item.m_itemType && otherItem->m_materialType == item.m_materialType; });
		// Add to.
		if(found != items.end())
		{
			(*found)->merge(item);
			return;
		}
	}
	items.push_back(&item);
	shape_enter(index, item);
	//TODO: optimize by storing underground status in item or shape to prevent repeted set insertions / removals.
	if(m_underground[index])
		m_area.m_hasItems.setItemIsNotOnSurface(item);
	else
		m_area.m_hasItems.setItemIsOnSurface(item);
}
void Blocks::item_remove(BlockIndex index, Item& item)
{
	auto& items = m_items.at(index);
	assert(std::ranges::find(items, &item) != items.end());
	std::erase(items, &item);
	shape_exit(index, item);
}
Item& Blocks::item_addGeneric(BlockIndex index, const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
{
	assert(itemType.generic);
	auto& items = m_items.at(index);
	auto found = std::ranges::find_if(items, [&](Item* item) { return item->m_itemType == itemType && item->m_materialType == materialType; });
	// Add to.
	if(found != items.end())
	{
		shape_addQuantity(index, **found, quantity);
		return **found;
	}
	// Create.
	Item& item = m_area.m_simulation.m_hasItems->createItem({
		.itemType=itemType,
		.materialType=materialType,
		.quantity=quantity,
		.location=index
	});
	return item;
}
void Blocks::item_remove(BlockIndex index, const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
{
	assert(itemType.generic);
	auto& items = m_items.at(index);
	auto found = std::ranges::find_if(items, [&](Item* item) { return item->m_itemType == itemType && item->m_materialType == materialType; });
	assert(found != items.end());
	assert((*found)->getQuantity() >= quantity);
	// Remove all.
	if((*found)->getQuantity() == quantity)
	{
		item_remove(index, **found);
		// TODO: don't remove if it's about to be readded, requires knowing destination / origin.
		if(m_outdoors[index])
			m_area.m_hasItems.setItemIsNotOnSurface(**found);
	}
	else
	{
		// Remove some.
		(*found)->removeQuantity(quantity);
		(*found)->m_reservable.setMaxReservations((*found)->getQuantity());
		shape_removeQuantity(index, **found, quantity);
	}
}
void Blocks::item_setTemperature(BlockIndex index, Temperature temperature)
{
	for(Item* item : m_items.at(index))
		item->setTemperature(temperature);
}
void Blocks::item_disperseAll(BlockIndex index)
{
	auto& items = m_items.at(index);
	if(items.empty())
		return;
	std::vector<BlockIndex> blocks;
	for(BlockIndex otherIndex : getAdjacentOnSameZLevelOnly(index))
		if(!solid_is(otherIndex))
			blocks.push_back(otherIndex);
	auto copy = items;
	for(Item* item : copy)
	{
		//TODO: split up stacks of generics, prefer blocks with more empty space.
		BlockIndex block = m_area.m_simulation.m_random.getInVector(blocks);
		item->setLocation(block);
	}
}
uint32_t Blocks::item_getCount(BlockIndex index, const ItemType& itemType, const MaterialType& materialType) const
{
	auto& items = m_items.at(index);
	auto found = std::ranges::find_if(items, [&](Item* item)
	{
		return item->m_itemType == itemType && item->m_materialType == materialType;
	});
	if(found == items.end())
		return 0;
	else
		return (*found)->getQuantity();
}
Item& Blocks::item_getGeneric(BlockIndex index, const ItemType& itemType, const MaterialType& materialType) const
{
	auto& items = m_items.at(index);
	auto found = std::ranges::find_if(items, [&](Item* item) { return item->m_itemType == itemType && item->m_materialType == materialType; });
	assert(found != items.end());
	return **found;
}
// TODO: buggy
bool Blocks::item_hasInstalledType(BlockIndex index, const ItemType& itemType) const
{
	auto& items = m_items.at(index);
	auto found = std::ranges::find_if(items, [&](Item* item) { return item->m_itemType == itemType; });
	return found != items.end() && (*found)->m_installed;
}
bool Blocks::item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(BlockIndex index, const Actor& actor) const
{
	for(const Item* item : m_items.at(index))
		//TODO: account for container weight when full, needs to have fluid type passed in.
		if(item->m_itemType.internalVolume != 0 && item->m_itemType.canHoldFluids && actor.m_canPickup.canPickupAny(*item))
			return true;
	return false;
}
bool Blocks::item_hasContainerContainingFluidTypeCarryableBy(BlockIndex index, const Actor& actor, const FluidType& fluidType) const
{
	for(const Item* item : m_items.at(index))
		if(item->m_itemType.internalVolume != 0 && item->m_itemType.canHoldFluids && actor.m_canPickup.canPickupAny(*item) && item->m_hasCargo.getFluidType() == fluidType)
			return true;
	return false;
}
bool Blocks::item_empty(BlockIndex index) const
{
	return m_items.at(index).empty();
}
std::vector<Item*>& Blocks::item_getAll(BlockIndex index)
{
	return m_items[index];
}
const std::vector<Item*>& Blocks::item_getAll(BlockIndex index) const
{
	return m_items[index];
}
