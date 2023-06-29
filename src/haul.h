enum class HaulStrategy { Individual, Team, Cart, TeamCart, Panniers, AnimalCart, StrongSentient };

class HaulSubproject : Subproject
{
	Item* m_toHaul;
	uint32_t quantity;
	Block& m_destination;
	HaulStrategy m_strategy;
	Block* m_toHaulLocation;
	std::unordered_set<Actor*> m_workers;
	std::unordered_set<Actor> m_nonsentients;
	Item* m_haulTool;
	std::unordered_map<Actor*, Block*> m_liftPoints; // Used by Team strategy.
	uint32_t m_inplaceCount;
	Actor* m_leader;
	bool m_itemIsMoving;
	Actor* m_beastOfBurden;
public:
	HaulSubproject(Item& th, Block& d, HaulStrategy hs) : m_toHaul(th), m_destination(d), m_strategy(hs), m_haulTool(nullptr), m_inplaceCount(0), m_itemIsMoving(false), m_leader(nullptr) { }
	void commandWorker(Actor& actor)
	{
		assert(m_workers.contains(actor));
		switch(m_strategy)
		{
			case HaulStrategy::Individual:
				assert(m_workers.size() == 1);
				if(actor.m_hasItems.m_carrying == &m_toHaul)
				{
					if(actor.m_location == &m_destination)
					{
						actor.m_hasItems.drop();
						onComplete();
					}
					else
						actor.setDestination(m_destination);
				}
				else
				{
					if(actor.m_location == m_toHaul.m_location)
						actor.pickUp(m_toHaul, quantity);
					else
						actor.setDestination(*m_toHaul.m_location);
				}
				break;
			case HaulStrategy::Team:
				// TODO: teams of more then 2, requires muliple followers.
				assert(m_workers.size == 2);
				if(m_leader == nullptr)
					m_leader = &actor;
				if(m_itemIsMoving)
				{
					assert(actor == m_leader);
					if(actor.m_location == &m_location)
					{
						// At destination.
						m_toHaul.m_canFollow.unfollow();
						for(Actor* actor : m_workers)
							if(actor != m_leader)
								actor.m_canFollow.unfollow();
						m_toHaul.m_location->m_hasItems.transferTo(m_destination.m_hasItems);
						onComplete();
					}
					else
					{
						actor.setDestination(m_destination);
					}
				}
				else 
				{
					// Item is not moving.
					if(m_liftPoints.contains(actor))
					{
						// A lift point exists.
						if(actor.m_location == m_liftPoints.at(&actor))
						{
							// Actor is at lift point.
							++m_inplaceCount;
							if(m_inplaceCount == m_workers.size())
							{
								// All actors are at lift points.
								m_toHaul.m_canFollow.follow(m_leader);
								for(Actor* follower : m_workers)
									if(follower != m_leader)
										follower.m_canFollow.follow(m_toHaul);
								m_leader->setDestination(m_destination);
								m_itemIsMoving = true;
							}
							block.m_reservable.unreserve();
						}
						else
							// Actor is not at lift point.
							actor.setDestination(m_liftPoints.at(&actor));
					}
					else
					{
						// No lift point exists for this actor, find one.
						for(Block* block : m_toHaul.m_location->m_adjacentsVector)
							if(block->actorCanEnterEver(actor) && !block->m_reservable.isFullyReserved())
							{
								m_liftPoints[&actor] = block;
								// TODO: support multi block actors.
								block.m_reservable.reserveFor(actor.m_canReserve);
								actor.setDestination(*block);
							}
						assert(false); // team strategy should not be choosen if there is not enough space to reserve for lifting.
					}
				}
				break;
			//TODO: Reserve destinations?
			case HaulStrategy::Cart:
				assert(m_haulTool != nullptr);
				if(actor.m_canLead.isLeading(m_haulTool))
				{
					if(m_haulTool.m_hasItems.contains(m_toHaul))
					{
						if(actor.m_location == m_destination)
						{
							m_haulTool.m_hasItems.transferTo(m_destination.m_hasItems);
							m_haulTool.m_canFollow.unfollow();
							m_haulTool.m_reservable.unreserve();
							onComplete();
						}
						else
							actor.setDestination(m_destination);
					}
					else
					{
						if(actor.isAdjacentTo(m_toHaul))
						{
							//TODO: set delay for loading.
							m_toHaul.m_location.m_hasItems.transferTo(m_haulTool.m_hasItems, m_toHaul);
							actor.setDestination(m_destination);
						}
						else
							actor.setDestinationAdjacentTo(m_toHaul);
					}
				}
				else
				{
					if(actor.isAdjacentTo(m_haulTool))
					{
						m_haulTool.m_canFollow.follow(actor);
						actor.setDestinationAdjacentTo(m_toHaul);
					}
					else
						actor.setDestinationAdjacentTo(m_haulTool);

				}
				break;
			case HaulStrategy::Panniers:
				assert(m_beastOfBurden != nullptr);
				assert(m_haulTool != nullptr);
				if(m_beastOfBurden->m_hasItems.contains(m_haulTool))
				{
					// Beast has panniers.
					if(m_haulTool.m_hasItems.contains(m_toHaul))
					{
						// Panniers have cargo.
						if(actor.m_location == &m_destination)
						{
							// Actor is at destination.
							//TODO: unloading delay.
							m_haulTool.m_hasItems.transferTo(m_destination);
							m_beastOfBurden.m_canFollow.unfollow();
							m_beastOfBurden.m_reservable.unreserve();
							onComplete();
						}
						else
							actor.setDestination(m_destination);
					}
					else
					{
						// Panniers don't have cargo.
						if(actor.m_location == m_toHaul.m_location)
						{
							// Actor is at pickup location.
							// TODO: loading delay.
							m_toHaul.m_location.m_hasItems.transferTo(m_haulTool.m_hasItems, m_toHaul);
							actor.setDestination(m_destination);
						}
						else
							actor.setDestination(m_toHaul.m_location);
					}
				}
				else
				{
					// Get beast panniers.
					if(actor.m_hasItems.m_carrying == &m_haulTool)
					{
						// Actor has panniers.
						if(actor.isAdjacentTo(m_beastOfBurden))
						{
							// Actor can put on panniers.
							m_beastOfBurden.m_hasItems.addEquipment(m_haulTool);
							m_beastOfBurden.m_canFollow.follow(actor);
							actor.setDestination(*m_toHaul.m_location);
						}
						else
							actor.setDestinationAdjacentTo(m_beastOfBurden);
					}
					else
					{
						// Bring panniers to beast.
						if(actor.m_location == m_haulTool.m_location)
						{
							actor.m_hasItems.pickUp(m_haulTool);
							actor.setDestinationAdjacentTo(m_beastOfBurden);
						}
						else
							actor.setDestination(*m_haulTool.m_location);
					}

				}
				break;
			case HaulStrategy::AnimalCart:
				assert(m_beastOfBurden != nullptr);
				assert(m_haulTool != nullptr);
				if(m_beastOfBurden->m_canLead.isLeading(m_haulTool))
				{
					// Beast has cart.
					if(m_haulTool.m_hasItems.contains(m_toHaul))
					{
						// Cart has cargo.
						if(actor.m_location == &m_destination)
						{
							// Actor is at destination.
							//TODO: unloading delay.
							//TODO: unfollow cart?
							m_haulTool.m_hasItems.transferTo(m_destination);
							m_beastOfBurden.m_canFollow.unfollow();
							m_beastOfBurden.m_reservable.unreserve();
							onComplete();
						}
						else
							actor.setDestination(m_destination);
					}
					else
					{
						// Cart doesn't have cargo.
						if(actor.m_location == m_toHaul.m_location)
						{
							// Actor is at pickup location.
							// TODO: loading delay.
							m_toHaul.m_location.m_hasItems.transferTo(m_haulTool.m_hasItems, m_toHaul);
							actor.setDestination(m_destination);
						}
						else
							actor.setDestination(m_toHaul.m_location);
					}
				}
				else
				{
					// Bring beast to cart.
					if(actor.m_canLead.isLeading(m_beastOfBurden))
					{
						// Actor has beast.
						if(actor.isAdjacentTo(m_haulTool))
						{
							// Actor can harness beast.
							m_haulTool.m_canFollow.follow(m_beastOfBurden);
							m_beastOfBurden.m_canFollow.follow(actor);
							actor.setDestination(*m_toHaul.m_location);
						}
						else
							actor.setDestinationAdjacentTo(m_haulTool);
					}
					else
					{
						// Get beast.
						if(actor.isAdjacentTo(m_beastOfBurden))
						{
							m_beastOfBurden.m_canFollow.follow(actor);
							actor.setDestinationAdjacentTo(m_haulTool);
						}
						else
							actor.setDestination(*m_haulTool.m_location);
					}

				}
				break;
			case HaulStrategy::TeamCart:
				break;
			case HaulStrategy::StrongSentient:
				break;
		}
	}
	bool tryToSetHaulStrategy(Actor& actor, std::unordered_set<Actor*>& waiting)
	{
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
					waiting.remove(actor);
				}
				return true;
			}
		}
		Item* haulTool = actor.m_location->m_area->m_hasHaulTools.hasToolFor(m_toHaul);
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
			Actor* yoked = actor.m_location->m_area->m_hasHaulTools.getActorToYokeFor(haulTool);
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
					waiting.remove(actor);
				}
				return true;
			}
		}
		Actor* pannierBearer = actor.m_location->m_area->m_hasHaulTools.getPannierBearerFor(m_toHaul);
		if(pannierBearer != nullptr)
		{
			Item* panniers = actor.m_location->m_area->m_hasHaulTools.getPanniersFor(pannierBearer);
			if(panniers != nullptr && getSpeedWithPannierBearer(m_toHaul, pannierBearer) >= m_minimumHaulSpeed)
			{
				m_strategy = HaulStrategy::Panniers;
				m_beastOfBurden = pannierBearer;
				m_haulTool = panniers;
				addWorker(actor);
			}
		}
	}
};
