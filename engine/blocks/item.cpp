#include "blocks.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../area.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "../types.h"
#include "../itemType.h"
#include "util.h"
#include <iterator>
void Blocks::item_record(BlockIndex index, ItemIndex item, CollisionVolume volume)
{
	m_itemVolume.at(index).emplace_back(item, volume);
	Items& items = m_area.getItems();
	m_items.at(index).push_back(item);
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
	auto iter2 = m_itemVolume.at(index).begin() + (std::distance(iter, m_dynamicVolume.begin()));
	if(items.isStatic(item))
		m_staticVolume.at(index) -= iter2->second;
	else
		m_dynamicVolume.at(index) -= iter2->second;
	(*iter) = blockItems.back();
	(*iter2) = blockItemVolume.back();
	blockItemVolume.pop_back();
	blockItems.pop_back();
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
	std::vector<BlockIndex> blocks;
	for(BlockIndex otherIndex : getAdjacentOnSameZLevelOnly(index))
		if(!solid_is(otherIndex))
			blocks.push_back(otherIndex);
	auto copy = itemsInBlock;
	Items& items = m_area.getItems();
	for(auto [item, volume] : copy)
	{
		//TODO: split up stacks of generics, prefer blocks with more empty space.
		Random& random = m_area.m_simulation.m_random;
		BlockIndex block = blocks.at(random.getInRange(0u, (uint)(blocks.size() - 1)));
		items.setLocation(item, block);
	}
}
uint32_t Blocks::item_getCount(BlockIndex index, const ItemType& itemType, const MaterialType& materialType) const
{
	auto& itemsInBlock = m_itemVolume.at(index);
	Items& items = m_area.getItems();
	auto found = std::ranges::find_if(itemsInBlock, [&](auto pair)
	{
		ItemIndex item = pair.first;
		return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType;
	});
	if(found == itemsInBlock.end())
		return 0;
	else
		return items.getQuantity(found->first);
}
ItemIndex Blocks::item_getGeneric(BlockIndex index, const ItemType& itemType, const MaterialType& materialType) const
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
bool Blocks::item_hasInstalledType(BlockIndex index, const ItemType& itemType) const
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
		const ItemType& itemType = items.getItemType(item);
		//TODO: account for container weight when full, needs to have fluid type passed in.
		if(itemType.internalVolume != 0 && itemType.canHoldFluids && actors.canPickUp_anyWithMass(actor, items.getMass(item)))
			return true;
	}
	return false;
}
bool Blocks::item_hasContainerContainingFluidTypeCarryableBy(BlockIndex index, const ActorIndex actor, const FluidType& fluidType) const
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	for(auto [item, volume]  : m_itemVolume.at(index))
	{
		const ItemType& itemType = items.getItemType(item);
		if(itemType.internalVolume != 0 && itemType.canHoldFluids &&
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
	return util::vectorContains(m_items.at(index), item);
}
std::vector<ItemIndex> Blocks::item_getAll(BlockIndex index)
{
	return m_items.at(index);
}
const std::vector<ItemIndex> Blocks::item_getAll(BlockIndex index) const
{
	return const_cast<Blocks*>(this)->item_getAll(index);
}
