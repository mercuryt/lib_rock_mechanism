#include "haul.h"
#include "haul.hpp"
#include "actor.h"
#include "item.h"
#include <unordered_set>
void CanPickup::pickUp(HasShape& hasShape, uint32_t quantity)
{
	if(hasShape.isItem())
		pickUp(static_cast<Item&>(hasShape), quantity);
	else
	{
		assert(hasShape.isActor());
		pickUp(static_cast<Actor&>(hasShape), quantity);
	}
}
void CanPickup::pickUp(Item& item, uint32_t quantity)
{
	assert(quantity <= item.m_quantity);
	assert(quantity == 1 || item.m_itemType.generic);
	item.m_reservable.clearReservationFor(m_actor.m_canReserve);
	if(quantity == item.m_quantity)
	{
		m_carrying = &item;
		item.exit();
	}
	else
	{
		item.m_quantity -= quantity;
		Item& newItem = Item::create(*m_actor.m_location->m_area, item.m_itemType, item.m_materialType, quantity);
		m_carrying = &newItem;
	}
	m_actor.m_canMove.updateIndividualSpeed();
}
void CanPickup::pickUp(Actor& actor, uint32_t quantity = 1)
{
	assert(quantity == 1);
	assert(!actor.isAwake() || actor.isInjured());
	assert(m_carrying = nullptr);
	actor.m_reservable.clearReservationFor(m_actor.m_canReserve);
	actor.exit();
	m_carrying = &actor;
	m_actor.m_canMove.updateIndividualSpeed();
}
void CanPickup::putDown(Block& location)
{
	assert(m_carrying != nullptr);
	m_carrying->setLocation(location);
	m_carrying = nullptr;
	m_actor.m_canMove.updateIndividualSpeed();
}
void CanPickup::putDownIfAny(Block& location)
{
	if(m_carrying != nullptr)
		putDown(location);
}
void CanPickup::add(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity)
{
	if(m_carrying == nullptr)
		m_carrying = &Item::create(*m_actor.m_location->m_area, itemType, materialType, quantity);
	else
	{
		assert(m_carrying->isItem());	
		Item& item = static_cast<Item&>(*m_carrying);
		assert(item.m_itemType.generic && item.m_itemType == itemType && item.m_materialType == materialType);
		item.m_quantity += quantity;
	}
}
void CanPickup::remove(Item& item)
{
	assert(m_carrying == &item);
	m_carrying = nullptr;
}
uint32_t CanPickup::canPickupQuantityOf(Item& item) const { return canPickupQuantityOf(item.m_itemType, item.m_materialType); }
uint32_t CanPickup::canPickupQuantityOf(const ItemType& itemType, const MaterialType& materialType) const
{
	uint32_t max = m_actor.m_attributes.getUnencomberedCarryMass();
	uint32_t current = (m_actor.m_equipmentSet.getMass() + m_actor.m_canPickup.getMass());
	return (max - current) / itemType.volume * (materialType.density);
}
bool CanPickup::isCarryingFluidType(const FluidType& fluidType) const 
{
	Item& item = static_cast<Item&>(*m_carrying);
       	return item.m_hasCargo.getFluidType() == fluidType; 
}
const uint32_t& CanPickup::getFluidVolume() const 
{ 
	Item& item = static_cast<Item&>(*m_carrying);
	return item.m_hasCargo.getFluidVolume(); 
}
const FluidType& CanPickup::getFluidType() const 
{ 
	Item& item = static_cast<Item&>(*m_carrying);
	return item.m_hasCargo.getFluidType(); 
}
bool CanPickup::isCarryingEmptyContainerWhichCanHoldFluid() const 
{ 
	Item& item = static_cast<Item&>(*m_carrying);
	return item.m_hasCargo.containsAnyFluid() && item.m_itemType.canHoldFluids; 
}
uint32_t CanPickup::getMass() const
{
	if(m_carrying == nullptr)
	{
		constexpr static uint32_t zero = 0;
		return zero;
	}
	return m_carrying->getMass();
}
HaulSubproject::HaulSubproject(Project& p, HaulSubprojectParamaters& paramaters) : Subproject(p), m_toHaul(*paramaters.toHaul), m_quantity(paramaters.quantity), m_destination(*paramaters.destination), m_strategy(paramaters.strategy), m_haulTool(paramaters.haulTool), m_leader(nullptr), m_itemIsMoving(false), m_beastOfBurden(paramaters.beastOfBurden), m_teamMemberInPlaceForHaulCount(0)
{
	for(Actor* actor : m_workers)
		commandWorker(*actor);
}
