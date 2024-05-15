#include "hasItems.h"
#include "../item.h"
#include "../block.h"
#include "../area.h"
#include "../simulation.h"
BlockHasItems::BlockHasItems(Block& b): m_block(b) { }
void BlockHasItems::add(Item& item)
{
	//assert(m_block.m_hasShapes.canEnterEverWithAnyFacing(item));
	assert(std::ranges::find(m_items, &item) == m_items.end());
	if(item.m_itemType.generic)
	{
		auto found = std::ranges::find_if(m_items, [&](Item* otherItem) { return otherItem->m_itemType == item.m_itemType && otherItem->m_materialType == item.m_materialType; });
		// Add to.
		if(found != m_items.end())
		{
			(*found)->merge(item);
			return;
		}
	}
	m_items.push_back(&item);
	m_block.m_hasShapes.enter(item);
	//TODO: optimize by storing underground status in item or shape to prevent repeted set insertions / removals.
	if(m_block.m_underground)
		m_block.m_area->m_hasItems.setItemIsNotOnSurface(item);
	else
		m_block.m_area->m_hasItems.setItemIsOnSurface(item);
}
void BlockHasItems::remove(Item& item)
{
	assert(std::ranges::find(m_items, &item) != m_items.end());
	std::erase(m_items, &item);
	m_block.m_hasShapes.exit(item);
}
Item& BlockHasItems::addGeneric(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
{
	assert(itemType.generic);
	auto found = std::ranges::find_if(m_items, [&](Item* item) { return item->m_itemType == itemType && item->m_materialType == materialType; });
	// Add to.
	if(found != m_items.end())
	{
		m_block.m_hasShapes.addQuantity(**found, quantity);
		return **found;
	}
	// Create.
	else
		return m_block.m_area->m_simulation.createItem({
			.itemType=itemType,
			.materialType=materialType,
			.quantity=quantity,
			.location=&m_block
		});
}
void BlockHasItems::remove(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
{
	assert(itemType.generic);
	auto found = std::ranges::find_if(m_items, [&](Item* item) { return item->m_itemType == itemType && item->m_materialType == materialType; });
	assert(found != m_items.end());
	assert((*found)->getQuantity() >= quantity);
	// Remove all.
	if((*found)->getQuantity() == quantity)
	{
		remove(**found);
		// TODO: don't remove if it's about to be readded, requires knowing destination / origin.
		if(m_block.m_outdoors)
			m_block.m_area->m_hasItems.setItemIsNotOnSurface(**found);
	}
	else
	{
		// Remove some.
		(*found)->removeQuantity(quantity);
		(*found)->m_reservable.setMaxReservations((*found)->getQuantity());
		m_block.m_hasShapes.removeQuantity(**found, quantity);
	}
}
void BlockHasItems::setTemperature(Temperature temperature)
{
	for(Item* item : m_items)
		item->setTemperature(temperature);
}
void BlockHasItems::disperseAll()
{
	if(m_items.empty())
		return;
	std::vector<Block*> blocks;
	for(Block* block : m_block.getAdjacentOnSameZLevelOnly())
		if(!block->isSolid())
			blocks.push_back(block);
	auto copy = m_items;
	for(Item* item : copy)
	{
		//TODO: split up stacks of generics, prefer blocks with more empty space.
		Block* block = m_block.m_area->m_simulation.m_random.getInVector(blocks);
		item->setLocation(*block);
	}
}
uint32_t BlockHasItems::getCount(const ItemType& itemType, const MaterialType& materialType) const
{
	auto found = std::ranges::find_if(m_items, [&](Item* item)
	{
		return item->m_itemType == itemType && item->m_materialType == materialType;
	});
	if(found == m_items.end())
		return 0;
	else
		return (*found)->getQuantity();
}
Item& BlockHasItems::getGeneric(const ItemType& itemType, const MaterialType& materialType) const
{
	auto found = std::ranges::find_if(m_items, [&](Item* item) { return item->m_itemType == itemType && item->m_materialType == materialType; });
	assert(found != m_items.end());
	return **found;
}
// TODO: buggy
bool BlockHasItems::hasInstalledItemType(const ItemType& itemType) const
{
	auto found = std::ranges::find_if(m_items, [&](Item* item) { return item->m_itemType == itemType; });
	return found != m_items.end() && (*found)->m_installed;
}
bool BlockHasItems::hasEmptyContainerWhichCanHoldFluidsCarryableBy(const Actor& actor) const
{
	for(const Item* item : m_items)
		//TODO: account for container weight when full, needs to have fluid type passed in.
		if(item->m_itemType.internalVolume != 0 && item->m_itemType.canHoldFluids && actor.m_canPickup.canPickupAny(*item))
			return true;
	return false;
}
bool BlockHasItems::hasContainerContainingFluidTypeCarryableBy(const Actor& actor, const FluidType& fluidType) const
{
	for(const Item* item : m_items)
		if(item->m_itemType.internalVolume != 0 && item->m_itemType.canHoldFluids && actor.m_canPickup.canPickupAny(*item) && item->m_hasCargo.getFluidType() == fluidType)
			return true;
	return false;
}
