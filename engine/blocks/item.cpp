#include "blocks.h"
#include "../items/items.h"
#include "../area.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "../types.h"
#include "../itemType.h"
#include <iterator>
void Blocks::item_record(BlockIndex index, ItemIndex item, CollisionVolume volume)
{
	m_items.at(index).emplace_back(item, volume);
	if(m_area.m_items.isStatic(item))
		m_staticVolume.at(index) += volume;
	else
		m_dynamicVolume.at(index) += volume;
}
void Blocks::item_erase(BlockIndex index, ItemIndex item)
{
	assert(m_area.m_items.getLocation(item) == index);
	auto& items = m_items.at(index);
	auto found = std::ranges::find(items, item, &std::pair<ItemIndex, CollisionVolume>::first);
	assert(found != items.end());
	if(m_area.m_items.isStatic(item))
		m_staticVolume.at(index) -= found->second;
	else
		m_dynamicVolume.at(index) -= found->second;
	items.erase(found);
}
void Blocks::item_setTemperature(BlockIndex index, Temperature temperature)
{
	for(auto [item, volume] : m_items.at(index))
		m_area.m_items.setTemperature(item, temperature);
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
	for(auto [item, volume] : copy)
	{
		//TODO: split up stacks of generics, prefer blocks with more empty space.
		Random& random = m_area.m_simulation.m_random;
		BlockIndex block = blocks.at(random.getInRange(0u, (uint)(blocks.size() - 1)));
		m_area.m_items.setLocation(item, block);
	}
}
uint32_t Blocks::item_getCount(BlockIndex index, const ItemType& itemType, const MaterialType& materialType) const
{
	auto& itemsInBlock = m_items.at(index);
	Items& items = m_area.m_items;
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
	auto& itemsInBlock = m_items.at(index);
	Items& items = m_area.m_items;
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
	auto& itemsInBlock = m_items.at(index);
	Items& items = m_area.m_items;
	auto found = std::ranges::find_if(itemsInBlock, [&](auto pair) { 
		ItemIndex item = pair.first;
		return items.getItemType(item) == itemType;
	});
	return found != itemsInBlock.end() && items.isInstalled(found->first);
}
bool Blocks::item_hasEmptyContainerWhichCanHoldFluidsCarryableBy(BlockIndex index, const ActorIndex actor) const
{
	Items& items = m_area.m_items;
	Actors& actors = m_area.m_actors;
	for(auto [item, volume] : m_items.at(index))
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
	Items& items = m_area.m_items;
	Actors& actors = m_area.m_actors;
	for(auto [item, volume]  : m_items.at(index))
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
	return m_items.at(index).empty();
}
std::vector<ItemIndex> Blocks::item_getAll(BlockIndex index)
{
	std::vector<ItemIndex> output;
	output.reserve(m_items.at(index).size());
	std::ranges::transform(m_items.at(index), std::back_inserter(output), [](const auto& pair) -> ItemIndex { return pair.first; });
	return output;
}
const std::vector<ItemIndex> Blocks::item_getAll(BlockIndex index) const
{
	return const_cast<Blocks*>(this)->item_getAll(index);
}
