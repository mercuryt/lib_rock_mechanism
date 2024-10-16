#include "items.h"
#include "../itemType.h"
#include "../area.h"
#include "../materialType.h"
#include "../types.h"
#include "../actors/actors.h"
#include <regex>
void Items::cargo_addActor(ItemIndex index, ActorIndex actor)
{
	assert(ItemType::getInternalVolume(m_itemType[index]).exists());
	assert(ItemType::getInternalVolume(m_itemType[index]) > m_area.getActors().getVolume(actor));
	if(m_hasCargo[index] == nullptr)
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(m_itemType[index]);
	m_hasCargo[index]->addActor(m_area, actor);
}
void Items::cargo_addItem(ItemIndex index, ItemIndex item, Quantity quantity)
{
	assert(getLocation(item).empty());
	if(m_hasCargo[index] == nullptr)
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(m_itemType[index]);
	if(isGeneric(item) && quantity != m_quantity[item])
	{
		removeQuantity(item, quantity);
		m_hasCargo[index]->addItemGeneric(m_area, getItemType(item), getMaterialType(item), quantity);
	}
	else
		m_hasCargo[index]->addItem(m_area, item);
}
void Items::cargo_addItemGeneric(ItemIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity)
{
	assert(ItemType::getIsGeneric(itemType));
	if(m_hasCargo[index] == nullptr)
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(m_itemType[index]);
	m_hasCargo[index]->addItemGeneric(m_area, itemType, materialType, quantity);
}
void Items::cargo_addPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItemIndex, Quantity quantity)
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
void Items::cargo_addFluid(ItemIndex index, FluidTypeId fluidType, CollisionVolume volume)
{
	if(m_hasCargo[index] == nullptr)
		m_hasCargo[index] = std::make_unique<ItemHasCargo>(m_itemType[index]);
	m_hasCargo[index]->addFluid(fluidType, volume);
}
void Items::cargo_loadActor(ItemIndex index, ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	assert(actors.hasLocation(actor));
	actors.exit(actor);
	cargo_addActor(index, actor);
}
void Items::cargo_loadItem(ItemIndex index, ItemIndex item, Quantity quantity)
{
	Items& items = m_area.getItems();
	assert(items.hasLocation(item));
	items.exit(item);
	cargo_addItem(index, item, quantity);
}
void Items::cargo_loadPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItem, Quantity quantity)
{
	assert(actorOrItem.exists());
	if(actorOrItem.isActor())
		cargo_loadActor(index, actorOrItem.getActor());
	else
		cargo_loadItem(index, actorOrItem.getItem(), quantity);
}
void Items::cargo_loadFluidFromLocation(ItemIndex index, FluidTypeId fluidType, CollisionVolume volume, BlockIndex location)
{
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.fluid_volumeOfTypeContains(location, fluidType) >= volume);
	blocks.fluid_remove(location, volume, fluidType);
	cargo_addFluid(index, fluidType, volume);
}
void Items::cargo_loadFluidFromItem(ItemIndex index, FluidTypeId fluidType, CollisionVolume volume, ItemIndex item)
{
	Items& items = m_area.getItems();
	assert(items.cargo_getFluidVolume(item) >= volume);
	assert(items.cargo_getFluidType(item) == fluidType);
	items.cargo_removeFluid(item, volume);
	items.cargo_addFluid(index, fluidType, volume);
}
void Items::cargo_removeActor(ItemIndex index, ActorIndex actor)
{
	assert(m_hasCargo[index]);
	ItemHasCargo& hasCargo = *m_hasCargo[index];
	hasCargo.removeActor(m_area, actor);
	if(hasCargo.empty())
		m_hasCargo[index] = nullptr;
}
void Items::cargo_removeItem(ItemIndex index, ItemIndex item)
{
	assert(m_hasCargo[index]);
	ItemHasCargo& hasCargo = *m_hasCargo[index];
	hasCargo.removeItem(m_area, item);
	if(hasCargo.empty())
		m_hasCargo[index] = nullptr;
}
void Items::cargo_removeItemGeneric(ItemIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity)
{
	assert(m_hasCargo[index]);
	ItemHasCargo& hasCargo = *m_hasCargo[index];
	hasCargo.removeItemGeneric(m_area, itemType, materialType, quantity);
	if(hasCargo.empty())
		m_hasCargo[index] = nullptr;
}
void Items::cargo_removeFluid(ItemIndex index, CollisionVolume volume)
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
void Items::cargo_unloadActorToLocation(ItemIndex index, ActorIndex actor, BlockIndex location)
{
	assert(m_hasCargo[index]);
	assert(cargo_containsActor(index, actor));
	Actors& actors = m_area.getActors();
	assert(actors.getLocation(actor).empty());
	cargo_removeActor(index, actor);
	actors.setLocation(actor, location);
}
void Items::cargo_unloadItemToLocation(ItemIndex index, ItemIndex item, BlockIndex location)
{
	assert(cargo_containsItem(index, item));
	assert(getLocation(item).empty());
	cargo_removeItem(index, item);
	setLocationAndFacing(item, location, Facing::create(0));
}
void Items::cargo_updateItemIndex(ItemIndex index, ItemIndex oldIndex, ItemIndex newIndex)
{
	m_hasCargo[index]->m_items.update(oldIndex, newIndex);
}
void Items::cargo_updateActorIndex(ItemIndex index, ActorIndex oldIndex, ActorIndex newIndex)
{
	m_hasCargo[index]->m_actors.update(oldIndex, newIndex);
}
ItemIndex Items::cargo_unloadGenericItemToLocation(ItemIndex index, ItemIndex item, BlockIndex location, Quantity quantity)
{
	return cargo_unloadGenericItemToLocation(index, getItemType(item), getMaterialType(item), location, quantity);
}
ItemIndex Items::cargo_unloadGenericItemToLocation(ItemIndex index,ItemTypeId itemType,MaterialTypeId materialType, BlockIndex location, Quantity quantity)
{
	cargo_removeItemGeneric(index, itemType, materialType, quantity);
	return m_area.getBlocks().item_addGeneric(location, itemType, materialType, quantity);
}
ActorOrItemIndex Items::cargo_unloadPolymorphicToLocation(ItemIndex index, ActorOrItemIndex actorOrItem, BlockIndex location, Quantity quantity)
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
void Items::cargo_unloadFluidToLocation(ItemIndex index, CollisionVolume volume, BlockIndex location)
{
	assert(cargo_getFluidVolume(index) >= volume);
	FluidTypeId fluidType = cargo_getFluidType(index);
	cargo_removeFluid(index, volume);
	m_area.getBlocks().fluid_add(location, volume, fluidType);
}
bool Items::cargo_exists(ItemIndex index) const { return m_hasCargo[index] != nullptr; }
bool Items::cargo_containsActor(ItemIndex index, ActorIndex actor) const
{
	if(m_hasCargo[index] == nullptr)
		return false;
	return m_hasCargo[index]->containsActor(actor);
}
bool Items::cargo_containsItem(ItemIndex index, ItemIndex item) const
{
	if(m_hasCargo[index] == nullptr)
		return false;
	return m_hasCargo[index]->containsItem(item);
}
bool Items::cargo_containsItemGeneric(ItemIndex index,ItemTypeId itemType,MaterialTypeId materialType, Quantity quantity) const
{
	if(m_hasCargo[index] == nullptr)
		return false;
	return m_hasCargo[index]->containsGeneric(m_area, itemType, materialType, quantity);
}
bool Items::cargo_containsPolymorphic(ItemIndex index, ActorOrItemIndex actorOrItem, Quantity quantity) const
{
	if(actorOrItem.isActor())
		return cargo_containsActor(index, actorOrItem.getActor());
	else
		return cargo_containsItem(index, actorOrItem.getItem()) && getQuantity(actorOrItem.getItem()) >= quantity;
}
bool Items::cargo_containsAnyFluid(ItemIndex index) const
{
	if(m_hasCargo[index] == nullptr)
		return false;
	return m_hasCargo[index]->containsAnyFluid();
}
bool Items::cargo_containsFluidType(ItemIndex index, FluidTypeId fluidType) const
{
	if(m_hasCargo[index] == nullptr)
		return false;
	return m_hasCargo[index]->containsFluidType(fluidType);
}
CollisionVolume Items::cargo_getFluidVolume(ItemIndex index) const
{
	if(m_hasCargo[index] == nullptr)
		return CollisionVolume::create(0);
	return m_hasCargo[index]->getFluidVolume();
}
FluidTypeId Items::cargo_getFluidType(ItemIndex index) const
{
	return m_hasCargo[index]->getFluidType();
}
bool Items::cargo_canAddActor(ItemIndex index, ActorIndex actor) const
{
	if(m_hasCargo[index] != nullptr)
		return m_hasCargo[index]->canAddActor(m_area, actor);
	else
		return ItemType::getInternalVolume(m_itemType[index]) >= m_area.getActors().getVolume(actor);
}
bool Items::cargo_canAddItem(ItemIndex index, ItemIndex item) const
{
	if(m_hasCargo[index] != nullptr)
		return m_hasCargo[index]->canAddItem(m_area, item);
	else
		return ItemType::getInstallable(m_itemType[index]) >= m_area.getItems().getVolume(item);
}
Mass Items::cargo_getMass(ItemIndex index) const
{
	return m_hasCargo[index]->getMass();
}
const ItemIndices& Items::cargo_getItems(ItemIndex index) const { return m_hasCargo[index]->getItems(); }
const ActorIndices& Items::cargo_getActors(ItemIndex index) const { return const_cast<ItemHasCargo&>(*m_hasCargo[index]).getActors(); }
