#include "actorOrItemIndex.h"
#include "actors.h"
#include "../items/items.h"
#include "../area/area.h"
#include "../definitions/itemType.h"
#include "../portables.hpp"
#include "numericTypes/index.h"
#include "definitions/moveType.h"
#include "sleep.h"
#include "numericTypes/types.h"
void Actors::canPickUp_pickUpItem(const ActorIndex& index, const ItemIndex& item)
{
	canPickUp_pickUpItemQuantity(index, item, Quantity::create(1));
}
void Actors::canPickUp_pickUpItemQuantity(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity)
{
	Items& items = m_area.getItems();
	assert(quantity <= items.getQuantity(item));
	assert(quantity == 1 || ItemType::getGeneric(items.getItemType(item)));
	assert(m_carrying[index].empty());
	items.reservable_maybeUnreserve(item, *m_canReserve[index]);
	assert(items.isStatic(item));
	if(quantity == items.getQuantity(item))
	{
		m_carrying[index] = item.toActorOrItemIndex();
		if(items.getLocation(item).exists())
			items.location_clear(item);
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
	items.setCarrier(m_carrying[index].getItem(), ActorOrItemIndex::createForActor(index));
	move_updateIndividualSpeed(index);
}
void Actors::canPickUp_pickUpActor(const ActorIndex& index, const ActorIndex& other)
{
	assert(m_carrying[index].empty());
	m_reservables[other]->maybeClearReservationFor(*m_canReserve[index]);
	if(m_location[other].exists())
		location_clear(other);
	m_carrying[index] = ActorOrItemIndex::createForActor(other);
	setCarrier(other, ActorOrItemIndex::createForActor(index));
	move_updateIndividualSpeed(index);
}
ActorOrItemIndex Actors::canPickUp_pickUpPolymorphic(const ActorIndex& index, ActorOrItemIndex actorOrItemIndex, const Quantity& quantity)
{
	assert(actorOrItemIndex.exists());
	if(actorOrItemIndex.isActor())
	{
		canPickUp_pickUpActor(index, actorOrItemIndex.getActor());
		return actorOrItemIndex;
	}
	else
	{
		canPickUp_pickUpItemQuantity(index, actorOrItemIndex.getItem(), quantity);
		return canPickUp_getPolymorphic(index);
	}
}
ActorIndex Actors::canPickUp_tryToPutDownActor(const ActorIndex& index, const Point3D& location, const Distance maxRange)
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isActor());
	ActorIndex other = m_carrying[index].getActor();
	assert(m_static[other]);
	Space& space = m_area.getSpace();
	ShapeId shape = m_shape[other];
	auto predicate = [&](const Point3D& point) {
		return space.shape_anythingCanEnterEver(point) &&
			space.shape_staticShapeCanEnterWithAnyFacing(point, shape, {});
	};
	Point3D targetLocation = space.getPointInRangeWithCondition(location, maxRange, predicate);
	if(targetLocation.empty())
	{
		static const MoveTypeId moveTypeNone = MoveType::byName("none");
		// No location found, try again without respecting Config::maxPointVolume.
		auto predicate2 = [&](const Point3D& point)
		{
			return space.shape_anythingCanEnterEver(point) &&
				   space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(point, shape, moveTypeNone);
		};
		targetLocation = space.getPointInRangeWithCondition(location, maxRange, predicate2);
	}
	if(targetLocation.empty())
		// There is nowhere to put down then actor even ignoring max point volume, i.e. it is a multi point shape which is interfearing with unenterable space.
		return ActorIndex::null();
	m_carrying[index].clear();
	unsetCarrier(other, ActorOrItemIndex::createForActor(index));
	move_updateIndividualSpeed(index);
	const Facing4& facing = space.shape_canEnterCurrentlyWithAnyFacingReturnFacing(targetLocation, shape, m_occupied[index]);
	location_set(other, targetLocation, facing);
	return other;
}
ItemIndex Actors::canPickUp_tryToPutDownItem(const ActorIndex& index, const Point3D& location, const Distance maxRange)
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isItem());
	ItemIndex item = m_carrying[index].getItem();
	Items& items = m_area.getItems();
	assert(items.isStatic(item));
	Space& space = m_area.getSpace();
	ShapeId shape = m_area.getItems().getShape(item);
	// Find a location to put down the carried item.
	auto predicate = [&](const Point3D& point) {
		return space.shape_anythingCanEnterEver(point) &&
			space.shape_staticShapeCanEnterWithAnyFacing(point, shape, {});
	};
	Point3D targetLocation = space.getPointInRangeWithCondition(location, maxRange, predicate);
	if(targetLocation.empty())
	{
		static const MoveTypeId moveTypeNone = MoveType::byName("none");
		// No location found, try again without respecting Config::maxPointVolume.
		auto predicate2 = [&](const Point3D &point)
		{
			return space.shape_anythingCanEnterEver(point) &&
				space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(point, shape, moveTypeNone);
		};
		targetLocation = space.getPointInRangeWithCondition(location, maxRange, predicate2);
	}
	if(targetLocation.empty())
		// There is nowhere to put down then item even ignoring max point volume, i.e. it is a multi point shape which is interfearing with unenterable space.
		return ItemIndex::null();
	m_carrying[index].clear();
	items.unsetCarrier(item, ActorOrItemIndex::createForActor(index));
	move_updateIndividualSpeed(index);
	return m_area.getItems().location_set(item, targetLocation, Facing4::North);
}
ActorOrItemIndex Actors::canPickUp_tryToPutDownIfAny(const ActorIndex& index, const Point3D& location, const Distance maxRange)
{
	if(!m_carrying[index].exists())
		return m_carrying[index];
	return canPickUp_tryToPutDownPolymorphic(index, location, maxRange);
}
ActorOrItemIndex Actors::canPickUp_tryToPutDownPolymorphic(const ActorIndex& index, const Point3D& location, const Distance maxRange)
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
		items.setCarrier(item, ActorOrItemIndex::createForActor(index));
	}
	move_updateIndividualSpeed(index);
}
void Actors::canPickUp_removeItem(const ActorIndex& index, [[maybe_unused]] const ItemIndex& item)
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isItem());
	assert(m_carrying[index].get().toItem() == item);
	m_area.getItems().unsetCarrier(m_carrying[index].getItem(), ActorOrItemIndex::createForActor(index));
	m_carrying[index].clear();
	move_updateIndividualSpeed(index);
}
void Actors::canPickUp_removeActor(const ActorIndex& index, [[maybe_unused]] const ActorIndex& actor)
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index].isActor());
	assert(m_carrying[index].get().toActor() == actor);
	m_area.getActors().unsetCarrier(m_carrying[index].getActor(), ActorOrItemIndex::createForActor(index));
	m_carrying[index].clear();
	move_updateIndividualSpeed(index);
}
void Actors::canPickUp_remove(const ActorIndex& index, [[maybe_unused]] const ActorOrItemIndex& actorOrItem)
{
	assert(m_carrying[index].exists());
	assert(m_carrying[index] == actorOrItem);
	if(m_carrying[index].isActor())
		canPickUp_removeActor(index, m_carrying[index].getActor());
	else
		canPickUp_removeItem(index, m_carrying[index].getItem());
}
void Actors::canPickUp_destroyItem(const ActorIndex& index, const ItemIndex& item)
{
	canPickUp_removeItem(index, item);
	m_area.getItems().destroy(item);
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
	return canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(index, ItemType::getFullDisplacement(itemType) * MaterialType::getDensity(materialType), Config::minimumHaulSpeedInital);
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
bool Actors::canPickUp_canPutDown(const ActorIndex& index, const Point3D& point)
{
	Space& space = m_area.getSpace();
	auto& carrying = m_carrying[index];
	return space.shape_canEnterCurrentlyWithAnyFacing(point, carrying.getShape(m_area), {});
}
void Actors::canPickUp_updateActorIndex(const ActorIndex& index, [[maybe_unused]] const ActorIndex& oldIndex, const ActorIndex& newIndex)
{
	assert(oldIndex != newIndex);
	assert(m_carrying[index].isActor());
	assert(m_carrying[index].get().toActor() == oldIndex);
	m_carrying[index].updateIndex(newIndex);
}
void Actors::canPickUp_updateItemIndex(const ActorIndex& index, [[maybe_unused]] const ItemIndex& oldIndex, const ItemIndex& newIndex)
{
	assert(oldIndex != newIndex);
	assert(m_carrying[index].isItem());
	assert(m_carrying[index].getItem() == oldIndex);
	m_carrying[index].updateIndex(newIndex);
}
void Actors::canPickUp_updateUnencomberedCarryMass(const ActorIndex& index)
{
	m_unencomberedCarryMass[index] = Mass::create(Config::unitsOfCarryMassPerUnitOfStrength * getStrength(index).get());
}
void Actors::canPickUp_addFluidToContainerFromAdjacentPointsIncludingOtherContainersWithLimit(const ActorIndex& index, const FluidTypeId& fluidType, const CollisionVolume& volume)
{
	Space& space = m_area.getSpace();
	Items& items = m_area.getItems();
	const ItemIndex container = m_carrying[index].getItem();
	CollisionVolume capacity = volume;
	if(items.cargo_containsFluidType(container, fluidType))
		capacity -= items.cargo_getFluidVolume(container);
	else
		assert(!items.cargo_containsAnyFluid(container));
	for(const Point3D& point : getAdjacentPoints(index))
	{
		if(space.fluid_contains(point, fluidType))
		{
			const CollisionVolume avalible = space.fluid_volumeOfTypeContains(point, fluidType);
			const CollisionVolume volumeToTransfer = std::min(capacity, avalible);
			assert(volumeToTransfer != 0);
			space.fluid_remove(point, volumeToTransfer, fluidType);
			items.cargo_addFluid(container, fluidType, volumeToTransfer);
			capacity -= volumeToTransfer;
			if(capacity == 0)
				break;
		}
		for(const ItemIndex& otherContainer : space.item_getAll(point))
		{
			if(items.cargo_containsFluidType(otherContainer, fluidType))
			{
				const CollisionVolume avalible = items.cargo_getFluidVolume(otherContainer);
				const CollisionVolume volumeToTransfer = std::min(capacity, avalible);
				assert(volumeToTransfer != 0);
				items.cargo_removeFluid(otherContainer, volumeToTransfer);
				items.cargo_addFluid(container, fluidType, volumeToTransfer);
				capacity -= volumeToTransfer;
				if(capacity == 0)
					break;
			}
		}
		if(capacity == 0)
			break;
	}
}
ActorOrItemIndex Actors::canPickUp_getCarrying(const ActorIndex& index) const
{
	return m_carrying[index];
}
