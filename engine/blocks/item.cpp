#include "blocks.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../area/area.h"
#include "../simulation/simulation.h"
#include "../simulation/hasItems.h"
#include "../types.h"
#include "../itemType.h"
#include <iterator>
void Blocks::item_record(const BlockIndex& index, const ItemIndex& item, const CollisionVolume& volume)
{
	Items& items = m_area.getItems();
	if(items.isStatic(item))
		item_recordStatic(index, item, volume);
	else
		item_recordDynamic(index, item, volume);
}
void Blocks::item_recordStatic(const BlockIndex& index, const ItemIndex& item, const CollisionVolume& volume)
{
	assert(index.exists());
	m_itemVolume[index].emplace_back(item, volume);
	m_items[index].insert(item);
	m_staticVolume[index] += volume;
}
void Blocks::item_recordDynamic(const BlockIndex& index, const ItemIndex& item, const CollisionVolume& volume)
{
	assert(index.exists());
	m_itemVolume[index].emplace_back(item, volume);
	m_items[index].insert(item);
	m_dynamicVolume[index] += volume;
}
void Blocks::item_erase(const BlockIndex& index, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	if(items.isStatic(item))
		item_eraseStatic(index, item);
	else
		item_eraseDynamic(index, item);
}
void Blocks::item_eraseDynamic(const BlockIndex& index, const ItemIndex& item)
{
	auto& blockItems = m_items[index];
	auto& blockItemVolume = m_itemVolume[index];
	auto iter = blockItems.find(item);
	uint16_t offset = &*iter - &*blockItems.begin();
	auto iter2 = m_itemVolume[index].begin() + offset;
	m_dynamicVolume[index] -= iter2->second;
	blockItems.erase(iter);
	(*iter2) = blockItemVolume.back();
	blockItemVolume.pop_back();
}
void Blocks::item_eraseStatic(const BlockIndex& index, const ItemIndex& item)
{
	auto& blockItems = m_items[index];
	auto& blockItemVolume = m_itemVolume[index];
	auto iter = blockItems.find(item);
	uint16_t offset = &*iter - &*blockItems.begin();
	auto iter2 = m_itemVolume[index].begin() + offset;
	m_staticVolume[index] -= iter2->second;
	blockItems.erase(iter);
	(*iter2) = blockItemVolume.back();
	blockItemVolume.pop_back();
}
void Blocks::item_setTemperature(const BlockIndex& index, const Temperature& temperature)
{
	Items& items = m_area.getItems();
	for(const ItemIndex& item : m_items[index])
		items.setTemperature(item, temperature, index);
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
		const BlockIndex block = blocks.random(m_area.m_simulation);
		const Facing4 facing = (Facing4)(m_area.m_simulation.m_random.getInRange(0, 3));
		// TODO: use location_tryToSetStatic and find another location on fail.
		items.location_setStatic(item, block, facing);
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
	assert(shape_anythingCanEnterEver(index));
	Items& items = m_area.getItems();
	CollisionVolume volume = Shape::getCollisionVolumeAtLocationBlock(ItemType::getShape(itemType)) * quantity;
	// Assume that generics added through this method are static.
	// Generics in transit ( being hauled by multiple workers ) should not use this method and should use setLocationAndFacing instead.
	m_staticVolume[index] += volume;
	auto& blockItems = m_items[index];
	auto found = blockItems.findIf([itemType, &items](const auto& item) { return items.getItemType(item) == itemType; });
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
	const auto found = itemsInBlock.findIf([&](const ItemIndex& item)
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
	const auto found = itemsInBlock.findIf([&](const ItemIndex& item) {
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
ItemIndicesForBlock& Blocks::item_getAll(const BlockIndex& index)
{
	return m_items[index];
}
const ItemIndicesForBlock& Blocks::item_getAll(const BlockIndex& index) const
{
	return const_cast<Blocks*>(this)->item_getAll(index);
}
