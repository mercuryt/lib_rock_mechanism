#include "actorOrItemIndex.h"
#include "actors.h"
#include "../items/items.h"
#include "../area.h"
#include "../itemType.h"
#include "sleep.h"
#include "types.h"
void Actors::canPickUp_pickUpItem(ActorIndex index, ItemIndex item)
{
	canPickUp_pickUpItemQuantity(index, item, 1);
}
void Actors::canPickUp_pickUpItemQuantity(ActorIndex index, ItemIndex item, Quantity quantity)
{
	Items& items = m_area.getItems();
	assert(quantity <= items.getQuantity(item));
	assert(quantity == 1 || items.getItemType(item).generic);
	items.reservable_maybeUnreserve(item, *m_canReserve.at(index));
	if(quantity == items.getQuantity(item))
	{
		m_carrying.at(item) = ActorOrItemIndex::createForItem(item);
		if(items.getLocation(item) != BLOCK_INDEX_MAX)
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
	if(m_location.at(other) != BLOCK_INDEX_MAX)
		exit(other);
	m_carrying.at(index) = ActorOrItemIndex::createForActor(other);
	move_updateIndividualSpeed(index);
}
void Actors::canPickUp_pickUpPolymorphic(ActorIndex index, ActorOrItemIndex actorOrItemIndex, Quantity quantity)
{
	assert(actorOrItemIndex.exists());
	if(actorOrItemIndex.isActor())
		canPickUp_pickUpActor(index, actorOrItemIndex.get());
	else
		canPickUp_pickUpItemQuantity(index, actorOrItemIndex.get(), quantity);
}
ActorIndex Actors::canPickUp_putDownActor(ActorIndex index, BlockIndex location)
{
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isActor());
	ActorIndex other = m_carrying.at(index).get();
	m_carrying.at(index).clear();
	move_updateIndividualSpeed(index);
	setLocation(other, location);
	return other;
}
ItemIndex Actors::canPickUp_putDownItem(ActorIndex index, BlockIndex location)
{
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isItem());
	ItemIndex item = m_carrying.at(index).get();
	m_carrying.at(index).clear();
	move_updateIndividualSpeed(index);
	m_area.getItems().setLocation(item, location);
	return item;
}
ActorOrItemIndex Actors::canPickUp_putDownIfAny(ActorIndex index, BlockIndex location)
{
	if(!m_carrying.at(index).exists())
		return m_carrying.at(index);
	return canPickUp_putDownPolymorphic(index, location);
}
ActorOrItemIndex Actors::canPickUp_putDownPolymorphic(ActorIndex index, BlockIndex location)
{
	assert(m_carrying.at(index).exists());
	if(m_carrying.at(index).isActor())
		return ActorOrItemIndex::createForActor(canPickUp_putDownActor(index, location));
	else
		return ActorOrItemIndex::createForItem(canPickUp_putDownItem(index, location));
}
void Actors::canPickUp_removeFluidVolume(ActorIndex index, CollisionVolume volume)
{
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isItem());
	ItemIndex item = m_carrying.at(index).get();
	m_area.getItems().cargo_removeFluid(item, volume);
}
Quantity Actors::canPickUp_quantityWhichCanBePickedUpUnencombered(ActorIndex index, const ItemType& itemType, const MaterialType& materialType) const
{
	return canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(index, itemType.volume * materialType.density, Config::minimumHaulSpeedInital);
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
	return canPickUp_speedIfCarryingQuantity(index, mass, 1) > 0;
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
	return canPickUp_speedIfCarryingQuantity(index, mass, 1) == move_getIndividualSpeedWithAddedMass(index, 0);
}
bool Actors::canPickUp_isCarryingItemGeneric(ActorIndex index, const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const
{
	if(!m_carrying.at(index).exists() || !m_carrying.at(index).isItem())
		return false;
	ItemIndex item = m_carrying.at(index).get();
	Items& items = m_area.getItems();
	if(items.getItemType(item) != itemType)
		return false;
	if(items.getMaterialType(item) != materialType)
		return false;
	return items.getQuantity(item) >= quantity;
}
bool Actors::canPickUp_isCarryingFluidType(ActorIndex index, const FluidType& fluidType) const 
{
	if(!m_carrying.at(index).exists())
		return false;
	ItemIndex item = m_carrying.at(index).get();
	Items& items = m_area.getItems();
	return items.cargo_containsFluidType(item, fluidType);
}
CollisionVolume Actors::canPickUp_getFluidVolume(ActorIndex index) const 
{ 
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isItem());
	return m_area.getItems().cargo_getFluidVolume(m_carrying.at(index).get());
}
const FluidType& Actors::canPickUp_getFluidType(ActorIndex index) const 
{ 
	assert(m_carrying.at(index).exists());
	assert(m_carrying.at(index).isItem());
	ItemIndex item = m_carrying.at(index).get();
	Items& items = m_area.getItems();
	assert(items.cargo_containsAnyFluid(item));
	return items.cargo_getFluidType(item); 
}
bool Actors::canPickUp_isCarryingEmptyContainerWhichCanHoldFluid(ActorIndex index) const 
{ 
	if(!m_carrying.at(index).exists() || !m_carrying.at(index).isItem())
		return false;
	ItemIndex item = m_carrying.at(index).get();
	Items& items = m_area.getItems();
	return items.getItemType(item).canHoldFluids && !items.cargo_exists(item); 
}
Mass Actors::canPickUp_getMass(ActorIndex index) const
{
	if(!m_carrying.at(index).exists())
	{
		constexpr static Mass zero = 0;
		return zero;
	}
	return m_carrying.at(index).getMass(m_area);
}
Speed Actors::canPickUp_speedIfCarryingQuantity(ActorIndex index, Mass mass, Quantity quantity) const
{
	assert(!m_carrying.at(index).exists());
	return move_getIndividualSpeedWithAddedMass(index, quantity * mass);
}
Quantity Actors::canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(ActorIndex index, Mass mass, Speed minimumSpeed) const
{
	assert(minimumSpeed != 0);
	//TODO: replace iteration with calculation?
	Quantity quantity = 0;
	while(canPickUp_speedIfCarryingQuantity(index, mass, quantity + 1) >= minimumSpeed)
		quantity++;
	return quantity;
}
