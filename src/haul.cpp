#include "haul.h"
#include "actor.h"
#include "item.h"
#include "area.h"
#include <unordered_set>
// Check that the haul strategy choosen during read step is still valid in write step.
bool HaulSubprojectParamaters::validate() const
{
	assert(toHaul != nullptr);
	assert(quantity != 0);
	assert(destination != nullptr);
	assert(strategy != HaulStrategy::None);
	assert(toHaulLocation != nullptr);
	for(Actor* worker : workers)
		if(worker->m_reservable.isFullyReserved())
			return false;
	if(haulTool != nullptr && haulTool->m_reservable.isFullyReserved())
		return false;
	if(beastOfBurden != nullptr && beastOfBurden->m_reservable.isFullyReserved())
		return false;
	return true;
}
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
uint32_t CanPickup::canPickupQuantityOf(HasShape& hasShape) const
{
	if(hasShape.isItem())
		return canPickupQuantityOf(static_cast<Item&>(hasShape));
	else
	{
		assert(hasShape.isActor());
		return canPickupQuantityOf(static_cast<Actor&>(hasShape));
	}
}
uint32_t CanPickup::canPickupQuantityOf(Actor& actor) const
{ 
	return canPickupQuantityWithSingeUnitMass(actor.getMass());
}
uint32_t CanPickup::canPickupQuantityOf(Item& item) const { return canPickupQuantityOf(item.m_itemType, item.m_materialType); }
uint32_t CanPickup::canPickupQuantityOf(const ItemType& itemType, const MaterialType& materialType) const
{
	return canPickupQuantityWithSingeUnitMass(itemType.volume * materialType.density);
}
uint32_t CanPickup::canPickupQuantityWithSingeUnitMass(uint32_t unitMass) const
{
	uint32_t max = m_actor.m_attributes.getUnencomberedCarryMass();
	uint32_t current = (m_actor.m_equipmentSet.getMass() + m_actor.m_canPickup.getMass());
	return (max - current) / unitMass;
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
uint32_t CanPickup::speedIfCarryingAny(HasShape& hasShape) const
{
	assert(m_carrying == nullptr);
	return m_actor.m_canMove.getIndividualMoveSpeedWithAddedMass(hasShape.singleUnitMass());
}
HaulSubproject::HaulSubproject(Project& p, HaulSubprojectParamaters& paramaters) : Subproject(p), m_toHaul(*paramaters.toHaul), m_quantity(paramaters.quantity), m_destination(*paramaters.destination), m_strategy(paramaters.strategy), m_haulTool(paramaters.haulTool), m_leader(nullptr), m_itemIsMoving(false), m_beastOfBurden(paramaters.beastOfBurden), m_teamMemberInPlaceForHaulCount(0)
{
	for(Actor* actor : m_workers)
		commandWorker(*actor);
}
void HaulSubproject::commandWorker(Actor& actor)
{
	assert(m_workers.contains(&actor));
	switch(m_strategy)
	{
		case HaulStrategy::Individual:
			assert(m_workers.size() == 1);
			if(actor.m_canPickup.isCarrying(m_toHaul))
			{
				if(actor.isAdjacentTo(m_destination))
				{
					actor.m_canPickup.putDown(m_destination);
					onComplete();
				}
				else
					actor.m_canMove.setDestinationAdjacentTo(m_destination);
			}
			else
			{
				if(actor.isAdjacentTo(m_toHaul))
					actor.m_canPickup.pickUp(m_toHaul, m_quantity);
				else
					actor.m_canMove.setDestinationAdjacentTo(m_toHaul);
			}
			break;
		case HaulStrategy::Team:
			// TODO: teams of more then 2, requires muliple followers.
			assert(m_workers.size() == 2);
			if(m_leader == nullptr)
				m_leader = &actor;
			if(m_itemIsMoving)
			{
				assert(&actor == m_leader);
				if(actor.m_location == &m_destination)
				{
					// At destination.
					m_toHaul.m_canFollow.unfollow();
					for(Actor* actor : m_workers)
						if(actor != m_leader)
							actor->m_canFollow.unfollow();
					m_toHaul.setLocation(m_destination);
					onComplete();
				}
				else
				{
					actor.m_canMove.setDestination(m_destination);
				}
			}
			else 
			{
				// Item is not moving.
				if(m_liftPoints.contains(&actor))
				{
					// A lift point exists.
					if(actor.m_location == m_liftPoints.at(&actor))
					{
						// Actor is at lift point.
						++m_teamMemberInPlaceForHaulCount;
						if(m_teamMemberInPlaceForHaulCount == m_workers.size())
						{
							// All actors are at lift points.
							m_toHaul.m_canFollow.follow(m_leader->m_canLead);
							for(Actor* follower : m_workers)
								if(follower != m_leader)
									follower->m_canFollow.follow(m_toHaul.m_canLead);
							m_leader->m_canMove.setDestination(m_destination);
							m_itemIsMoving = true;
						}
						m_liftPoints.at(&actor)->m_reservable.clearReservationFor(actor.m_canReserve);
					}
					else
						// Actor is not at lift point.
						actor.m_canMove.setDestination(*m_liftPoints.at(&actor));
				}
				else
				{
					// No lift point exists for this actor, find one.
					for(Block* block : m_toHaul.getAdjacentBlocks())
						// TODO: support multi block actors.
						if(block->m_hasShapes.canEnterEverWithFacing(actor, 0) && !block->m_reservable.isFullyReserved())
						{
							m_liftPoints[&actor] = block;
							block->m_reservable.reserveFor(actor.m_canReserve, 1);
							actor.m_canMove.setDestination(*block);
						}
					assert(false); // team strategy should not be choosen if there is not enough space to reserve for lifting.
				}
			}
			break;
			//TODO: Reserve destinations?
		case HaulStrategy::Cart:
			assert(m_haulTool != nullptr);
			if(actor.m_canLead.isLeading(*m_haulTool))
			{
				if(m_haulTool->m_hasCargo.contains(m_toHaul))
				{
					if(actor.isAdjacentTo(m_destination))
					{
						m_haulTool->m_hasCargo.remove(m_toHaul);
						// TODO: set rotation?
						m_toHaul.setLocation(m_destination);
						m_haulTool->m_canFollow.unfollow();
						m_haulTool->m_reservable.clearReservationFor(actor.m_canReserve);
						onComplete();
					}
					else
						actor.m_canMove.setDestination(m_destination);
				}
				else
				{
					if(actor.isAdjacentTo(m_toHaul))
					{
						//TODO: set delay for loading.
						m_toHaul.exit();
						m_haulTool->m_hasCargo.add(m_toHaul);
						actor.m_canMove.setDestination(m_destination);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul);
				}
			}
			else
			{
				if(actor.isAdjacentTo(*m_haulTool))
				{
					m_haulTool->m_canFollow.follow(actor.m_canLead);
					actor.m_canMove.setDestinationAdjacentTo(m_toHaul);
				}
				else
					actor.m_canMove.setDestinationAdjacentTo(*m_haulTool);

			}
			break;
		case HaulStrategy::Panniers:
			assert(m_beastOfBurden != nullptr);
			assert(m_haulTool != nullptr);
			if(m_beastOfBurden->m_equipmentSet.contains(*m_haulTool))
			{
				// Beast has panniers.
				if(m_haulTool->m_hasCargo.contains(m_toHaul))
				{
					// Panniers have cargo.
					if(actor.m_location == &m_destination)
					{
						// Actor is at destination.
						//TODO: unloading delay.
						m_haulTool->m_hasCargo.remove(m_toHaul);
						// TODO: set rotation?
						m_toHaul.setLocation(m_destination);
						m_beastOfBurden->m_canFollow.unfollow();
						m_beastOfBurden->m_reservable.clearReservationFor(actor.m_canReserve);
						onComplete();
					}
					else
						actor.m_canMove.setDestination(m_destination);
				}
				else
				{
					// Panniers don't have cargo.
					if(actor.isAdjacentTo(m_toHaul))
					{
						// Actor is at pickup location.
						// TODO: loading delay.
						m_toHaul.exit();
						m_haulTool->m_hasCargo.add(m_toHaul);
						actor.m_canMove.setDestinationAdjacentTo(m_destination);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul);
				}
			}
			else
			{
				// Get beast panniers.
				if(actor.m_canPickup.isCarrying(*m_haulTool))
				{
					// Actor has panniers.
					if(actor.isAdjacentTo(*m_beastOfBurden))
					{
						// Actor can put on panniers.
						m_haulTool->exit();
						m_beastOfBurden->m_equipmentSet.addEquipment(*m_haulTool);
						m_beastOfBurden->m_canFollow.follow(actor.m_canLead);
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(*m_beastOfBurden);
				}
				else
				{
					// Bring panniers to beast.
					// TODO: move to adjacent to panniers.
					if(actor.m_location == m_haulTool->m_location)
					{
						actor.m_canPickup.pickUp(*m_haulTool, 1u);
						actor.m_canMove.setDestinationAdjacentTo(*m_beastOfBurden);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(*m_haulTool);
				}

			}
			break;
		case HaulStrategy::AnimalCart:
			assert(m_beastOfBurden != nullptr);
			assert(m_haulTool != nullptr);
			if(m_beastOfBurden->m_canLead.isLeading(*m_haulTool))
			{
				// Beast has cart.
				if(m_haulTool->m_hasCargo.contains(m_toHaul))
				{
					// Cart has cargo.
					if(actor.m_location == &m_destination)
					{
						// Actor is at destination.
						//TODO: unloading delay.
						//TODO: unfollow cart?
						m_haulTool->m_hasCargo.remove(m_toHaul);
						// TODO: set rotation?
						m_toHaul.setLocation(m_destination);
						m_beastOfBurden->m_canFollow.unfollow();
						m_beastOfBurden->m_reservable.clearReservationFor(actor.m_canReserve);
						onComplete();
					}
					else
						actor.m_canMove.setDestination(m_destination);
				}
				else
				{
					// Cart doesn't have cargo.
					if(actor.m_location == m_toHaul.m_location)
					{
						// Actor is at pickup location.
						// TODO: loading delay.
						m_toHaul.exit();
						m_haulTool->m_hasCargo.add(m_toHaul);
						actor.m_canMove.setDestination(m_destination);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul);
				}
			}
			else
			{
				// Bring beast to cart.
				if(actor.m_canLead.isLeading(*m_beastOfBurden))
				{
					// Actor has beast.
					if(actor.isAdjacentTo(*m_haulTool))
					{
						// Actor can harness beast.
						m_haulTool->m_canFollow.follow(m_beastOfBurden->m_canLead);
						m_beastOfBurden->m_canFollow.follow(actor.m_canLead);
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(*m_haulTool);
				}
				else
				{
					// Get beast.
					if(actor.isAdjacentTo(*m_beastOfBurden))
					{
						m_beastOfBurden->m_canFollow.follow(actor.m_canLead);
						actor.m_canMove.setDestinationAdjacentTo(*m_haulTool);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(*m_beastOfBurden);
				}

			}
			break;
		case HaulStrategy::TeamCart:
			//TODO
			break;
		case HaulStrategy::StrongSentient:
			//TODO
			break;
		case HaulStrategy::None:
			assert(false); // this method should only be called after a strategy is choosen.
	}
}
void HaulSubproject::addWorker(Actor& actor)
{
	assert(!m_workers.contains(&actor));
	m_workers.insert(&actor);
	commandWorker(actor);
}
void HaulSubproject::removeWorker(Actor& actor)
{
	assert(m_workers.contains(&actor));
	cancel();
}
void HaulSubproject::cancel()
{
	for(Actor* actor : m_workers)
	{
		actor->m_canPickup.putDownIfAny(*actor->m_location);
		actor->m_hasObjectives.taskComplete();
		actor->m_canFollow.unfollowIfFollowing();
	}
}
HaulSubprojectParamaters HaulSubproject::tryToSetHaulStrategy(Project& project, HasShape& toHaul, Actor& worker)
{
	HaulSubprojectParamaters output;
	// Individual
	if(worker.m_canPickup.speedIfCarryingAny(toHaul) >= project.getMinimumHaulSpeed())
	{
		output.strategy = HaulStrategy::Individual;
		output.quantity = worker.m_canPickup.canPickupQuantityOf(toHaul);
		output.workers.push_back(&worker);
		return output;
	}
	// Team
	if(project.m_waiting.size() != 1)
	{
		assert(!project.m_waiting.empty());
		output.workers = actorsNeededToHaulAtMinimumSpeed(project, worker, toHaul, project.m_waiting);
		if(!output.workers.empty())
		{
			output.strategy = HaulStrategy::Team;
			return output;
		}
	}
	Item* haulTool = toHaul.m_location->m_area->m_hasHaulTools.getToolToHaul(toHaul);
	// Cart
	if(haulTool != nullptr)
	{
		// Cart
		// TODO: make exception for slow haul if very close.
		if(getSpeedWithHaulToolAndCargo(worker, *haulTool, toHaul) >= project.getMinimumHaulSpeed())
		{
			output.strategy = HaulStrategy::Cart;
			output.haulTool = haulTool;
			output.workers.push_back(&worker);
			return output;
		}
		// Animal Cart
		Actor* yoked = toHaul.m_location->m_area->m_hasHaulTools.getActorToYokeForHaulTool(*haulTool);
		if(yoked != nullptr && getSpeedWithHaulToolAndYokedAndCargo(worker, *yoked, *haulTool, toHaul) >= project.getMinimumHaulSpeed())
		{
			output.strategy = HaulStrategy::AnimalCart;
			output.haulTool = haulTool;
			output.beastOfBurden = yoked;
			output.workers.push_back(&worker);
			return output;
		}
		// Team Cart
		output.workers = actorsNeededToHaulAtMinimumSpeedWithTool(project, worker, toHaul, *haulTool, project.m_waiting);
		if(!output.workers.empty())
		{
			output.strategy = HaulStrategy::TeamCart;
			output.haulTool = haulTool;
			return output;
		}
	}
	// Panniers
	Actor* pannierBearer = toHaul.m_location->m_area->m_hasHaulTools.getPannierBearerToHaul(toHaul);
	if(pannierBearer != nullptr)
	{
		Item* panniers = toHaul.m_location->m_area->m_hasHaulTools.getPanniersForActor(*pannierBearer);
		if(panniers != nullptr && getSpeedWithPannierBearerAndPanniers(worker, *pannierBearer, *panniers, toHaul) >= project.getMinimumHaulSpeed())
		{
			output.strategy = HaulStrategy::Panniers;
			output.beastOfBurden = pannierBearer;
			output.haulTool = panniers;
			output.workers.push_back(&worker);
			return output;
		}
	}
	assert(output.strategy == HaulStrategy::None);
	return output;
}
// Class method.
std::vector<Actor*> HaulSubproject::actorsNeededToHaulAtMinimumSpeed(Project& project, Actor& leader, HasShape& toHaul, std::unordered_set<Actor*> waiting)
{
	std::vector<HasShape*> actorsAndItems;
	std::vector<Actor*> output;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&toHaul);
	// For each actor waiting add to actors and items and see if the item can be moved fast enough.
	for(Actor* actor : waiting)
	{
		output.push_back(actor);
		actorsAndItems.push_back(actor);
		uint32_t speed = CanLead::getMoveSpeedForGroup(actorsAndItems);
		if(speed  >= project.getMinimumHaulSpeed())
			return output;
	}
	// All the actors waiting put together are not enough to move at minimum acceptable speed.
	output.clear();
	return output;
}
// Class method.
uint32_t HaulSubproject::getSpeedWithHaulToolAndCargo(Actor& leader, Item& haulTool, HasShape& toHaul)
{
	std::vector<HasShape*> actorsAndItems;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&haulTool);
	// actorsAndItems, rollingMass, deadMass
	return CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, toHaul.getMass(), 0);
}
// Class method.
uint32_t HaulSubproject::getSpeedWithHaulToolAndYokedAndCargo(Actor& leader, Actor& yoked, Item& haulTool, HasShape& toHaul)
{
	std::vector<HasShape*> actorsAndItems;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&yoked);
	actorsAndItems.push_back(&haulTool);
	// actorsAndItems, rollingMass, deadMass
	return CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, toHaul.singleUnitMass(), 0);
}
// Class method.
std::vector<Actor*> HaulSubproject::actorsNeededToHaulAtMinimumSpeedWithTool(Project& project, Actor& leader, HasShape& toHaul, Item& haulTool, std::unordered_set<Actor*> waiting)
{
	assert(haulTool.m_itemType.internalVolume >= toHaul.getVolume());
	std::vector<HasShape*> actorsAndItems;
	std::vector<Actor*> output;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&haulTool);
	// For each actor waiting add to actors and items and see if the item can be moved fast enough.
	for(Actor* actor : waiting)
	{
		output.push_back(actor);
		actorsAndItems.push_back(actor);
		uint32_t speed = CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, toHaul.singleUnitMass(), 0);
		if(speed  >= project.getMinimumHaulSpeed())
			return output;
	}
	// All the actors waiting put together are not enough to move at minimum acceptable speed.
	output.clear();
	return output;
}
// Class method.
uint32_t HaulSubproject::getSpeedWithPannierBearerAndPanniers(Actor& leader, Actor& yoked, Item& haulTool, HasShape& toHaul)
{
	std::vector<HasShape*> actorsAndItems;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&yoked);
	// actorsAndItems, rollingMass, deadMass
	return CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, 0, toHaul.singleUnitMass() + haulTool.getMass());
}
