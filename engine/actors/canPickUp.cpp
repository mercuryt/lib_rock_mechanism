#include "actorOrItemIndex.h"
#include "actors.h"
#include "../items/items.h"
#include "../area.h"
#include "../itemType.h"
#include "index.h"
#include "sleep.h"
#include "types.h"
void Actors::canPickUp_pickUpItem(ActorIndex index, ItemIndex item)
{
	canPickUp_pickUpItemQuantity(index, item, Quantity::create(0));
}
void Actors::canPickUp_pickUpItemQuantity(ActorIndex index, ItemIndex item, Quantity quantity)
{
	Items& items = m_area.getItems();
	assert(quantity <= items.getQuantity(item));
	assert(quantity == 1 || ItemType::getGeneric(items.getItemType(item)));
	items.reservable_maybeUnreserve(item, *m_canReserve.at(index));
	if(quantity == items.getQuantity(item))
	{
		m_carrying.at(index) = item.toActorOrItemIndex();
		if(items.getLocation(item).exists())
			items.exit(item);
	}
	else
	{
		items.removeQuantity(item, quantity);
		ItemIndex newItem = items.create({
			.itemType=items.getItemType(item), 
			.materialType=items.getMaterialType(item), 
			.quantity=quantity
		});
		m_carrying.at(index) = ActorOrItemIndex::createForItem(newItem);
	}
	move_updateIndividualSpeed(index);
}
void Actors::canPickUp_pickUpActor(ActorIndex index, ActorIndex other)
{
	assert(!m_mustSleep.at(other)->isAwake() || !move_canMove(other));
	assert(m_carrying.at(index).exists());
	m_reservables.at(other)->maybeClearReservationFor(*m_canReserve.at(index));
	if(m_location.at(other).exists())
		exit(other);
	m_carrying.at(index) = ActorOrItemIndex::createForActor(other);
	move_updateIndividualSpeed(index);
}
void Actors::canPickUp_pickUpPolymorphic(ActorIndex index, ActorOrItemIndex actorOrItemIndex, Quantity quantity)
{
	assert(actorOrItemIndex.exists());
	if(actorOrItemIndex.isActor())
		canPickUp_pickUpActor(index, actorOrItemIndex.getActor());
	else
		canPickUp_pickUpItemQuantity(index, actorOrItemIndex.getItem(), quantity);
}
ActorIndex Actors::canPickUp_tryToPutDownActor(ActorIndex index, BlockIndex location, DistanceInBlocks maxRange)
{
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isActor());
	ActorIndex other = m_carrying.at(index).getActor();
	Blocks& blocks = m_area.getBlocks();
	auto predicate = [&](BlockIndex block) { return blocks.actor_canEnterCurrentlyWithAnyFacing(block, other); };
	BlockIndex location2 = blocks.getBlockInRangeWithCondition(location, maxRange, predicate);
	if(location2.empty())
		return ActorIndex::null();
	m_carrying.at(index).clear();
	move_updateIndividualSpeed(index);
	setLocation(other, location2);
	return other;
}
ItemIndex Actors::canPickUp_tryToPutDownItem(ActorIndex index, BlockIndex location, DistanceInBlocks maxRange)
{
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isItem());
	ItemIndex item = m_carrying.at(index).getItem();
	Blocks& blocks = m_area.getBlocks();
	auto predicate = [&](BlockIndex block) { return blocks.item_canEnterCurrentlyWithAnyFacing(block, item); };
	BlockIndex location2 = blocks.getBlockInRangeWithCondition(location, maxRange, predicate);
	if(location2.empty())
		return ItemIndex::null();
	m_carrying.at(index).clear();
	move_updateIndividualSpeed(index);
	m_area.getItems().setLocation(item, location2);
	return item;
}
ActorOrItemIndex Actors::canPickUp_tryToPutDownIfAny(ActorIndex index, BlockIndex location, DistanceInBlocks maxRange)
{
	if(!m_carrying.at(index).exists())
		return m_carrying.at(index);
	return canPickUp_tryToPutDownPolymorphic(index, location, maxRange);
}
ActorOrItemIndex Actors::canPickUp_tryToPutDownPolymorphic(ActorIndex index, BlockIndex location, DistanceInBlocks maxRange)
{
	assert(m_carrying.at(index).exists());
	if(m_carrying.at(index).isActor())
		return ActorOrItemIndex::createForActor(canPickUp_tryToPutDownActor(index, location, maxRange));
	else
		return ActorOrItemIndex::createForItem(canPickUp_tryToPutDownItem(index, location, maxRange));
}
void Actors::canPickUp_removeFluidVolume(ActorIndex index, CollisionVolume volume)
{
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isItem());
	ItemIndex item = m_carrying.at(index).getItem();
	m_area.getItems().cargo_removeFluid(item, volume);
}
void Actors::canPickUp_add(ActorIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity)
{
	Items& items = m_area.getItems();
	auto& carrying = m_carrying.at(index);
	if(carrying.exists())
	{
		assert(carrying.isItem());
		ItemIndex item = carrying.getItem();
		assert(items.getItemType(item) == itemType);
		assert(items.getMaterialType(item) == materialType);
		items.addQuantity(item, quantity);
	}
	else
	{
		ItemIndex item = items.create({.itemType=itemType, .materialType=materialType, .quantity=quantity});
		carrying = ActorOrItemIndex::createForItem(item);
	}
	move_updateIndividualSpeed(index);
}
void Actors::canPickUp_removeItem(ActorIndex index, ItemIndex item)
{
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isItem());
	assert(m_carrying.at(index).get() == item);
	m_carrying.at(index).clear();
	Items& items = m_area.getItems();
	items.destroy(item);
	move_updateIndividualSpeed(index);
}
Quantity Actors::canPickUp_quantityWhichCanBePickedUpUnencombered(ActorIndex index, ItemTypeId itemType, MaterialTypeId materialType) const
{
	return canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(index, ItemType::getVolume(itemType) * MaterialType::getDensity(materialType), Config::minimumHaulSpeedInital);
}
bool Actors::canPickUp_polymorphic(ActorIndex index, ActorOrItemIndex actorOrItemIndex) const
{
	Mass mass = actorOrItemIndex.getSingleUnitMass(m_area);
	return canPickUp_anyWithMass(index, mass);
}
bool Actors::canPickUp_item(ActorIndex index, ItemIndex item) const
{
	Mass mass = m_area.getItems().getMass(item);
	return canPickUp_anyWithMass(index, mass);
}
bool Actors::canPickUp_itemQuantity(ActorIndex index, ItemIndex item, Quantity quantity) const
{
	Mass mass = m_area.getItems().getMass(item) * quantity;
	return canPickUp_anyWithMass(index, mass);
}
bool Actors::canPickUp_singleItem(ActorIndex index, ItemIndex item) const
{
	Mass mass = m_area.getItems().getSingleUnitMass(item);
	return canPickUp_anyWithMass(index, mass);
}
bool Actors::canPickUp_actor(ActorIndex index, ActorIndex actor) const
{
	Mass mass = m_area.getActors().getMass(actor);
	return canPickUp_anyWithMass(index, mass);
}
bool Actors::canPickUp_anyWithMass(ActorIndex index, Mass mass) const
{
	return canPickUp_speedIfCarryingQuantity(index, mass, Quantity::create(0)) > 0;
}
bool Actors::canPickUp_polymorphicUnencombered(ActorIndex index, ActorOrItemIndex actorOrItemIndex) const
{
	Mass mass = actorOrItemIndex.getSingleUnitMass(m_area);
	return canPickUp_anyWithMassUnencombered(index, mass);
}
bool Actors::canPickUp_itemUnencombered(ActorIndex index, ItemIndex item) const
{
	Mass mass = m_area.getItems().getSingleUnitMass(item);
	return canPickUp_anyWithMassUnencombered(index, mass);
}
bool Actors::canPickUp_actorUnencombered(ActorIndex index, ActorIndex actor) const
{
	Mass mass = m_area.getActors().getMass(actor);
	return canPickUp_anyWithMassUnencombered(index, mass);
}
bool Actors::canPickUp_anyWithMassUnencombered(ActorIndex index, Mass mass) const
{
	return canPickUp_speedIfCarryingQuantity(index, mass, Quantity::create(0)) == move_getIndividualSpeedWithAddedMass(index, Mass::create(0));
}
bool Actors::canPickUp_isCarryingItemGeneric(ActorIndex index, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity) const
{
	if(!m_carrying.at(index).exists() || !m_carrying.at(index).isItem())
		return false;
	ItemIndex item = m_carrying.at(index).getItem();
	Items& items = m_area.getItems();
	if(items.getItemType(item) != itemType)
		return false;
	if(items.getMaterialType(item) != materialType)
		return false;
	return items.getQuantity(item) >= quantity;
}
bool Actors::canPickUp_isCarryingFluidType(ActorIndex index, FluidTypeId fluidType) const 
{
	if(!m_carrying.at(index).exists())
		return false;
	ItemIndex item = m_carrying.at(index).getItem();
	Items& items = m_area.getItems();
	return items.cargo_containsFluidType(item, fluidType);
}
CollisionVolume Actors::canPickUp_getFluidVolume(ActorIndex index) const 
{ 
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isItem());
	return m_area.getItems().cargo_getFluidVolume(m_carrying.at(index).getItem());
}
FluidTypeId Actors::canPickUp_getFluidType(ActorIndex index) const 
{ 
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isItem());
	ItemIndex item = m_carrying.at(index).getItem();
	Items& items = m_area.getItems();
	assert(items.cargo_containsAnyFluid(item));
	return items.cargo_getFluidType(item); 
}
bool Actors::canPickUp_isCarryingEmptyContainerWhichCanHoldFluid(ActorIndex index) const 
{ 
	if(!m_carrying.at(index).exists() || !m_carrying.at(index).isItem())
		return false;
	ItemIndex item = m_carrying.at(index).getItem();
	Items& items = m_area.getItems();
	return ItemType::getCanHoldFluids(items.getItemType(item)) && !items.cargo_exists(item); 
}
Mass Actors::canPickUp_getMass(ActorIndex index) const
{
	if(!m_carrying.at(index).exists())
	{
		return Mass::null();
	}
	return m_carrying.at(index).getMass(m_area);
}
Speed Actors::canPickUp_speedIfCarryingQuantity(ActorIndex index, Mass mass, Quantity quantity) const
{
	assert(!m_carrying.at(index).exists());
	return move_getIndividualSpeedWithAddedMass(index, mass * quantity);
}
Quantity Actors::canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(ActorIndex index, Mass mass, Speed minimumSpeed) const
{
	assert(minimumSpeed != 0);
	//TODO: replace iteration with calculation?
	Quantity quantity = Quantity::create(0);
	while(canPickUp_speedIfCarryingQuantity(index, mass, quantity + 1) >= minimumSpeed)
		++quantity;
	return quantity;
}		
bool Actors::canPickUp_canPutDown(ActorIndex index, BlockIndex block)
{
	Blocks& blocks = m_area.getBlocks();
	auto& carrying = m_carrying.at(index);
	if(carrying.isActor())
		return blocks.actor_canEnterCurrentlyWithAnyFacing(block, carrying.getActor());
	else
		return blocks.item_canEnterCurrentlyWithAnyFacing(block, carrying.getItem());
}
void Actors::canPickUp_updateActorIndex(ActorIndex index, ActorIndex oldIndex, ActorIndex newIndex)
{
	assert(m_carrying.at(index).get() == oldIndex);
	assert(m_carrying.at(index).isActor());
	m_carrying.at(index).updateIndex(newIndex);
}
void Actors::canPickUp_updateItemIndex(ActorIndex index, ItemIndex oldIndex, ItemIndex newIndex)
{
	assert(m_carrying.at(index).get() == oldIndex);
	assert(m_carrying.at(index).isItem());
	m_carrying.at(index).updateIndex(newIndex);
}
void Actors::canPickUp_updateUnencomberedCarryMass(ActorIndex index)
{
	m_unencomberedCarryMass.at(index) = Mass::create(Config::unitsOfCarryMassPerUnitOfStrength * getStrength(index).get());
}
