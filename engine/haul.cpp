#include "haul.h"
#include "config.h"
#include "hasShape.h"
#include "item.h"
#include "area.h"
#include "materialType.h"
#include "project.h"
#include "reservable.h"
#include "simulation/hasItems.h"
#include "simulation/hasActors.h"
#include "types.h"
#include "simulation.h"

#include <memory>
#include <sys/types.h>
#include <unordered_set>
#include <algorithm>

HaulStrategy haulStrategyFromName(std::string name)
{
	if(name == "Individual")
		return HaulStrategy::Individual;
	if(name == "Team")
		return HaulStrategy::Team;
	if(name == "IndividualCargoIsCart")
		return HaulStrategy::IndividualCargoIsCart;
	if(name == "Cart")
		return HaulStrategy::Cart;
	if(name == "TeamCart")
		return HaulStrategy::TeamCart;
	if(name == "Panniers")
		return HaulStrategy::Panniers;
	if(name == "AnimalCart")
		return HaulStrategy::AnimalCart;
	assert(name == "StrongSentient");
	return HaulStrategy::StrongSentient;
}
std::string haulStrategyToName(HaulStrategy strategy)
{
	if(strategy == HaulStrategy::Individual)
		return "Individual";
	if(strategy == HaulStrategy::Team)
		return "Team";
	if(strategy == HaulStrategy::IndividualCargoIsCart)
		return "IndividualCargoIsCart";
	if(strategy == HaulStrategy::Cart)
		return "Cart";
	if(strategy == HaulStrategy::TeamCart)
		return "TeamCart";
	if(strategy == HaulStrategy::Panniers)
		return "Panniers";
	if(strategy == HaulStrategy::AnimalCart)
		return "AnimalCart";
	assert(strategy == HaulStrategy::StrongSentient);
	return "StrongSentient";
}
void HaulSubprojectParamaters::reset()
{
	toHaul.clear();
	fluidType = nullptr;
	quantity = 0;
	strategy = HaulStrategy::None;
	haulTool = ITEM_INDEX_MAX;
	beastOfBurden = ACTOR_INDEX_MAX;
	projectRequirementCounts = nullptr;	
}
// Check that the haul strategy selected in read step is still valid in write step.
bool HaulSubprojectParamaters::validate(Area& area) const
{
	assert(toHaul.exists());
	assert(quantity != 0);
	assert(strategy != HaulStrategy::None);
	assert(projectRequirementCounts != nullptr);
	Faction& faction = *area.m_actors.getFaction(*workers.begin());
	for(ActorIndex worker : workers)
		if(area.m_actors.isFullyReserved(worker, &faction))
			return false;
	if(haulTool != ITEM_INDEX_MAX && area.m_items.isFullyReserved(haulTool, &faction))
		return false;
	if(beastOfBurden != ACTOR_INDEX_MAX && area.m_actors.isFullyReserved(beastOfBurden, &faction))
		return false;
	return true;
}
HaulSubprojectDishonorCallback::HaulSubprojectDishonorCallback(const Json data, DeserializationMemo& deserializationMemo) :
	m_haulSubproject(*deserializationMemo.m_haulSubprojects.at(data["haulSubproject"])) { }
HaulSubproject::HaulSubproject(Project& p, HaulSubprojectParamaters& paramaters) :
	m_project(p), m_workers(paramaters.workers.begin(), paramaters.workers.end()), m_toHaul(paramaters.toHaul),
       	m_quantity(paramaters.quantity), m_strategy(paramaters.strategy), m_haulTool(paramaters.haulTool),
	m_beastOfBurden(paramaters.beastOfBurden), m_projectRequirementCounts(*paramaters.projectRequirementCounts)
{
	assert(!m_workers.empty());
	Area& area = m_project.m_area;
	Items& items = area.m_items;
	if(m_haulTool != ITEM_INDEX_MAX)
	{
		std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<HaulSubprojectDishonorCallback>(*this);
		items.reservable_reserve(m_haulTool, m_project.m_canReserve, 1u, std::move(dishonorCallback));
	}
	if(m_beastOfBurden != ACTOR_INDEX_MAX)
	{
		std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<HaulSubprojectDishonorCallback>(*this);
		items.reservable_reserve(m_beastOfBurden, m_project.m_canReserve, 1u, std::move(dishonorCallback));
	}
	if(m_toHaul.isGeneric(area))
	{
		m_genericItemType = area.m_items.getItemType(m_toHaul.get());
		m_genericMaterialType = area.m_items.getMaterialType(m_toHaul.get());
	}
	for(ActorIndex actor : m_workers)
	{
		m_project.m_workers.at(actor).haulSubproject = this;
		commandWorker(actor);
	}
}
/*
HaulSubproject::HaulSubproject(const Json& data, Project& p, DeserializationMemo& deserializationMemo) :
	m_project(p),
	m_toHaul(data.contains("toHaulItem") ? static_cast<ActorOrItemIndex>(deserializationMemo.itemReference(data["toHaulItem"])) : static_cast<ActorOrItemIndex>(deserializationMemo.actorReference(data["toHaulActor"]))),
	m_quantity(data["quantity"].get<Quantity>()),
	m_strategy(haulStrategyFromName(data["haulStrategy"].get<std::string>())),
	m_haulTool(nullptr),
	m_leader(nullptr),
	m_itemIsMoving(data["itemIsMoving"].get<bool>()),
	m_beastOfBurden(nullptr),
	m_projectRequirementCounts(deserializationMemo.projectRequirementCountsReference(data["requirementCounts"])),
       	m_genericItemType(nullptr),
	m_genericMaterialType(nullptr)
{
	if(data.contains("haulTool"))
		m_haulTool = &deserializationMemo.m_simulation.m_hasItems->getById(data["haulTool"].get<ItemId>());
	if(data.contains("leader"))
		m_leader = &deserializationMemo.m_simulation.m_hasActors->getById(data["leader"].get<ActorId>());
	if(data.contains("beastOfBurden"))
		m_beastOfBurden = &deserializationMemo.m_simulation.m_hasActors->getById(data["beastOfBurden"].get<ActorId>());
	if(data.contains("genericItemType"))
		m_genericItemType = &ItemType::byName(data["genericItemType"].get<std::string>());
	if(data.contains("genericMaterialType"))
		m_genericMaterialType = &MaterialType::byName(data["genericMaterialType"].get<std::string>());
	if(data.contains("workers"))
		for(const Json& workerId : data["workers"])
			m_workers.insert(&deserializationMemo.m_simulation.m_hasActors->getById(workerId.get<ActorId>()));
	if(data.contains("nonsentients"))
		for(const Json& actorId : data["nonsentients"])
			m_nonsentients.insert(&deserializationMemo.m_simulation.m_hasActors->getById(actorId.get<ActorId>()));
	if(data.contains("liftPoints"))
		for(const Json& liftPointData : data["liftPoints"])
		{
			ActorIndex actor = deserializationMemo.m_simulation.m_hasActors->getById(liftPointData[0].get<ActorId>());
			m_liftPoints[actor] = liftPointData[1].get<BlockIndex>();
		}
	deserializationMemo.m_haulSubprojects[data["address"].get<uintptr_t>()] = this;
}
Json HaulSubproject::toJson() const
{
	Json data({
		{"quantity", m_quantity},
		{"haulStrategy", haulStrategyToName(m_strategy)},
		{"itemIsMoving", m_itemIsMoving},
		{"address", reinterpret_cast<uintptr_t>(this)},
		{"requirementCounts", reinterpret_cast<uintptr_t>(&m_projectRequirementCounts)},
	});
	if(m_toHaul.isItem())
		data["toHaulItem"] = static_cast<ItemIndex>(m_toHaul).m_id;
	else
	{
		assert(m_toHaul.isActor());
		data["toHaulActor"] = static_cast<ActorIndex>(m_toHaul).m_id;
	}
	if(m_haulTool != nullptr)
		data["haulTool"] = m_haulTool->m_id;
	if(m_leader != nullptr)
		data["leader"] = m_leader->m_id;
	if(m_beastOfBurden != nullptr)
		data["beastOfBurden"] = m_beastOfBurden->m_id;
	if(m_genericItemType != nullptr)
		data["genericItemType"] = m_genericItemType->name;
	if(m_genericMaterialType!= nullptr)
		data["genericMaterialType"] = m_genericMaterialType->name;
	if(!m_workers.empty())
	{
		data["workers"] = Json::array();
		for(ActorIndex worker : m_workers)
			data["workers"].push_back(worker->m_id);
	}
	if(!m_nonsentients.empty())
	{
		data["nonsentients"] = Json::array();
		for(ActorIndex nonsentient : m_nonsentients)
			data["nonsentients"].push_back(nonsentient->m_id);
	}
	if(!m_liftPoints.empty())
	{
		data["liftPoints"] = Json::array();
		for(auto& [actor, block] : m_liftPoints)
			data["liftPoints"].push_back(Json::array({actor->m_id, block}));
	}
	return data;
}
*/
void HaulSubproject::commandWorker(ActorIndex actor)
{
	assert(m_workers.contains(actor));
	Objective& objective = m_project.m_workers.at(actor).objective;
	bool detour = objective.m_detour;
	bool hasCargo = false;
	Area& area = m_project.m_area;
	Actors& actors = area.m_actors;
	Items& items = area.m_items;
	assert(actors.objective_exists(actor));
	auto& blocks = actors.getBlocks(actor);
	BlockIndex actorLocation = actors.getLocation(actor);
	Faction& faction = m_project.getFaction();
	switch(m_strategy)
	{
		case HaulStrategy::Individual:
			assert(m_workers.size() == 1);
			hasCargo = m_genericItemType != nullptr ?
				actors.canPickUp_isCarryingItemGeneric(actor, *m_genericItemType, *m_genericMaterialType, m_quantity) :
				actors.canPickUp_isCarryingPolymorphic(actor, m_toHaul);
			if(hasCargo)
			{
				if(actors.isAdjacentToLocation(actor, m_project.m_location))
				{
					// Unload
					ActorOrItemIndex cargo = actors.canPickUp_putDownItem(actor, actorLocation, m_quantity);
					complete(cargo);
				}
				else
					actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
			}
			else
			{
				if(m_toHaul.isAdjacentToActor(actor))
				{
					actors.canReserve_clearAll(actor);
					m_toHaul.reservable_unreserve(area, m_project.m_canReserve);
					actors.canPickUp_pickUp(actor, m_toHaul, m_quantity);
					commandWorker(area, actor);
				}
				else
					actors.move_setDestinationAdjacentTo(m_toHaul, detour);
			}
			break;
		case HaulStrategy::IndividualCargoIsCart:
			assert(m_workers.size() == 1);
			if(actors.isLeadingPolymorphic(actor, m_toHaul))
			{
				BlockIndex projectLocation = m_project.m_location;
				if(actors.isAdjacentToLocation(actor, projectLocation))
				{
					// Unload
					m_toHaul.unfollow(area);
					complete(m_toHaul);
				}
				else
					actors.move_setDestinationAdjacentToLocation(actor, projectLocation);
			}
			else
			{
				if(m_toHaul.isAdjacentToActor(area, actor))
				{
					actors.unreserveAll(actor);
					m_toHaul.reservable_unreserve(area, m_project.m_canReserve, m_quantity);
					m_toHaul.follow(area, actors.getCanLead(actor), true);
					commandWorker(area, actor);
				}
				else
					actors.move_setDestinationAdjacentToPolymorphic(actor, m_toHaul, detour);
			}
			break;
		case HaulStrategy::Team:
			// TODO: teams of more then 2, requires muliple followers.
			assert(m_workers.size() == 2);
			if(m_leader == ACTOR_INDEX_MAX)
				m_leader = actor;
			if(m_itemIsMoving)
			{
				assert(actor == m_leader);
				if(actors.isAdjacentToLocation(actor, m_project.m_location))
				{
					// At destination.
					m_toHaul.unfollow(area);
					for(ActorIndex actor : m_workers)
						if(actor != m_leader)
							actors.unfollow(actor);
					m_toHaul.setLocationAndFacing(area, actorLocation, 0);
					complete(m_toHaul);
				}
				else
					actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
			}
			else
			{
				// Item is not moving.
				if(m_liftPoints.contains(actor))
				{
					// A lift point exists.
					if(actorLocation == m_liftPoints.at(actor))
					{
						// Actor is at lift point.
						if(allWorkersAreAdjacentTo(m_toHaul))
						{
							// All actors are at lift points.
							m_toHaul.follow(area, actors.getCanLead(m_leader));
							for(ActorIndex follower : m_workers)
							{
								if(follower != m_leader)
									actors.follow(follower, m_toHaul.getCanLead(area));
								actors.canReserve_clearAll(follower);
							}
							actors.canReserve_clearAll(m_leader);
							actors.move_setDestinationAdjacentToLocation(m_leader, m_project.m_location, detour);
							m_itemIsMoving = true;
							m_toHaul.reservable_unreserve(area, m_project.m_canReserve, m_quantity);
						}
					}
					else
						// Actor is not at lift point.
						// destination, detour, adjacent, unreserved, reserve
						actors.move_setDestination(m_liftPoints.at(actor), detour);
				}
				else
				{
					// No lift point exists for this actor, find one.
					// TODO: move this logic to tryToSetHaulStrategy?
					for(BlockIndex block : m_toHaul.getAdjacentBlocks(area))
					{
						Facing facing = blocks.facingToSetWhenEnteringFrom(m_project.m_location, block);
						if(blocks.shape_canEnterEverWithFacing(block, actor, facing) && !blocks.isReserved(block, faction))
						{
							m_liftPoints[actor] = block;
							blocks.reserve(block, actors.getCanReserve(actor));
							// Destination, detour, adjacent, unreserved, reserve
							actors.move_setDestination(actor, block, detour);
							return;
						}
					}
					assert(false); // team strategy should not be choosen if there is not enough space to reserve for lifting.
				}
			}
			break;
			//TODO: Reserve destinations?
		case HaulStrategy::Cart:
			assert(m_haulTool != ITEM_INDEX_MAX);
			if(actors.isLeadingPolymorphic(m_haulTool))
			{
				// Has cart.
				hasCargo = m_genericItemType != nullptr ?
					items.cargo_containsGeneric(m_haulTool, *m_genericItemType, *m_genericMaterialType, m_quantity) :
					items.cargo_contains(m_haulTool, m_toHaul);
				if(hasCargo)
				{
					// Cart is loaded.
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						// At drop off point.
						items.unfollow(m_haulTool);
						ActorOrItemIndex delivered;
						if(m_genericItemType == nullptr)
						{
							items.cargo_unloadToPolymorphic(m_haulTool, m_toHaul, m_project.m_location);
							delivered = m_toHaul;
						}
						else
							delivered = items.cargo_unloadGenericTo(m_haulTool, *m_genericItemType, *m_genericMaterialType, m_quantity, actorLocation);
						// TODO: set rotation?
						assert(delivered.exists());
						complete(delivered);
					}
					else
						// Go to drop off point.
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					// Cart not loaded.
					if(m_toHaul.isAdjacentToActor(area, actor))
					{
						// Can load here.
						//TODO: set delay for loading.
						items.reservable_maybeUnreserve(m_toHaul, m_project.m_canReserve, m_quantity);
						items.cargo_loadPolymorphyc(m_toHaul, m_quantity);
						actors.unreserveAll(actor);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						// Go somewhere to load.
						actors.move_setDestinationAdjacentToPolymorphic(m_toHaul, detour);
				}
			}
			else
			{
				// Don't have Cart.
				if(actors.isAdjacentToItem(m_haulTool))
				{
					// Cart is here.
					items.follow(m_haulTool, actors.getCanLead(actor));
					actors.unreserveAll(actor);
					actors.move_setDestinationAdjacentToPolymorphic(actor, m_toHaul, detour);
				}
				else
					// Go to cart.
					actors.move_setDestinationAdjacentToItem(actor, m_haulTool, detour);

			}
			break;
		case HaulStrategy::Panniers:
			assert(m_beastOfBurden != ACTOR_INDEX_MAX);
			assert(m_haulTool != ITEM_INDEX_MAX);
			if(actors.getEquipmentSet(m_beastOfBurden).contains(m_haulTool))
			{
				// Beast has panniers.
				hasCargo = m_genericItemType != nullptr ?
					items.cargo_containsGeneric(m_haulTool, *m_genericItemType, *m_genericMaterialType, m_quantity) :
					items.cargo_contains(m_haulTool, m_toHaul);
				if(hasCargo)
				{
					// Panniers have cargo.
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						// Actor is at destination.
						ActorOrItemIndex delivered;
						//TODO: unloading delay.
						if(m_genericItemType == nullptr)
						{
							items.cargo_unloadToPolymorphic(m_toHaul, actorLocation);
							delivered = m_toHaul;
						}
						else
						{
							delivered = items.cargo_unloadGenericTo(m_haulTool, *m_genericItemType, *m_genericMaterialType, m_quantity, actorLocation);
						}
						// TODO: set rotation?
						actors.unfollow(m_beastOfBurden);
						complete(delivered);
					}
					else
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					// Panniers don't have cargo.
					if(m_toHaul.isAdjacentToActor(area, actor))
					{
						// Actor is at pickup location.
						// TODO: loading delay.
						items.reservable_maybeUnreserve(m_toHaul, m_project.m_canReserve, m_quantity);
						items.cargo_loadPolymorphyc(m_toHaul, m_quantity);
						actors.unreserveAll(actor);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						actors.move_setDestinationAdjacentToPolymorphic(actor, m_toHaul, detour);
				}
			}
			else
			{
				// Get beast panniers.
				if(actors.canPickUp_isCarryingItem(m_haulTool))
				{
					// Actor has panniers.
					if(actors.isAdjacentToActor(actor, m_beastOfBurden))
					{
						// Actor can put on panniers.
						actors.canPickUp_canPickUpItem(actor, m_haulTool);
						actors.getEquipmentSet(m_beastOfBurden).addEquipment(m_haulTool);
						actors.follow(m_beastOfBurden, actors.getCanLead(actor));
						actors.unreserveAll(actor);
						actors.move_setDestinationAdjacentToPolymorphic(actor, m_toHaul, detour);
					}
					else
						actors.move_setDestinationAdjacentToActor(actor, m_beastOfBurden, detour);
				}
				else
				{
					// Bring panniers to beast.
					// TODO: move to adjacent to panniers.
					if(actors.isAdjacentToLocation(items.getLocation(m_haulTool)))
					{
						actors.canPickUp_pickUp(actor, m_haulTool);
						actors.unreserveAll(actor);
						actors.move_setDestinationAdjacentToActor(actor, m_beastOfBurden, detour);
					}
					else
						actors.move_setDesitinationAdjacentTo(actor, m_haulTool, detour);
				}

			}
			break;
		case HaulStrategy::AnimalCart:
			//TODO: what if already attached to a different cart?
			assert(m_beastOfBurden != ACTOR_INDEX_MAX);
			assert(m_haulTool != ITEM_INDEX_MAX);
			if(actors.isLeadingPolymorphic(m_beastOfBurden, m_haulTool))
			{
				// Beast has cart.
				hasCargo = m_genericItemType != nullptr ?
					items.cargo_containsGeneric(m_haulTool, *m_genericItemType, *m_genericMaterialType, m_quantity) :
					items.cargo_contains(m_haulTool, m_toHaul);
				if(hasCargo)
				{
					// Cart has cargo.
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						// Actor is at destination.
						//TODO: unloading delay.
						//TODO: unfollow cart?
						ActorOrItemIndex delivered;
						if(m_genericItemType == nullptr)
						{
							items.cargo_unloadToPolymorphic(m_toHaul, actorLocation);
							delivered = m_toHaul;
						}
						else
							delivered = items.cargo_unloadGenericTo(m_haulTool, *m_genericItemType, *m_genericMaterialType, m_quantity, actorLocation);
						// TODO: set rotation?
						actors.unfollow(m_beastOfBurden);
						complete(delivered);
					}
					else
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					// Cart doesn't have cargo.
					if(m_toHaul.isAdjacentToActor(area, actor))
					{
						// Actor is at pickup location.
						// TODO: loading delay.
						items.reservable_maybeUnreserve(m_toHaul, m_project.m_canReserve);
						items.cargo_loadPolymorphyc(m_toHaul, m_quantity);
						actors.unreserveAll(actor);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						actors.move_setDestinationAdjacentToPolymorphic(actor, m_toHaul, detour);
				}
			}
			else
			{
				// Bring beast to cart.
				if(actors.isLeading(actor, m_beastOfBurden))
				{
					// Actor has beast.
					if(actors.isAdjacentToItem(m_haulTool))
					{
						// Actor can harness beast to item.
						// Don't check if item is adjacent to beast, allow it to teleport.
						// TODO: Make not teleport.
						items.follow(actors.getCanLead(m_beastOfBurden), false);
						// Skip adjacent check, potentially teleport.
						actors.unreserveAll(actor);
						actors.move_setDestinationAdjacentToPolymorphic(actor, m_toHaul, detour);
					}
					else
						actors.move_setDesitinationAdjacentTo(actor, m_haulTool, detour);
				}
				else
				{
					// Get beast.
					if(actors.isAdjacentToActor(actor, m_beastOfBurden))
					{
						actors.follow(m_beastOfBurden, actors.getCanLead(actor));
						actors.unreserveAll(actor);
						actors.unreserveAll(m_beastOfBurden);
						actors.move_setDesitinationAdjacentTo(actor, m_haulTool, detour);
					}
					else
						actors.move_setDestinationAdjacentToActor(actor, m_beastOfBurden, detour);
				}

			}
			break;
		case HaulStrategy::TeamCart:
			assert(m_haulTool != ITEM_INDEX_MAX);
			// TODO: teams of more then 2, requires muliple followers.
			assert(m_workers.size() == 2);
			if(m_leader == ACTOR_INDEX_MAX)
				m_leader = actor;
			if(actors.isLeadingItem(actor, m_haulTool))
			{
				assert(actor == m_leader);
				hasCargo = m_genericItemType != nullptr ?
					items.cargo_containsGeneric(m_haulTool, *m_genericItemType, *m_genericMaterialType, m_quantity) :
					items.cargo_contains(m_haulTool, m_toHaul);
				if(hasCargo)
				{
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						ActorOrItemIndex delivered;
						if(m_genericItemType == nullptr)
						{
							items.cargo_unloadToPolymorphic(m_toHaul, actorLocation);
							delivered = m_toHaul;
						}
						else
							delivered = items.cargo_unloadGenericTo(m_haulTool, *m_genericItemType, *m_genericMaterialType, m_quantity, actorLocation);
						// TODO: set rotation?
						actors.disbandLeadAndFollow(m_leader);
						complete(delivered);
					}
					else
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					if(m_toHaul.isAdjacentToActor(area, actor))
					{
						//TODO: set delay for loading.
						items.reservable_maybeUnreserve(m_toHaul, m_project.m_canReserve);
						items.cargo_loadPolymorphyc(m_toHaul, m_quantity);
						actors.unreserveAll(actor);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						actors.move_setDestinationAdjacentToPolymorphic(actor, m_toHaul, detour);
				}
			}
			else if(actors.isAdjacentToItem(m_haulTool))
			{
				if(allWorkersAreAdjacentTo(area, m_haulTool))
				{
					// All actors are adjacent to the haul tool.
					items.followActor(m_haulTool, m_leader);
					for(ActorIndex follower : m_workers)
					{
						if(follower != m_leader)
							actors.followItem(follower, m_haulTool);
						actors.canReserve_clearAll(follower);
					}
					actors.canReserve_clearAll(m_leader);
					actors.move_setDestinationAdjacentToItem(m_leader, m_toHaul, detour);
				}
			}
			else
			{
				actors.unreserveAll(actor);
				actors.move_setDesitinationAdjacentTo(actor, m_haulTool, detour);
			}
			break;
		case HaulStrategy::StrongSentient:
			//TODO
			break;
		case HaulStrategy::None:
			assert(false); // this method should only be called after a strategy is choosen.
	}
}
void HaulSubproject::addWorker(ActorIndex actor)
{
	assert(!m_workers.contains(actor));
	m_workers.insert(actor);
	commandWorker(m_project.m_area, actor);
}
void HaulSubproject::removeWorker(ActorIndex actor)
{
	assert(m_workers.contains(actor));
	cancel();
}
void HaulSubproject::cancel()
{
	m_project.haulSubprojectCancel(*this);
}
bool HaulSubproject::allWorkersAreAdjacentTo(ActorOrItemIndex actorOrItem)
{
	return std::all_of(m_workers.begin(), m_workers.end(), [&](ActorIndex worker) { return actorOrItem.isAdjacentToActor(area, worker); });
}
bool HaulSubproject::allWorkersAreAdjacentTo(ItemIndex index)
{
	return std::all_of(m_workers.begin(), m_workers.end(), [&](ActorIndex worker) { return area.m_items.isAdjacentToActor(index, worker); });
}
HaulSubprojectParamaters HaulSubproject::tryToSetHaulStrategy(const Project& project, ActorOrItemIndex toHaul, ActorIndex worker)
{
	// TODO: make exception for slow haul if very close.
	HaulSubprojectParamaters output;
	output.toHaul = &toHaul;
	output.projectRequirementCounts = project.m_toPickup.at(&toHaul).first;
	Quantity maxQuantityRequested = project.m_toPickup.at(&toHaul).second;
	Speed minimumSpeed = project.getMinimumHaulSpeed();
	std::vector<ActorIndex> workers;
	// TODO: shouldn't this be m_waiting?
	for(auto& pair : project.m_workers)
		workers.push_back(pair.first);
	assert(maxQuantityRequested != 0);
	// toHaul is wheeled.
	static const MoveType& wheeled = MoveType::byName("roll");
	if(toHaul.getMoveType() == wheeled)
	{
		assert(toHaul.isItem());
		ItemIndex haulableItem = static_cast<ItemIndex>(toHaul);
		std::vector<const ActorOrItemIndex> list{&worker, &toHaul};
		if(CanLead::getMoveSpeedForGroup(list) >= minimumSpeed)
		{
			output.strategy = HaulStrategy::IndividualCargoIsCart;
			output.quantity = 1;
			output.workers.push_back(&worker);
			return output;
		}
		else
		{
			output.workers = actorsNeededToHaulAtMinimumSpeedWithTool(project, worker, toHaul, haulableItem);
			if(!output.workers.empty())
			{
				assert(output.workers.size() > 1);
				output.strategy = HaulStrategy::Team;
				output.quantity = 1;
				return output;
			}
		}
	}
	// Individual
	// TODO:: Prioritize cart if a large number of items are requested.
	Quantity maxQuantityCanCarry = worker.m_canPickup.maximumNumberWhichCanBeCarriedWithMinimumSpeed(toHaul, minimumSpeed);
	if(maxQuantityCanCarry != 0)
	{
		output.strategy = HaulStrategy::Individual;
		output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
		output.workers.push_back(&worker);
		return output;
	}
	// Team
	output.workers = actorsNeededToHaulAtMinimumSpeed(project, worker, toHaul);
	if(!output.workers.empty())
	{
		assert(output.workers.size() == 2); // TODO: teams larger then two.
		output.strategy = HaulStrategy::Team;
		output.quantity = 1;
		return output;
	}
	ItemIndex haulTool = worker.m_area->m_hasHaulTools.getToolToHaul(*worker.getFaction(), toHaul);
	// Cart
	if(haulTool != nullptr)
	{
		// Cart
		maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(worker, *haulTool, toHaul, minimumSpeed);
		if(maxQuantityCanCarry != 0)
		{
			output.strategy = HaulStrategy::Cart;
			output.haulTool = haulTool;
			output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
			output.workers.push_back(&worker);
			return output;
		}
		// Animal Cart
		ActorIndex yoked = worker.m_area->m_hasHaulTools.getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(*worker.getFaction(), *haulTool, toHaul.getMass(), minimumSpeed);
		if(yoked != nullptr)
		{
			maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(worker, *yoked, *haulTool, toHaul, minimumSpeed);
			if(maxQuantityCanCarry != 0)
			{
				output.strategy = HaulStrategy::AnimalCart;
				output.haulTool = haulTool;
				output.beastOfBurden = yoked;
				output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
				output.workers.push_back(&worker);
				return output;
			}
		}
		// Team Cart
		output.workers = actorsNeededToHaulAtMinimumSpeedWithTool(project, worker, toHaul, *haulTool);
		if(!output.workers.empty())
		{
			assert(output.workers.size() > 1);
			output.strategy = HaulStrategy::TeamCart;
			output.haulTool = haulTool;
			output.quantity = 1;
			return output;
		}
	}
	// Panniers
	ActorIndex pannierBearer = worker.m_area->m_hasHaulTools.getPannierBearerToHaulCargoWithMassWithMinimumSpeed(*worker.getFaction(), toHaul, minimumSpeed);
	if(pannierBearer != nullptr)
	{
		//TODO: If pannierBearer already has panniers equiped then use those, otherwise find ones to use. Same for animalCart.
		ItemIndex panniers = worker.m_area->m_hasHaulTools.getPanniersForActorToHaul(*worker.getFaction(), *pannierBearer, toHaul);
		if(panniers != nullptr)
		{
			maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithPanniersAndAnimal(worker, *pannierBearer, *panniers, toHaul, minimumSpeed);
			if(maxQuantityCanCarry != 0)
			{
				output.strategy = HaulStrategy::Panniers;
				output.beastOfBurden = pannierBearer;
				output.haulTool = panniers;
				output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
				output.workers.push_back(&worker);
				return output;
			}
		}
	}
	assert(output.strategy == HaulStrategy::None);
	return output;
}
void HaulSubproject::complete(ActorOrItemIndex delivered)
{
	if(m_haulTool != nullptr)
		m_haulTool->m_reservable.clearReservationFor(m_project.m_canReserve);
	if(m_beastOfBurden != nullptr)
		m_beastOfBurden->m_reservable.clearReservationFor(m_project.m_canReserve);
	m_projectRequirementCounts.delivered += m_quantity;
	assert(m_projectRequirementCounts.delivered <= m_projectRequirementCounts.required);
	if(delivered.isItem())
	{
		if(m_projectRequirementCounts.consumed)
			m_project.m_toConsume.insert(static_cast<ItemIndex>(&delivered));
		if(static_cast<ItemIndex>(delivered).isWorkPiece())
			delivered.setLocation(m_project.m_location);
	}
	for(ActorIndex worker : m_workers)
		worker->m_canReserve.deleteAllWithoutCallback();
	delivered.m_reservable.reserveFor(m_project.m_canReserve, m_quantity);
	std::vector<ActorIndex> workers(m_workers.begin(), m_workers.end());
	Project& project = m_project;
	m_project.m_deliveredItems.push_back(static_cast<ItemIndex>(&delivered));
	m_project.onDelivered(delivered);
	m_project.m_haulSubprojects.remove(*this);
	for(ActorIndex worker : workers)
	{
		project.m_workers.at(worker).haulSubproject = nullptr;
		project.commandWorker(*worker);
	}
}
// Class method.
std::vector<ActorIndex> HaulSubproject::actorsNeededToHaulAtMinimumSpeed(const Project& project, ActorIndex leader, const ActorOrItemIndex toHaul)
{
	std::vector<const ActorOrItemIndex> actorsAndItems;
	std::vector<ActorIndex> output;
	output.push_back(&leader);
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&toHaul);
	// For each actor without a sub project add to actors and items and see if the item can be moved fast enough.
	for(auto& [actor, projectWorker] : project.m_workers)
	{
		if(actor == &leader || projectWorker.haulSubproject != nullptr || !actor->isSentient())
			continue;
		assert(std::ranges::find(output, actor) == output.end());
		output.push_back(actor);
		if(output.size() > 2) //TODO: More then two requires multiple followers for one leader.
			break;
		actorsAndItems.push_back(actor);
		Speed speed = CanLead::getMoveSpeedForGroup(actorsAndItems);
		if(speed >= project.getMinimumHaulSpeed())
			return output;
	}
	// Cannot form a team to move at acceptable speed.
	output.clear();
	return output;
}
//Class method.
Speed HaulSubproject::getSpeedWithHaulToolAndCargo(const ActorIndex leader, const ItemIndex haulTool, const ActorOrItemIndex toHaul, Quantity quantity)
{
	std::vector<const ActorOrItemIndex> actorsAndItems;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&haulTool);
	// actorsAndItems, rollingMass, deadMass
	return CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, toHaul.singleUnitMass() * quantity, 0);
}
// Class method.
Quantity HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(const ActorIndex leader, const ItemIndex haulTool, const ActorOrItemIndex toHaul, Speed minimumSpeed)
{
	assert(minimumSpeed != 0);
	Quantity quantity = 0;
	while(getSpeedWithHaulToolAndCargo(leader, haulTool, toHaul, quantity + 1) >= minimumSpeed)
		quantity++;
	return quantity;
}
// Class method.
Speed HaulSubproject::getSpeedWithHaulToolAndAnimal(const ActorIndex leader, const ActorIndex yoked, const ItemIndex haulTool, const ActorOrItemIndex toHaul, Quantity quantity)
{
	std::vector<const ActorOrItemIndex> actorsAndItems;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&yoked);
	actorsAndItems.push_back(&haulTool);
	// actorsAndItems, rollingMass, deadMass
	return CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, toHaul.singleUnitMass() * quantity, 0);
}
// Class method.
Speed HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(const ActorIndex leader, ActorIndex yoked, const ItemIndex haulTool, const ActorOrItemIndex toHaul, Speed minimumSpeed)
{
	assert(minimumSpeed != 0);
	Quantity quantity = 0;
	while(getSpeedWithHaulToolAndAnimal(leader, yoked, haulTool, toHaul, quantity + 1) >= minimumSpeed)
		quantity++;
	return quantity;
}
// Class method.
std::vector<ActorIndex> HaulSubproject::actorsNeededToHaulAtMinimumSpeedWithTool(const Project& project, ActorIndex leader, const ActorOrItemIndex toHaul, const ItemIndex haulTool)
{
	std::vector<const ActorOrItemIndex> actorsAndItems;
	std::vector<ActorIndex> output;
	output.push_back(&leader);
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&haulTool);
	// For each actor without a sub project add to actors and items and see if the item can be moved fast enough.
	for(auto& [actor, projectWorker] : project.m_workers)
	{
		if(actor == &leader || projectWorker.haulSubproject != nullptr || !actor->isSentient())
			continue;
		assert(std::ranges::find(output, actor) == output.end());
		output.push_back(actor);
		if(output.size() > 2) //TODO: More then two requires multiple followers for one leader.
			break;
		actorsAndItems.push_back(actor);
		// If to haul is also haul tool then there is no cargo, we are hauling the haulTool itself.
		Mass mass = &toHaul == &static_cast<const ActorOrItemIndex>(haulTool) ? 0 : toHaul.singleUnitMass();
		Speed speed = CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, mass);
		if(speed >= project.getMinimumHaulSpeed())
			return output;
	}
	// Cannot form a team to move at acceptable speed.
	output.clear();
	return output;
}
// Class method.
Speed HaulSubproject::getSpeedWithPannierBearerAndPanniers(const ActorIndex leader, const ActorIndex pannierBearer, const ItemIndex panniers, const ActorOrItemIndex toHaul, Quantity quantity)
{
	std::vector<const ActorOrItemIndex> actorsAndItems;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&pannierBearer);
	// actorsAndItems, rollingMass, deadMass
	return CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, 0, (toHaul.singleUnitMass() * quantity) + panniers.getMass());
}
// Class method.
Quantity HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithPanniersAndAnimal(const ActorIndex leader, const ActorIndex pannierBearer, const ItemIndex panniers, const ActorOrItemIndex toHaul, Speed minimumSpeed)
{
	assert(minimumSpeed != 0);
	Quantity quantity = 0;
	while(getSpeedWithPannierBearerAndPanniers(leader, pannierBearer, panniers, toHaul, quantity + 1) >= minimumSpeed)
		quantity++;
	return quantity;
}
//TODO: optimize?
bool AreaHasHaulTools::hasToolToHaul(Faction& faction, const ActorOrItemIndex hasShape) const
{
	return getToolToHaul(faction, hasShape) != nullptr;
}
ItemIndex AreaHasHaulTools::getToolToHaul(Faction& faction, const ActorOrItemIndex hasShape) const
{
	// Items like panniers also have internal volume but aren't relevent for this method.
	static const MoveType& none = MoveType::byName("none");
	for(ItemIndex item : m_haulTools)
		if(item->m_itemType.moveType != none && !item->m_reservable.isFullyReserved(&faction) && item->m_itemType.internalVolume >= hasShape.getVolume())
			return item;
	return nullptr;
}
bool AreaHasHaulTools::hasToolToHaulFluid(Faction& faction) const
{
	return getToolToHaulFluid(faction) != nullptr;
}
ItemIndex AreaHasHaulTools::getToolToHaulFluid(Faction& faction) const
{
	for(ItemIndex item : m_haulTools)
		if(!item->m_reservable.isFullyReserved(&faction) && item->m_itemType.canHoldFluids)
			return item;
	return nullptr;
}
ActorIndex AreaHasHaulTools::getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(Faction& faction, const ItemIndex haulTool, Mass cargoMass, Speed minimumHaulSpeed) const
{
	[[maybe_unused]] static const MoveType& rollingType = MoveType::byName("roll");
	assert(haulTool.m_itemType.moveType == rollingType);
	for(ActorIndex actor : m_yolkableActors)
	{
		std::vector<const ActorOrItemIndex> shapes = { actor, &haulTool };
		if(!actor->m_reservable.isFullyReserved(&faction) && minimumHaulSpeed <= actor->m_canLead.getMoveSpeedForGroupWithAddedMass(shapes, cargoMass))
			return actor;
	}
	return nullptr;
}
ActorIndex AreaHasHaulTools::getPannierBearerToHaulCargoWithMassWithMinimumSpeed(Faction& faction, const ActorOrItemIndex hasShape, Speed minimumHaulSpeed) const
{
	//TODO: Account for pannier mass?
	for(ActorIndex actor : m_yolkableActors)
		if(!actor->m_reservable.isFullyReserved(&faction) && minimumHaulSpeed <= actor->m_canPickup.speedIfCarryingQuantity(hasShape, 1u))
			return actor;
	return nullptr;
}
ItemIndex AreaHasHaulTools::getPanniersForActorToHaul(Faction& faction, const ActorIndex actor, const ActorOrItemIndex toHaul) const
{
	for(ItemIndex item : m_haulTools)
		if(!item->m_reservable.isFullyReserved(&faction) && item->m_itemType.internalVolume >= toHaul.getVolume() && actor.m_equipmentSet.canEquipCurrently(*item))
			return item;
	return nullptr;
}
void AreaHasHaulTools::registerHaulTool(ItemIndex item)
{
	assert(!m_haulTools.contains(&item));
	assert(item.m_itemType.internalVolume != 0);
	m_haulTools.insert(&item);
}
void AreaHasHaulTools::registerYokeableActor(ActorIndex actor)
{
	assert(!m_yolkableActors.contains(actor));
	m_yolkableActors.insert(actor);
}
void AreaHasHaulTools::unregisterHaulTool(ItemIndex item)
{
	assert(m_haulTools.contains(&item));
	m_haulTools.erase(&item);
}
void AreaHasHaulTools::unregisterYokeableActor(ActorIndex actor)
{
	assert(m_yolkableActors.contains(actor));
	m_yolkableActors.erase(actor);
}
