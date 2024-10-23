#include "actorOrItemIndex.h"
#include "actors.h"
#include "../items/items.h"
#include "../area.h"
#include "../itemType.h"
#include "index.h"
#include "moveType.h"
#include "sleep.h"
#include "types.h"
void Actors::canPickUp_pickUpItem(const ActorIndex& index, const ItemIndex& item)
{
	canPickUp_pickUpItemQuantity(index, item, Quantity::create(1));
}
void Actors::canPickUp_pickUpItemQuantity(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity)
{
	Items& items = m_area.getItems();
	assert(quantity <= items.getQuantity(item));
	assert(quantity == 1 || ItemType::getGeneric(items.getItemType(item)));
	items.reservable_maybeUnreserve(item, *m_canReserve[index]);
	if(quantity == items.getQuantity(item))
	{
		m_carrying[index] = item.toActorOrItemIndex();
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
		m_carrying[index] = ActorOrItemIndex::createForItem(newItem);
	}
	move_updateIndividualSpeed(index);
}
void Actors::canPickUp_pickUpActor(const ActorIndex& index, const ActorIndex& other)
{
	assert(!m_mustSleep[other]->isAwake() || !move_canMove(other));
	assert(m_carrying[index].exists());
	m_reservables[other]->maybeClearReservationFor(*m_canReserve[index]);
	if(m_location[other].exists())
		exit(other);
	m_carrying[index] = ActorOrItemIndex::createForActor(other);
	move_updateIndividualSpeed(index);
}
void Actors::canPickUp_pickUpPolymorphic(const ActorIndex& index, ActorOrItemIndex actorOrItemIndex, const Quantity& quantity)
{
	assert(actorOrItemIndex.exists());
	if(actorOrItemIndex.isActor())
		canPickUp_pickUpActor(index, actorOrItemIndex.getActor());
	else
		canPickUp_pickUpItemQuantity(index, actorOrItemIndex.getItem(), quantity);
}
ActorIndex Actors::canPickUp_tryToPutDownActor(const ActorIndex& index, const BlockIndex& location, const DistanceInBlocks maxRange)
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isActor());
	ActorIndex other = m_carrying[index].getActor();
	Blocks& blocks = m_area.getBlocks();
	ShapeId shape = m_shape[other];
	auto predicate = [&](const BlockIndex& block) { return blocks.shape_canEnterCurrentlyWithAnyFacing(block, shape, {}); };
	BlockIndex location2 = blocks.getBlockInRangeWithCondition(location, maxRange, predicate);
	if(location2.empty())
		return ActorIndex::null();
	m_carrying[index].clear();
	move_updateIndividualSpeed(index);
	setLocation(other, location2);
	return other;
}
ItemIndex Actors::canPickUp_tryToPutDownItem(const ActorIndex& index, const BlockIndex& location, const DistanceInBlocks maxRange)
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isItem());
	ItemIndex item = m_carrying[index].getItem();
	Blocks& blocks = m_area.getBlocks();
	ShapeId shape = m_area.getItems().getShape(item);
	static const MoveTypeId& moveType = MoveType::byName("none");
	auto predicate = [&](const BlockIndex& block) {
		return blocks.shape_anythingCanEnterEver(block) && 
			blocks.shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithAnyFacing(block, shape, moveType, {});
	};
	BlockIndex targetLocation = blocks.getBlockInRangeWithCondition(location, maxRange, predicate);
	if(targetLocation.empty())
		return ItemIndex::null();
	m_carrying[index].clear();
	move_updateIndividualSpeed(index);
	return m_area.getItems().setLocationAndFacing(item, targetLocation, Facing::create(0));
}
ActorOrItemIndex Actors::canPickUp_tryToPutDownIfAny(const ActorIndex& index, const BlockIndex& location, const DistanceInBlocks maxRange)
{
	if(!m_carrying[index].exists())
		return m_carrying[index];
	return canPickUp_tryToPutDownPolymorphic(index, location, maxRange);
}
ActorOrItemIndex Actors::canPickUp_tryToPutDownPolymorphic(const ActorIndex& index, const BlockIndex& location, const DistanceInBlocks maxRange)
{
	assert(m_carrying[index].exists());
	if(m_carrying[index].isActor())
		return ActorOrItemIndex::createForActor(canPickUp_tryToPutDownActor(index, location, maxRange));
	else
		return ActorOrItemIndex::createForItem(canPickUp_tryToPutDownItem(index, location, maxRange));
}
void Actors::canPickUp_removeFluidVolume(const ActorIndex& index, const CollisionVolume& volume)
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isItem());
	ItemIndex item = m_carrying[index].getItem();
	m_area.getItems().cargo_removeFluid(item, volume);
}
void Actors::canPickUp_add(const ActorIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	Items& items = m_area.getItems();
	auto& carrying = m_carrying[index];
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
void Actors::canPickUp_removeItem(const ActorIndex& index, const ItemIndex& item)
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isItem());
	assert(m_carrying[index].get().toItem() == item);
	m_carrying[index].clear();
	Items& items = m_area.getItems();
	items.destroy(item);
	move_updateIndividualSpeed(index);
}
ItemIndex Actors::canPickUp_getItem(const ActorIndex& index) const
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isItem());
	return m_carrying[index].getItem();
}
ActorIndex Actors::canPickUp_getActor(const ActorIndex& index) const
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isActor());
	return m_carrying[index].getActor();
}
ActorOrItemIndex Actors::canPickUp_getPolymorphic(const ActorIndex& index) const
{
	return m_carrying[index];
}
Quantity Actors::canPickUp_quantityWhichCanBePickedUpUnencombered(const ActorIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType) const
{
	return canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(index, ItemType::getVolume(itemType) * MaterialType::getDensity(materialType), Config::minimumHaulSpeedInital);
}
bool Actors::canPickUp_polymorphic(const ActorIndex& index, ActorOrItemIndex actorOrItemIndex) const
{
	Mass mass = actorOrItemIndex.getSingleUnitMass(m_area);
	return canPickUp_anyWithMass(index, mass);
}
bool Actors::canPickUp_item(const ActorIndex& index, const ItemIndex& item) const
{
	Mass mass = m_area.getItems().getMass(item);
	return canPickUp_anyWithMass(index, mass);
}
bool Actors::canPickUp_itemQuantity(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity) const
{
	Mass mass = m_area.getItems().getMass(item) * quantity;
	return canPickUp_anyWithMass(index, mass);
}
bool Actors::canPickUp_singleItem(const ActorIndex& index, const ItemIndex& item) const
{
	Mass mass = m_area.getItems().getSingleUnitMass(item);
	return canPickUp_anyWithMass(index, mass);
}
bool Actors::canPickUp_actor(const ActorIndex& index, const ActorIndex& actor) const
{
	Mass mass = m_area.getActors().getMass(actor);
	return canPickUp_anyWithMass(index, mass);
}
bool Actors::canPickUp_anyWithMass(const ActorIndex& index, const Mass& mass) const
{
	return canPickUp_speedIfCarryingQuantity(index, mass, Quantity::create(1)) > 0;
}
bool Actors::canPickUp_polymorphicUnencombered(const ActorIndex& index, ActorOrItemIndex actorOrItemIndex) const
{
	Mass mass = actorOrItemIndex.getSingleUnitMass(m_area);
	return canPickUp_anyWithMassUnencombered(index, mass);
}
bool Actors::canPickUp_itemUnencombered(const ActorIndex& index, const ItemIndex& item) const
{
	Mass mass = m_area.getItems().getSingleUnitMass(item);
	return canPickUp_anyWithMassUnencombered(index, mass);
}
bool Actors::canPickUp_actorUnencombered(const ActorIndex& index, const ActorIndex& actor) const
{
	Mass mass = m_area.getActors().getMass(actor);
	return canPickUp_anyWithMassUnencombered(index, mass);
}
bool Actors::canPickUp_anyWithMassUnencombered(const ActorIndex& index, const Mass& mass) const
{
	return canPickUp_speedIfCarryingQuantity(index, mass, Quantity::create(0)) == move_getIndividualSpeedWithAddedMass(index, Mass::create(0));
}
bool Actors::canPickUp_isCarryingItemGeneric(const ActorIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity) const
{
	if(!m_carrying[index].exists() || !m_carrying[index].isItem())
		return false;
	ItemIndex item = m_carrying[index].getItem();
	Items& items = m_area.getItems();
	if(items.getItemType(item) != itemType)
		return false;
	if(items.getMaterialType(item) != materialType)
		return false;
	return items.getQuantity(item) >= quantity;
}
bool Actors::canPickUp_isCarryingFluidType(const ActorIndex& index, FluidTypeId fluidType) const 
{
	if(!m_carrying[index].exists())
		return false;
	ItemIndex item = m_carrying[index].getItem();
	Items& items = m_area.getItems();
	return items.cargo_containsFluidType(item, fluidType);
}
bool Actors::canPickUp_isCarryingPolymorphic(const ActorIndex& index, ActorOrItemIndex actorOrItemIndex) const
{
	if(actorOrItemIndex.isActor())
		return canPickUp_isCarryingActor(index, actorOrItemIndex.getActor());
	else
		return canPickUp_isCarryingItem(index, actorOrItemIndex.getItem());
}
CollisionVolume Actors::canPickUp_getFluidVolume(const ActorIndex& index) const 
{ 
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isItem());
	return m_area.getItems().cargo_getFluidVolume(m_carrying[index].getItem());
}
FluidTypeId Actors::canPickUp_getFluidType(const ActorIndex& index) const 
{ 
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isItem());
	ItemIndex item = m_carrying[index].getItem();
	Items& items = m_area.getItems();
	assert(items.cargo_containsAnyFluid(item));
	return items.cargo_getFluidType(item); 
}
bool Actors::canPickUp_isCarryingEmptyContainerWhichCanHoldFluid(const ActorIndex& index) const 
{ 
	if(!m_carrying[index].exists() || !m_carrying[index].isItem())
		return false;
	ItemIndex item = m_carrying[index].getItem();
	Items& items = m_area.getItems();
	return ItemType::getCanHoldFluids(items.getItemType(item)) && !items.cargo_exists(item); 
}
Mass Actors::canPickUp_getMass(const ActorIndex& index) const
{
	if(!m_carrying[index].exists())
		return Mass::create(0);
	return m_carrying[index].getMass(m_area);
}
Speed Actors::canPickUp_speedIfCarryingQuantity(const ActorIndex& index, const Mass& mass, const Quantity& quantity) const
{
	assert(!m_carrying[index].exists());
	return move_getIndividualSpeedWithAddedMass(index, mass * quantity);
}
Quantity Actors::canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(const ActorIndex& index, const Mass& mass, Speed minimumSpeed) const
{
	assert(minimumSpeed != 0);
	//TODO: replace iteration with calculation?
	Quantity quantity = Quantity::create(0);
	while(canPickUp_speedIfCarryingQuantity(index, mass, quantity + 1) >= minimumSpeed)
		++quantity;
	return quantity;
}		
bool Actors::canPickUp_canPutDown(const ActorIndex& index, const BlockIndex& block)
{
	Blocks& blocks = m_area.getBlocks();
	auto& carrying = m_carrying[index];
	return blocks.shape_canEnterCurrentlyWithAnyFacing(block, carrying.getShape(m_area), {});
}
void Actors::canPickUp_updateActorIndex(const ActorIndex& index, const ActorIndex& oldIndex, const ActorIndex& newIndex)
{
	assert(m_carrying[index].get().toActor() == oldIndex);
	assert(m_carrying[index].isActor());
	m_carrying[index].updateIndex(newIndex);
}
void Actors::canPickUp_updateItemIndex(const ActorIndex& index, const ItemIndex& oldIndex, const ItemIndex& newIndex)
{
	assert(m_carrying[index].isItem());
	assert(m_carrying[index].getItem() == oldIndex);
	m_carrying[index].updateIndex(newIndex);
}
void Actors::canPickUp_updateUnencomberedCarryMass(const ActorIndex& index)
{
	m_unencomberedCarryMass[index] = Mass::create(Config::unitsOfCarryMassPerUnitOfStrength * getStrength(index).get());
}
ActorOrItemIndex Actors::canPickUp_getCarrying(const ActorIndex& index) const
{
	return m_carrying[index];
}
