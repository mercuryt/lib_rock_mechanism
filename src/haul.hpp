#include "actor.h"
#include "block.h"
#include "area.h"
template<class ToHaul>
void HaulSubproject<ToHaul>::commandWorker(Actor& actor)
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
						++m_inplaceCount;
						if(m_inplaceCount == m_workers.size())
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
template<class ToHaul>
void HaulSubproject<ToHaul>::addWorker(Actor& actor)
{
	assert(!m_workers.contains(&actor));
	m_workers.insert(&actor);
	commandWorker(actor);
}
template<class ToHaul>
bool HaulSubproject<ToHaul>::tryToSetHaulStrategy(Actor& actor, std::unordered_set<Actor*>& waiting)
{
	assert(m_strategy == HaulStrategy::None);
	if(getSpeed(actor, m_toHaul) >= m_minimumHaulSpeed)
	{
		m_strategy = HaulStrategy::Individual;
		addWorker(actor);
		return true;
	}
	if(waiting.size() != 0)
	{
		std::vector<Actor*> waitingActorsToAdd = actorsNeededToHaulAtMinimumSpeed(actor, m_toHaul, waiting);
		if(!waitingActorsToAdd.empty())
		{
			m_strategy = HaulStrategy::Team;
			addWorker(actor);
			for(Actor* actor : waitingActorsToAdd)
			{
				addWorker(actor);
				waiting.erase(actor);
			}
			return true;
		}
	}
	Item* haulTool = actor.m_location->m_area->m_hasHaulTools.hasToolToHaul(m_toHaul);
	if(haulTool != nullptr)
	{
		// TODO: make exception for slow haul if very close.
		if(getSpeedWithHaulToolAndCargo(actor, haulTool, m_toHaul) >= m_minimumHaulSpeed)
		{
			m_strategy = HaulStrategy::Cart;
			m_haulTool = haulTool;
			addWorker(actor);
			return true;
		}
		Actor* yoked = actor.m_location->m_area->m_hasHaulTools.getActorToYokeForHaulTool(*haulTool);
		if(yoked != nullptr && getSpeedWithHaulToolAndYokedAndCargo(haulTool, yoked, m_toHaul) >= m_minimumHaulSpeed)
		{
			m_strategy = HaulStrategy::AnimalCart;
			m_haulTool = haulTool;
			m_beastOfBurden = yoked;
			addWorker(actor);
			return true;
		}
		std::vector<Actor*> waitingActorsToAdd = actorsNeededToHaulAtMinimumSpeedWithTool(actor, m_toHaul, haulTool, waiting);
		if(!waitingActorsToAdd.empty())
		{
			m_strategy = HaulStrategy::TeamCart;
			m_haulTool = haulTool;
			addWorker(actor);
			for(Actor* actor : waitingActorsToAdd)
			{
				addWorker(actor);
				waiting.erase(actor);
			}
			return true;
		}
	}
	Actor* pannierBearer = actor.m_location->m_area->m_hasHaulTools.getPannierBearerToHaul(m_toHaul);
	if(pannierBearer != nullptr)
	{
		Item* panniers = actor.m_location->m_area->m_hasHaulTools.getPanniersForActor(*pannierBearer);
		if(panniers != nullptr && getSpeedWithPannierBearer(m_toHaul, pannierBearer) >= m_minimumHaulSpeed)
		{
			m_strategy = HaulStrategy::Panniers;
			m_beastOfBurden = pannierBearer;
			m_haulTool = panniers;
			addWorker(actor);
		}
	}
}
