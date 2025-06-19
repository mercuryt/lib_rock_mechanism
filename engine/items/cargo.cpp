#include "items.h"
#include "../definitions/itemType.h"
#include "../area/area.h"
#include "../definitions/materialType.h"
#include "../types.h"
#include "../portables.hpp"
#include "../actors/actors.h"
#include <regex>
void Items::cargo_addActor(const ItemIndex& index, const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	assert(ItemType::getInternalVolume(m_itemType[index]).exists());
	assert(ItemType::getInternalVolume(m_itemType[index]) > actors.getVolume(actor));
	if(m_hasCargo[index] == nullptr)
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(m_itemType[index]);
	m_hasCargo[index]->addActor(m_area, actor);
	actors.setCarrier(actor, ActorOrItemIndex::createForItem(index));
}
ItemIndex Items::cargo_addItem(const ItemIndex& index, const ItemIndex& item, const Quantity& quantity)
{
	assert(getLocation(item).empty());
	if(m_hasCargo[index] == nullptr)
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(m_itemType[index]);
	ItemIndex output;
	if(isGeneric(item) && quantity != m_quantity[item])
	{
		removeQuantity(item, quantity);
		output = m_hasCargo[index]->addItemGeneric(m_area, getItemType(item), getMaterialType(item), quantity);
	}
	else
	{
		m_hasCargo[index]->addItem(m_area, item);
		output = item;
	}
	Items& items = m_area.getItems();
	items.setCarrier(item, ActorOrItemIndex::createForItem(index));
	return output;
}
void Items::cargo_addItemGeneric(const ItemIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	assert(ItemType::getIsGeneric(itemType));
	if(m_hasCargo[index] == nullptr)
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(m_itemType[index]);
	ItemIndex addedItem = m_hasCargo[index]->addItemGeneric(m_area, itemType, materialType, quantity);
	Items& items = m_area.getItems();
	items.maybeSetCarrier(addedItem, ActorOrItemIndex::createForItem(index));
}
void Items::cargo_addPolymorphic(const ItemIndex& index, const ActorOrItemIndex& actorOrItemIndex, const Quantity& quantity)
{
	assert(actorOrItemIndex.exists());
	assert(actorOrItemIndex.getLocation(m_area).empty());
	if(m_hasCargo[index] == nullptr)
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(m_itemType[index]);
	if(actorOrItemIndex.isActor())
		cargo_addActor(index, actorOrItemIndex.getActor());
	else
		cargo_addItem(index, actorOrItemIndex.getItem(), quantity);
}
void Items::cargo_addFluid(const ItemIndex& index, const FluidTypeId& fluidType, const CollisionVolume& volume)
{
	if(m_hasCargo[index] == nullptr)
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(m_itemType[index]);
	m_hasCargo[index]->addFluid(fluidType, volume);
}
void Items::cargo_loadActor(const ItemIndex& index, const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	assert(actors.hasLocation(actor));
	actors.location_clear(actor);
	cargo_addActor(index, actor);
}
ItemIndex Items::cargo_loadItem(const ItemIndex& index, const ItemIndex& item, const Quantity& quantity)
{
	Items& items = m_area.getItems();
	assert(items.hasLocation(item));
	items.location_clear(item);
	return cargo_addItem(index, item, quantity);
}
ActorOrItemIndex Items::cargo_loadPolymorphic(const ItemIndex& index, const ActorOrItemIndex& actorOrItem, const Quantity& quantity)
{
	assert(actorOrItem.exists());
	if(actorOrItem.isActor())
	{
		cargo_loadActor(index, actorOrItem.getActor());
		return actorOrItem;
	}
	else
		return ActorOrItemIndex::createForItem(cargo_loadItem(index, actorOrItem.getItem(), quantity));
}
void Items::cargo_loadFluidFromLocation(const ItemIndex& index, const FluidTypeId& fluidType, const CollisionVolume& volume, const BlockIndex& location)
{
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.fluid_volumeOfTypeContains(location, fluidType) >= volume);
	blocks.fluid_remove(location, volume, fluidType);
	cargo_addFluid(index, fluidType, volume);
}
void Items::cargo_loadFluidFromItem(const ItemIndex& index, const FluidTypeId& fluidType, const CollisionVolume& volume, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	assert(items.cargo_getFluidVolume(item) >= volume);
	assert(items.cargo_getFluidType(item) == fluidType);
	items.cargo_removeFluid(item, volume);
	items.cargo_addFluid(index, fluidType, volume);
}
void Items::cargo_remove(const ItemIndex& index, const ActorOrItemIndex& actorOrItem)
{
	if(actorOrItem.isActor())
		cargo_removeActor(index, actorOrItem.getActor());
	else
		cargo_removeItem(index, actorOrItem.getItem());
}
void Items::cargo_removeActor(const ItemIndex& index, const ActorIndex& actor)
{
	assert(m_hasCargo[index]);
	ItemHasCargo& hasCargo = *m_hasCargo[index];
	hasCargo.removeActor(m_area, actor);
	if(hasCargo.empty())
		m_hasCargo[index] = nullptr;
	Actors& actors = m_area.getActors();
	actors.unsetCarrier(actor, ActorOrItemIndex::createForItem(index));
}
void Items::cargo_removeItem(const ItemIndex& index, const ItemIndex& item)
{
	assert(m_hasCargo[index]);
	ItemHasCargo& hasCargo = *m_hasCargo[index];
	hasCargo.removeItem(m_area, item);
	if(hasCargo.empty())
		m_hasCargo[index] = nullptr;
	Items& items = m_area.getItems();
	items.unsetCarrier(item, ActorOrItemIndex::createForItem(index));
}
void Items::cargo_removeItemGeneric(const ItemIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	assert(m_hasCargo[index]);
	ItemHasCargo& hasCargo = *m_hasCargo[index];
	hasCargo.removeItemGeneric(m_area, itemType, materialType, quantity);
	if(hasCargo.empty())
		m_hasCargo[index] = nullptr;
}
void Items::cargo_removeFluid(const ItemIndex& index, const CollisionVolume& volume)
{
	assert(m_hasCargo[index] != nullptr);
	assert(cargo_containsAnyFluid(index));
	assert(cargo_getFluidVolume(index) >= volume);
	auto& hasCargo = *m_hasCargo[index];
	//TODO: passing fluid type is pointless here, either pass it into cargo_removeFluid or make a HasCargo::removeFluidVolume(Volume volume)
	hasCargo.removeFluidVolume(hasCargo.getFluidType(), volume);
	if(hasCargo.empty())
		m_hasCargo[index] = nullptr;
}
void Items::cargo_unloadActorToLocation(const ItemIndex& index, const ActorIndex& actor, const BlockIndex& location)
{
	assert(m_hasCargo[index]);
	assert(cargo_containsActor(index, actor));
	Actors& actors = m_area.getActors();
	assert(actors.getLocation(actor).empty());
	cargo_removeActor(index, actor);
	Blocks& blocks = m_area.getBlocks();
	const Facing4& facing = blocks.facingToSetWhenEnteringFrom(location, m_location[index]);
	actors.location_set(actor, location, facing);
}
void Items::cargo_unloadItemToLocation(const ItemIndex& index, const ItemIndex& item, const BlockIndex& location)
{
	assert(cargo_containsItem(index, item));
	assert(getLocation(item).empty());
	cargo_removeItem(index, item);
	Blocks& blocks = m_area.getBlocks();
	const Facing4& facing = blocks.facingToSetWhenEnteringFrom(location, m_location[index]);
	location_set(item, location, facing);
}
void Items::cargo_updateItemIndex(const ItemIndex& index, const ItemIndex& oldIndex, const ItemIndex& newIndex)
{
	m_hasCargo[index]->m_items.update(oldIndex, newIndex);
}
void Items::cargo_updateActorIndex(const ItemIndex& index, const ActorIndex& oldIndex, const ActorIndex& newIndex)
{
	m_hasCargo[index]->m_actors.update(oldIndex, newIndex);
}
ItemIndex Items::cargo_unloadGenericItemToLocation(const ItemIndex& index, const ItemIndex& item, const BlockIndex& location, const Quantity& quantity)
{
	return cargo_unloadGenericItemToLocation(index, getItemType(item), getMaterialType(item), location, quantity);
}
ItemIndex Items::cargo_unloadGenericItemToLocation(const ItemIndex& index,const ItemTypeId& itemType,const MaterialTypeId& materialType, const BlockIndex& location, const Quantity& quantity)
{
	cargo_removeItemGeneric(index, itemType, materialType, quantity);
	return m_area.getBlocks().item_addGeneric(location, itemType, materialType, quantity);
}
ActorOrItemIndex Items::cargo_unloadPolymorphicToLocation(const ItemIndex& index, const ActorOrItemIndex& actorOrItem, const BlockIndex& location, const Quantity& quantity)
{
	assert(actorOrItem.exists());
	assert(actorOrItem.getLocation(m_area).empty());
	if(actorOrItem.isActor())
	{
		cargo_unloadActorToLocation(index, actorOrItem.getActor(), location);
	}
	else
	{
		ItemIndex item = actorOrItem.getItem();
		if(isGeneric(item) && m_area.getItems().getQuantity(item) != quantity)
		{
			ItemIndex unloaded = cargo_unloadGenericItemToLocation(index, item, location, quantity);
			return ActorOrItemIndex::createForItem(unloaded);
		}
		else
			cargo_unloadItemToLocation(index, item, location);
	}
	return actorOrItem;
}
void Items::cargo_unloadFluidToLocation(const ItemIndex& index, const CollisionVolume& volume, const BlockIndex& location)
{
	assert(cargo_getFluidVolume(index) >= volume);
	FluidTypeId fluidType = cargo_getFluidType(index);
	cargo_removeFluid(index, volume);
	m_area.getBlocks().fluid_add(location, volume, fluidType);
}
bool Items::cargo_exists(const ItemIndex& index) const { return m_hasCargo[index] != nullptr; }
bool Items::cargo_containsActor(const ItemIndex& index, const ActorIndex& actor) const
{
	if(m_hasCargo[index] == nullptr)
		return false;
	return m_hasCargo[index]->containsActor(actor);
}
bool Items::cargo_containsItem(const ItemIndex& index, const ItemIndex& item) const
{
	if(m_hasCargo[index] == nullptr)
		return false;
	return m_hasCargo[index]->containsItem(item);
}
bool Items::cargo_containsItemGeneric(const ItemIndex& index,const ItemTypeId& itemType,const MaterialTypeId& materialType, const Quantity& quantity) const
{
	if(m_hasCargo[index] == nullptr)
		return false;
	return m_hasCargo[index]->containsGeneric(m_area, itemType, materialType, quantity);
}
bool Items::cargo_containsPolymorphic(const ItemIndex& index, const ActorOrItemIndex& actorOrItem, const Quantity quantity) const
{
	if(actorOrItem.isActor())
		return cargo_containsActor(index, actorOrItem.getActor());
	else
		return cargo_containsItem(index, actorOrItem.getItem()) && getQuantity(actorOrItem.getItem()) >= quantity;
}
bool Items::cargo_containsAnyFluid(const ItemIndex& index) const
{
	if(m_hasCargo[index] == nullptr)
		return false;
	return m_hasCargo[index]->containsAnyFluid();
}
bool Items::cargo_containsFluidType(const ItemIndex& index, const FluidTypeId& fluidType) const
{
	if(m_hasCargo[index] == nullptr)
		return false;
	return m_hasCargo[index]->containsFluidType(fluidType);
}
CollisionVolume Items::cargo_getFluidVolume(const ItemIndex& index) const
{
	if(m_hasCargo[index] == nullptr)
		return CollisionVolume::create(0);
	return m_hasCargo[index]->getFluidVolume();
}
FluidTypeId Items::cargo_getFluidType(const ItemIndex& index) const
{
	return m_hasCargo[index]->getFluidType();
}
bool Items::cargo_canAddActor(const ItemIndex& index, const ActorIndex& actor) const
{
	if(m_hasCargo[index] != nullptr)
		return m_hasCargo[index]->canAddActor(m_area, actor);
	else
		return ItemType::getInternalVolume(m_itemType[index]) >= m_area.getActors().getVolume(actor);
}
bool Items::cargo_canAddItem(const ItemIndex& index, const ItemIndex& item) const
{
	if(m_hasCargo[index] != nullptr)
		return m_hasCargo[index]->canAddItem(m_area, item);
	else
		return ItemType::getInstallable(m_itemType[index]) >= m_area.getItems().getVolume(item);
}
Mass Items::cargo_getMass(const ItemIndex& index) const
{
	return m_hasCargo[index]->getMass();
}
const SmallSet<ItemIndex>& Items::cargo_getItems(const ItemIndex& index) const { return m_hasCargo[index]->getItems(); }
const SmallSet<ActorIndex>& Items::cargo_getActors(const ItemIndex& index) const { return const_cast<ItemHasCargo&>(*m_hasCargo[index]).getActors(); }
