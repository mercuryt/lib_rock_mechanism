#include "haul.h"
#include "actorOrItemIndex.h"
#include "actors/actors.h"
#include "index.h"
#include "items/items.h"
#include "blocks/blocks.h"
#include "config.h"
#include "area.h"
#include "materialType.h"
#include "project.h"
#include "reference.h"
#include "reservable.h"
#include "simulation/hasItems.h"
#include "simulation/hasActors.h"
#include "types.h"
#include "simulation.h"
#include "itemType.h"
#include "moveType.h"

#include <memory>
#include <sys/types.h>
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
	quantity = Quantity::create(0);
	strategy = HaulStrategy::None;
	haulTool.clear();
	beastOfBurden.clear();
	projectRequirementCounts = nullptr;	
}
// Check that the haul strategy selected in read step is still valid in write step.
bool HaulSubprojectParamaters::validate(Area& area) const
{
	assert(toHaul.exists());
	assert(quantity != 0);
	assert(strategy != HaulStrategy::None);
	assert(projectRequirementCounts != nullptr);
	assert(!workers.empty());
	FactionId faction = area.getActors().getFactionId(workers.front());
	for(ActorIndex worker : workers)
		if(area.getActors().reservable_isFullyReserved(worker, faction))
			return false;
	if(haulTool.exists() && area.getItems().reservable_isFullyReserved(haulTool, faction))
		return false;
	if(beastOfBurden.exists() && area.getActors().reservable_isFullyReserved(beastOfBurden, faction))
		return false;
	return true;
}
HaulSubprojectDishonorCallback::HaulSubprojectDishonorCallback(const Json data, DeserializationMemo& deserializationMemo) :
	m_haulSubproject(*deserializationMemo.m_haulSubprojects.at(data["haulSubproject"])) { }
HaulSubproject::HaulSubproject(Project& p, HaulSubprojectParamaters& paramaters) :
	m_project(p), m_toHaul(paramaters.toHaul),
	m_quantity(paramaters.quantity), m_strategy(paramaters.strategy),
	m_projectRequirementCounts(*paramaters.projectRequirementCounts)
{
	Area& area = m_project.m_area;
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	assert(!paramaters.workers.empty());
	for(ActorIndex actor : paramaters.workers)
		m_workers.add(actors.getReference(actor));
	if(paramaters.haulTool.exists())
	{
		m_haulTool.setTarget(items.getReferenceTarget(paramaters.haulTool));
		std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<HaulSubprojectDishonorCallback>(*this);
		items.reservable_reserve(m_haulTool.getIndex(), m_project.m_canReserve, Quantity::create(1u), std::move(dishonorCallback));
	}
	if(paramaters.beastOfBurden.exists())
	{
		m_beastOfBurden.setTarget(actors.getReferenceTarget(paramaters.beastOfBurden));
		std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<HaulSubprojectDishonorCallback>(*this);
		actors.reservable_reserve(m_beastOfBurden.getIndex(), m_project.m_canReserve, Quantity::create(1u), std::move(dishonorCallback));
	}
	if(m_toHaul.getIndexPolymorphic().isGeneric(area))
	{
		m_genericItemType = items.getItemType(ItemIndex::cast(m_toHaul.getIndex()));
		m_genericMaterialType = items.getMaterialType(ItemIndex::cast(m_toHaul.getIndex()));
	}
	for(ActorReference actor : m_workers)
	{
		m_project.m_workers[actor].haulSubproject = this;
		commandWorker(actor.getIndex());
	}
}
HaulSubproject::HaulSubproject(const Json& data, Project& p, DeserializationMemo& deserializationMemo) :
	m_project(p),
	m_quantity(data["quantity"].get<Quantity>()),
	m_strategy(haulStrategyFromName(data["haulStrategy"].get<std::string>())),
	m_itemIsMoving(data["itemIsMoving"].get<bool>()),
	m_projectRequirementCounts(deserializationMemo.projectRequirementCountsReference(data["requirementCounts"]))
{
	Area& area = m_project.m_area;
	if(data.contains("toHaul"))
		m_toHaul.load(data["toHaul"], area);
	if(data.contains("haulTool"))
		m_haulTool.load(data["haulTool"], area);
	if(data.contains("leader"))
		m_leader.load(data["leader"], area);
	if(data.contains("beastOfBurden"))
		m_beastOfBurden.load(data["beastOfBurden"], area);
	if(data.contains("genericItemType"))
		m_genericItemType = ItemType::byName(data["genericItemType"].get<std::string>());
	if(data.contains("genericMaterialType"))
		m_genericMaterialType = MaterialType::byName(data["genericMaterialType"].get<std::string>());
	Actors& actors = area.getActors();
	if(data.contains("workers"))
		for(const Json& workerIndex : data["workers"])
			m_workers.add(actors.getReference(workerIndex.get<ActorIndex>()));
	if(data.contains("nonsentients"))
		for(const Json& actorIndex : data["nonsentients"])
			m_nonsentients.add(actors.getReference(actorIndex.get<ActorIndex>()));
	if(data.contains("liftPoints"))
		for(const Json& liftPointData : data["liftPoints"])
		{
			ActorIndex actor = liftPointData[0].get<ActorIndex>();
			m_liftPoints[actors.getReference(actor)] = liftPointData[1].get<BlockIndex>();
		}
	deserializationMemo.m_haulSubprojects[data["address"].get<uintptr_t>()] = this;
}
Json HaulSubproject::toJson() const
{
	Json data({
		{"quantity", m_quantity},
		{"haulStrategy", haulStrategyToName(m_strategy)},
		{"itemIsMoving", m_itemIsMoving},
		{"toHaul", m_toHaul},
		{"address", reinterpret_cast<uintptr_t>(this)},
		{"requirementCounts", reinterpret_cast<uintptr_t>(&m_projectRequirementCounts)},
	});
	if(m_haulTool.exists())
		data["haulTool"] = m_haulTool;
	if(m_leader.exists())
		data["leader"] = m_leader;
	if(m_beastOfBurden.exists())
		data["beastOfBurden"] = m_beastOfBurden;
	if(m_genericItemType.exists())
		data["genericItemType"] = m_genericItemType;
	if(m_genericMaterialType.exists())
		data["genericMaterialType"] = m_genericMaterialType;
	if(!m_workers.empty())
	{
		data["workers"] = Json::array();
		for(ActorReference worker : m_workers)
			data["workers"].push_back(worker);
	}
	if(!m_nonsentients.empty())
	{
		data["nonsentients"] = Json::array();
		for(ActorReference nonsentient : m_nonsentients)
			data["nonsentients"].push_back(nonsentient);
	}
	if(!m_liftPoints.empty())
	{
		data["liftPoints"] = Json::array();
		for(auto [actor, block] : m_liftPoints)
			data["liftPoints"].push_back(Json::array({actor, block}));
	}
	return data;
}
void HaulSubproject::commandWorker(const ActorIndex& actor)
{
	ActorReference ref = m_project.m_area.getActors().getReference(actor);
	assert(m_workers.contains(ref));
	Objective& objective = *m_project.m_workers[ref].objective;
	bool detour = objective.m_detour;
	bool hasCargo = false;
	Area& area = m_project.m_area;
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	Blocks& blocks = area.getBlocks();
	assert(actors.objective_exists(actor));
	BlockIndex actorLocation = actors.getLocation(actor);
	FactionId faction = m_project.getFaction();
	ActorOrItemIndex toHaulIndex = m_toHaul.getIndexPolymorphic();
	ItemIndex haulToolIndex;
	if(m_haulTool.exists())
		haulToolIndex = m_haulTool.getIndex();
	else
		haulToolIndex.clear();
	switch(m_strategy)
	{
		case HaulStrategy::Individual:
			assert(m_workers.size() == 1);
			hasCargo = m_genericItemType.exists() ?
				actors.canPickUp_isCarryingItemGeneric(actor, m_genericItemType, m_genericMaterialType, m_quantity) :
				actors.canPickUp_isCarryingPolymorphic(actor, m_toHaul.getIndexPolymorphic());
			if(hasCargo)
			{
				if(actors.isAdjacentToLocation(actor, m_project.m_location))
				{
					// Unload
					ActorOrItemIndex cargo = actors.canPickUp_tryToPutDownPolymorphic(actor, actorLocation);
					if(cargo.empty())
					{
						assert(false);
						//TODO: Cargo cannot be put down here, try again
					}
					complete(cargo);
				}
				else
					actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
			}
			else
			{
				if(toHaulIndex.isAdjacentToActor(area, actor))
				{
					actors.canReserve_clearAll(actor);
					toHaulIndex.reservable_unreserve(area, m_project.m_canReserve, m_quantity);
					actors.canPickUp_pickUpPolymorphic(actor, toHaulIndex, m_quantity);
					commandWorker(actor);
				}
				else
					actors.move_setDestinationAdjacentToPolymorphic(actor, toHaulIndex, detour);
			}
			break;
		case HaulStrategy::IndividualCargoIsCart:
			assert(m_workers.size() == 1);
			if(actors.isLeadingPolymorphic(actor, toHaulIndex))
			{
				BlockIndex projectLocation = m_project.m_location;
				if(actors.isAdjacentToLocation(actor, projectLocation))
				{
					// Unload
					toHaulIndex.unfollow(area);
					complete(toHaulIndex);
				}
				else
					actors.move_setDestinationAdjacentToLocation(actor, projectLocation);
			}
			else
			{
				if(toHaulIndex.isAdjacentToActor(area, actor))
				{
					actors.canReserve_clearAll(actor);
					// Reservation is held project, not by actor.
					toHaulIndex.reservable_unreserve(area, m_project.m_canReserve, m_quantity);
					toHaulIndex.followActor(area, actor);
					commandWorker(actor);
				}
				else
					actors.move_setDestinationAdjacentToPolymorphic(actor, toHaulIndex, detour);
			}
			break;
		case HaulStrategy::Team:
			// TODO: teams of more then 2, requires muliple followers.
			assert(m_workers.size() == 2);
			if(!m_leader.exists())
				m_leader.setTarget(actors.getReferenceTarget(actor));
			if(m_itemIsMoving)
			{
				assert(actor == m_leader.getIndex());
				if(actors.isAdjacentToLocation(actor, m_project.m_location))
				{
					// At destination.
					actors.leadAndFollowDisband(m_leader.getIndex());
					toHaulIndex.setLocationAndFacing(area, actorLocation, Facing::create(0));
					complete(toHaulIndex);
				}
				else
					actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
			}
			else
			{
				// Item is not moving.
				if(m_liftPoints.contains(ref))
				{
					// A lift point exists.
					if(actorLocation == m_liftPoints[ref])
					{
						// Actor is at lift point.
						if(allWorkersAreAdjacentTo(toHaulIndex))
						{
							// All actors are at lift points.
							toHaulIndex.followActor(area, m_leader.getIndex());
							for(ActorReference follower : m_workers)
							{
								if(follower != m_leader)
									actors.followPolymorphic(follower.getIndex(), toHaulIndex);
								actors.canReserve_clearAll(follower.getIndex());
							}
							actors.canReserve_clearAll(m_leader.getIndex());
							actors.move_setDestinationAdjacentToLocation(m_leader.getIndex(), m_project.m_location, detour);
							m_itemIsMoving = true;
							toHaulIndex.reservable_unreserve(area, m_project.m_canReserve, m_quantity);
						}
					}
					else
						// Actor is not at lift point.
						// destination, detour, adjacent, unreserved, reserve
						actors.move_setDestination(actor, m_liftPoints[ref], detour);
				}
				else
				{
					// No lift point exists for this actor, find one.
					// TODO: move this logic to tryToSetHaulStrategy?
					for(BlockIndex block : toHaulIndex.getAdjacentBlocks(area))
					{
						Facing facing = blocks.facingToSetWhenEnteringFrom(m_project.m_location, block);
						if(blocks.shape_shapeAndMoveTypeCanEnterEverWithFacing(block, actors.getShape(actor), actors.getMoveType(actor), facing) && !blocks.isReserved(block, faction))
						{
							m_liftPoints.insert(ref, block);
							actors.canReserve_reserveLocation(actor, block);
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
			assert(m_haulTool.exists());
			if(actors.isLeadingItem(actor, haulToolIndex))
			{
				// Has cart.
				hasCargo = m_genericItemType.exists() ?
					items.cargo_containsItemGeneric(haulToolIndex, m_genericItemType, m_genericMaterialType, m_quantity) :
					items.cargo_containsPolymorphic(haulToolIndex, toHaulIndex);
				if(hasCargo)
				{
					// Cart is loaded.
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						// At drop off point.
						items.unfollow(haulToolIndex);
						ActorOrItemIndex delivered;
						if(m_genericItemType.exists())
						{
							items.cargo_unloadPolymorphicToLocation(haulToolIndex, toHaulIndex, actorLocation, m_quantity);
							delivered = ActorOrItemIndex::createForItem(haulToolIndex);
						}
						else
						{
							ItemIndex item = items.cargo_unloadGenericItemToLocation(haulToolIndex, m_genericItemType, m_genericMaterialType, actorLocation, m_quantity);
							delivered = ActorOrItemIndex::createForItem(item);
						}

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
					if(toHaulIndex.isAdjacentToActor(area, actor))
					{
						// Can load here.
						//TODO: set delay for loading.
						toHaulIndex.reservable_maybeUnreserve(area, m_project.m_canReserve, m_quantity);
						items.cargo_loadPolymorphic(m_haulTool.getIndex(), toHaulIndex, m_quantity);
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						// Go somewhere to load.
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaulIndex, detour);
				}
			}
			else
			{
				// Don't have Cart.
				if(actors.isAdjacentToItem(actor, m_haulTool.getIndex()))
				{
					// Cart is here.
					items.followActor(m_haulTool.getIndex(), actor);
					actors.canReserve_clearAll(actor);
					actors.move_setDestinationAdjacentToPolymorphic(actor, toHaulIndex, detour);
				}
				else
					// Go to cart.
					actors.move_setDestinationAdjacentToItem(actor, m_haulTool.getIndex(), detour);
			}
			break;
		case HaulStrategy::Panniers:
			assert(m_beastOfBurden.exists());
			assert(m_haulTool.exists());
			if(actors.equipment_containsItem(m_beastOfBurden.getIndex(), m_haulTool.getIndex()))
			{
				// Beast has panniers.
				hasCargo = items.cargo_containsPolymorphic(m_haulTool.getIndex(), m_toHaul.getIndexPolymorphic(), m_quantity);
				if(hasCargo)
				{
					// Panniers have cargo.
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						// Actor is at destination.
						ActorOrItemIndex delivered;
						//TODO: unloading delay.
						if(m_genericItemType.empty())
						{
							items.cargo_unloadPolymorphicToLocation(m_haulTool.getIndex(), m_toHaul.getIndexPolymorphic(), actorLocation, m_quantity);
							delivered = toHaulIndex;
						}
						else
						{
							ItemIndex item = items.cargo_unloadGenericItemToLocation(m_haulTool.getIndex(), m_genericItemType, m_genericMaterialType, actorLocation, m_quantity);
							delivered = ActorOrItemIndex::createForItem(item);
						}
						// TODO: set rotation?
						actors.unfollow(m_beastOfBurden.getIndex());
						complete(delivered);
					}
					else
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					// Panniers don't have cargo.
					if(toHaulIndex.isAdjacentToActor(area, actor))
					{
						// Actor is at pickup location.
						// TODO: loading delay.
						toHaulIndex.reservable_maybeUnreserve(area, m_project.m_canReserve, m_quantity);
						items.cargo_loadPolymorphic(m_haulTool.getIndex(), toHaulIndex, m_quantity);
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaulIndex, detour);
				}
			}
			else
			{
				// Get beast panniers.
				if(actors.canPickUp_isCarryingItem(actor, m_haulTool.getIndex()))
				{
					// Actor has panniers.
					if(actors.isAdjacentToActor(actor, m_beastOfBurden.getIndex()))
					{
						// Actor can put on panniers.
						actors.canPickUp_removeItem(actor, m_haulTool.getIndex());
						actors.equipment_add(m_beastOfBurden.getIndex(), m_haulTool.getIndex());
						actors.followActor(m_beastOfBurden.getIndex(), actor);
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaulIndex, detour);
					}
					else
						actors.move_setDestinationAdjacentToActor(actor, m_beastOfBurden.getIndex(), detour);
				}
				else
				{
					// Bring panniers to beast.
					// TODO: move to adjacent to panniers.
					if(actors.isAdjacentToLocation(actor, items.getLocation(m_haulTool.getIndex())))
					{
						actors.canPickUp_pickUpItem(actor, m_haulTool.getIndex());
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToActor(actor, m_beastOfBurden.getIndex(), detour);
					}
					else
						actors.move_setDestinationAdjacentToItem(actor, m_haulTool.getIndex(), detour);
				}

			}
			break;
		case HaulStrategy::AnimalCart:
			//TODO: what if already attached to a different cart?
			assert(m_beastOfBurden.exists());
			assert(m_haulTool.exists());
			if(actors.isLeadingItem(m_beastOfBurden.getIndex(), m_haulTool.getIndex()))
			{
				// Beast has cart.
				hasCargo = m_genericItemType.exists()?
					items.cargo_containsItemGeneric(m_haulTool.getIndex(), m_genericItemType, m_genericMaterialType, m_quantity) :
					items.cargo_containsPolymorphic(m_haulTool.getIndex(), toHaulIndex);
				if(hasCargo)
				{
					// Cart has cargo.
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						// Actor is at destination.
						//TODO: unloading delay.
						//TODO: unfollow cart?
						ActorOrItemIndex delivered;
						if(m_genericItemType.empty())
						{
							items.cargo_unloadPolymorphicToLocation(m_haulTool.getIndex(), toHaulIndex, actorLocation, m_quantity);
							delivered = toHaulIndex;
						}
						else
						{
							ItemIndex item = items.cargo_unloadGenericItemToLocation(m_haulTool.getIndex(), m_genericItemType, m_genericMaterialType, actorLocation, m_quantity);
							delivered = ActorOrItemIndex::createForItem(item);
						}
						// TODO: set rotation?
						actors.leadAndFollowDisband(actor);
						complete(delivered);
					}
					else
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					// Cart doesn't have cargo.
					if(toHaulIndex.isAdjacentToActor(area, actor))
					{
						// Actor is at pickup location.
						// TODO: loading delay.
						toHaulIndex.reservable_maybeUnreserve(area, m_project.m_canReserve);
						items.cargo_loadPolymorphic(haulToolIndex, toHaulIndex, m_quantity);
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaulIndex, detour);
				}
			}
			else
			{
				// Bring beast to cart.
				if(actors.isLeadingActor(actor, m_beastOfBurden.getIndex()))
				{
					// Actor has beast.
					if(actors.isAdjacentToItem(actor, m_haulTool.getIndex()))
					{
						// Actor can harness beast to item.
						// Don't check if item is adjacent to beast, allow it to teleport.
						// TODO: Make not teleport.
						items.followActor(m_haulTool.getIndex(), m_beastOfBurden.getIndex());
						// Skip adjacent check, potentially teleport.
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaulIndex, detour);
					}
					else
						actors.move_setDestinationAdjacentToItem(actor, m_haulTool.getIndex(), detour);
				}
				else
				{
					// Get beast.
					if(actors.isAdjacentToActor(actor, m_beastOfBurden.getIndex()))
					{
						actors.followActor(m_beastOfBurden.getIndex(), actor);
						actors.canReserve_clearAll(actor);
						actors.canReserve_clearAll(m_beastOfBurden.getIndex());
						actors.move_setDestinationAdjacentToItem(actor, m_haulTool.getIndex(), detour);
					}
					else
						actors.move_setDestinationAdjacentToActor(actor, m_beastOfBurden.getIndex(), detour);
				}

			}
			break;
		case HaulStrategy::TeamCart:
			assert(m_haulTool.exists());
			// TODO: teams of more then 2, requires muliple followers.
			assert(m_workers.size() == 2);
			if(!m_leader.exists())
				m_leader.setTarget(actors.getReferenceTarget(actor));
			if(actors.isLeadingItem(actor, m_haulTool.getIndex()))
			{
				assert(actor == m_leader.getIndex());
				hasCargo = m_genericItemType.exists()?
					items.cargo_containsItemGeneric(m_haulTool.getIndex(), m_genericItemType, m_genericMaterialType, m_quantity) :
					items.cargo_containsPolymorphic(m_haulTool.getIndex(), toHaulIndex);
				if(hasCargo)
				{
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						ActorOrItemIndex delivered;
						if(m_genericItemType.empty())
						{
							items.cargo_unloadPolymorphicToLocation(m_haulTool.getIndex(), toHaulIndex, actorLocation, m_quantity);
							delivered = toHaulIndex;
						}
						else
						{
							ItemIndex item = items.cargo_unloadGenericItemToLocation(m_haulTool.getIndex(), m_genericItemType, m_genericMaterialType, actorLocation, m_quantity);
							delivered = ActorOrItemIndex::createForItem(item);
						}
						// TODO: set rotation?
						actors.leadAndFollowDisband(m_leader.getIndex());
						complete(delivered);
					}
					else
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					if(toHaulIndex.isAdjacentToActor(area, actor))
					{
						//TODO: set delay for loading.
						toHaulIndex.reservable_maybeUnreserve(area, m_project.m_canReserve);
						items.cargo_loadPolymorphic(m_haulTool.getIndex(), toHaulIndex, m_quantity);
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaulIndex, detour);
				}
			}
			else if(actors.isAdjacentToItem(actor, m_haulTool.getIndex()))
			{
				if(allWorkersAreAdjacentTo(m_haulTool.getIndex()))
				{
					// All actors are adjacent to the haul tool.
					items.followActor(m_haulTool.getIndex(), m_leader.getIndex());
					for(ActorReference follower : m_workers)
					{
						if(follower != m_leader)
							actors.followItem(follower.getIndex(), m_haulTool.getIndex());
						actors.canReserve_clearAll(follower.getIndex());
					}
					actors.canReserve_clearAll(m_leader.getIndex());
					actors.move_setDestinationAdjacentToPolymorphic(m_leader.getIndex(), toHaulIndex, detour);
				}
			}
			else
			{
				actors.canReserve_clearAll(actor);
				actors.move_setDestinationAdjacentToItem(actor, m_haulTool.getIndex(), detour);
			}
			break;
		case HaulStrategy::StrongSentient:
			//TODO
			break;
		case HaulStrategy::None:
			assert(false); // this method should only be called after a strategy is choosen.
	}
}
void HaulSubproject::addWorker(const ActorIndex& actor)
{
	ActorReference ref = m_project.m_area.getActors().getReference(actor);
	assert(!m_workers.contains(ref));
	m_workers.add(ref);
	commandWorker(actor);
}
void HaulSubproject::removeWorker(const ActorIndex& actor)
{
	ActorReference ref = m_project.m_area.getActors().getReference(actor);
	assert(m_workers.contains(ref));
	cancel();
}
void HaulSubproject::cancel()
{
	m_project.haulSubprojectCancel(*this);
}
bool HaulSubproject::allWorkersAreAdjacentTo(const ActorOrItemIndex& actorOrItem)
{
	return std::all_of(m_workers.begin(), m_workers.end(), [&](ActorReference worker) { return actorOrItem.isAdjacentToActor(m_project.m_area, worker.getIndex()); });
}
bool HaulSubproject::allWorkersAreAdjacentTo(const ItemIndex& index)
{
	return std::all_of(m_workers.begin(), m_workers.end(), [&](ActorReference worker) { return m_project.m_area.getItems().isAdjacentToActor(index, worker.getIndex()); });
}
HaulSubprojectParamaters HaulSubproject::tryToSetHaulStrategy(const Project& project, const ActorOrItemIndex& toHaul, const ActorIndex& worker)
{
	// TODO: make exception for slow haul if very close.
	Actors& actors = project.m_area.getActors();
	FactionId faction = actors.getFactionId(worker);
	HaulSubprojectParamaters output;
	ActorOrItemReference toHaulRef = toHaul.toReference(project.m_area);
	output.toHaul = toHaul.toReference(project.m_area);
	output.projectRequirementCounts = project.m_toPickup[toHaulRef].first;
	Quantity maxQuantityRequested = project.m_toPickup[toHaulRef].second;
	Speed minimumSpeed = project.getMinimumHaulSpeed();
	ActorIndices workers;
	// TODO: shouldn't this be m_waiting?
	for(auto& pair : project.m_workers)
		workers.add(pair.first.getIndex());
	assert(maxQuantityRequested != 0);
	// toHaul is wheeled.
	static MoveTypeId wheeled = MoveType::byName("roll");
	if(toHaul.getMoveType(project.m_area) == wheeled)
	{
		assert(toHaul.isItem());
		ItemIndex haulableItem = ItemIndex::cast(toHaul.get());
		ActorOrItemIndex workerPolymorphic = ActorOrItemIndex::createForActor(worker);
		std::vector<ActorOrItemIndex> list{workerPolymorphic, toHaul};
		if(Portables::getMoveSpeedForGroup(project.m_area, list) >= minimumSpeed)
		{
			output.strategy = HaulStrategy::IndividualCargoIsCart;
			output.quantity = Quantity::create(1);
			output.workers.add(worker);
			return output;
		}
		else
		{
			output.workers = actorsNeededToHaulAtMinimumSpeedWithTool(project, worker, toHaul, haulableItem);
			if(!output.workers.empty())
			{
				assert(output.workers.size() > 1);
				output.strategy = HaulStrategy::Team;
				output.quantity = Quantity::create(1);
				return output;
			}
		}
	}
	// Individual
	// TODO:: Prioritize cart if a large number of items are requested.
	Mass singleUnitMass = toHaul.getSingleUnitMass(project.m_area);
	Quantity maxQuantityCanCarry = actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(worker, singleUnitMass, minimumSpeed);
	if(maxQuantityCanCarry != 0)
	{
		output.strategy = HaulStrategy::Individual;
		output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
		output.workers.add(worker);
		return output;
	}
	// Team
	output.workers = actorsNeededToHaulAtMinimumSpeed(project, worker, toHaul);
	if(!output.workers.empty())
	{
		assert(output.workers.size() == 2); // TODO: teams larger then two.
		output.strategy = HaulStrategy::Team;
		output.quantity = Quantity::create(1);
		return output;
	}
	ItemIndex haulTool = project.m_area.m_hasHaulTools.getToolToHaulPolymorphic(project.m_area, faction, toHaul);
	// Cart
	if(haulTool.exists())
	{
		// Cart
		maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(project.m_area, worker, haulTool, toHaul, minimumSpeed);
		if(maxQuantityCanCarry != 0)
		{
			output.strategy = HaulStrategy::Cart;
			output.haulTool = haulTool;
			output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
			output.workers.add(worker);
			return output;
		}
		// Animal Cart
		ActorIndex yoked = project.m_area.m_hasHaulTools.getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(project.m_area, actors.getFactionId(worker), haulTool, toHaul.getMass(project.m_area), minimumSpeed);
		if(yoked.exists())
		{
			maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(project.m_area, worker, yoked, haulTool, toHaul, minimumSpeed);
			if(maxQuantityCanCarry != 0)
			{
				output.strategy = HaulStrategy::AnimalCart;
				output.haulTool = haulTool;
				output.beastOfBurden = yoked;
				output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
				output.workers.add(worker);
				return output;
			}
		}
		// Team Cart
		output.workers = actorsNeededToHaulAtMinimumSpeedWithTool(project, worker, toHaul, haulTool);
		if(!output.workers.empty())
		{
			assert(output.workers.size() > 1);
			output.strategy = HaulStrategy::TeamCart;
			output.haulTool = haulTool;
			output.quantity = Quantity::create(1);
			return output;
		}
	}
	// Panniers
	ActorIndex pannierBearer = project.m_area.m_hasHaulTools.getPannierBearerToHaulCargoWithMassWithMinimumSpeed(project.m_area, faction, toHaul, minimumSpeed);
	if(pannierBearer.exists())
	{
		//TODO: If pannierBearer already has panniers equiped then use those, otherwise find ones to use. Same for animalCart.
		ItemIndex panniers = project.m_area.m_hasHaulTools.getPanniersForActorToHaul(project.m_area, faction, pannierBearer, toHaul);
		if(panniers.exists())
		{
			maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithPanniersAndAnimal(project.m_area, worker, pannierBearer, panniers, toHaul, minimumSpeed);
			if(maxQuantityCanCarry != 0)
			{
				output.strategy = HaulStrategy::Panniers;
				output.beastOfBurden = pannierBearer;
				output.haulTool = panniers;
				output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
				output.workers.add(worker);
				return output;
			}
		}
	}
	assert(output.strategy == HaulStrategy::None);
	return output;
}
void HaulSubproject::complete(const ActorOrItemIndex& delivered)
{
	Actors& actors = m_project.m_area.getActors();
	Items& items = m_project.m_area.getItems();
	Project& project = m_project;
	if(m_haulTool.exists())
		items.reservable_unreserve(m_haulTool.getIndex(), m_project.m_canReserve);
	if(m_beastOfBurden.exists())
		actors.reservable_unreserve(m_beastOfBurden.getIndex(), m_project.m_canReserve);
	m_projectRequirementCounts.delivered += m_quantity;
	assert(m_projectRequirementCounts.delivered <= m_projectRequirementCounts.required);
	if(delivered.isItem())
	{
		ItemIndex deliveredIndex = ItemIndex::cast(delivered.get());
		// Reserve dropped off item with project.
		// Items are not reserved durring transit because reserved items don't move.
		items.reservable_reserve(deliveredIndex, m_project.getCanReserve(), m_quantity);
		if(m_projectRequirementCounts.consumed)
			m_project.m_toConsume.getOrInsert(items.getReference(deliveredIndex), Quantity::create(0)) += m_quantity;
		if(items.isWorkPiece(deliveredIndex))
			delivered.setLocationAndFacing(m_project.m_area, m_project.m_location, Facing::create(0));
		m_project.m_deliveredItems.maybeAdd(deliveredIndex, m_project.m_area);
	}
	else
		//TODO: deliver actors.
		assert(false);
	for(ActorReference worker : m_workers)
		actors.canReserve_clearAll(worker.getIndex());
	m_project.onDelivered(delivered);
	auto workers = std::move(m_workers);
	m_project.m_haulSubprojects.remove(*this);
	for(ActorReference worker : workers)
	{
		project.m_workers[worker].haulSubproject = nullptr;
		project.commandWorker(worker.getIndex());
	}
}
// Class method.
ActorIndices HaulSubproject::actorsNeededToHaulAtMinimumSpeed(const Project& project, const ActorIndex& leader, const ActorOrItemIndex& toHaul)
{
	std::vector<ActorOrItemIndex> actorsAndItems;
	ActorIndices output;
	output.add(leader);
	actorsAndItems.push_back(ActorOrItemIndex::createForActor(leader));
	actorsAndItems.push_back(toHaul);
	Actors& actors = project.m_area.getActors();
	// For each actor without a sub project add to actors and items and see if the item can be moved fast enough.
	for(auto& [actor, projectWorker] : project.m_workers)
	{
		ActorIndex actorIndex = actor.getIndex();
		if(actorIndex == leader || projectWorker.haulSubproject != nullptr || !actors.isSentient(actorIndex))
			continue;
		assert(std::ranges::find(output, actorIndex) == output.end());
		output.add(actorIndex);
		if(output.size() > 2) //TODO: More then two requires multiple followers for one leader.
			break;
		actorsAndItems.push_back(ActorOrItemIndex::createForActor(actorIndex));
		Speed speed = Portables::getMoveSpeedForGroup(project.m_area, actorsAndItems);
		if(speed >= project.getMinimumHaulSpeed())
			return output;
	}
	// Cannot form a team to move at acceptable speed.
	output.clear();
	return output;
}
//Class method.
Speed HaulSubproject::getSpeedWithHaulToolAndCargo(const Area& area, const ActorIndex& leader, const ItemIndex& haulTool, const ActorOrItemIndex& toHaul, const Quantity& quantity)
{
	std::vector<ActorOrItemIndex> actorsAndItems;
	actorsAndItems.push_back(ActorOrItemIndex::createForActor(leader));
	actorsAndItems.push_back(ActorOrItemIndex::createForItem(haulTool));
	// actorsAndItems, rollingMass, deadMass
	return Portables::getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, toHaul.getSingleUnitMass(area) * quantity, Mass::create(0));
}
// Class method.
Quantity HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(const Area& area, const ActorIndex& leader, const ItemIndex& haulTool, const ActorOrItemIndex& toHaul, const Speed& minimumSpeed)
{
	assert(minimumSpeed != 0);
	Quantity quantity = Quantity::create(0);
	while(getSpeedWithHaulToolAndCargo(area, leader, haulTool, toHaul, quantity + 1) >= minimumSpeed)
		++quantity;
	return quantity;
}
// Class method.
Speed HaulSubproject::getSpeedWithHaulToolAndAnimal(const Area& area, const ActorIndex& leader, const ActorIndex& yoked, const ItemIndex& haulTool, const ActorOrItemIndex& toHaul, const Quantity& quantity)
{
	std::vector<ActorOrItemIndex> actorsAndItems;
	actorsAndItems.push_back(ActorOrItemIndex::createForActor(leader));
	actorsAndItems.push_back(ActorOrItemIndex::createForActor(yoked));
	actorsAndItems.push_back(ActorOrItemIndex::createForItem(haulTool));
	// actorsAndItems, rollingMass, deadMass
	return Portables::getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, toHaul.getSingleUnitMass(area) * quantity, Mass::create(0));
}
// Class method.
Quantity HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(const Area& area, const ActorIndex& leader, const ActorIndex& yoked, const ItemIndex& haulTool, const ActorOrItemIndex& toHaul, const Speed& minimumSpeed)
{
	assert(minimumSpeed != 0);
	Quantity quantity = Quantity::create(0);
	while(getSpeedWithHaulToolAndAnimal(area, leader, yoked, haulTool, toHaul, quantity + 1) >= minimumSpeed)
		++quantity;
	return quantity;
}
// Class method.
ActorIndices HaulSubproject::actorsNeededToHaulAtMinimumSpeedWithTool(const Project& project, const ActorIndex& leader, const ActorOrItemIndex& toHaul, const ItemIndex& haulTool)
{
	std::vector<ActorOrItemIndex> actorsAndItems;
	ActorIndices output;
	output.add(leader);
	actorsAndItems.push_back(ActorOrItemIndex::createForActor(leader));
	actorsAndItems.push_back(ActorOrItemIndex::createForItem(haulTool));
	const Actors& actors = project.m_area.getActors();
	// For each actor without a sub project add to actors and items and see if the item can be moved fast enough.
	for(auto& [actor, projectWorker] : project.m_workers)
	{
		ActorIndex actorIndex = actor.getIndex();
		if(actorIndex == leader || projectWorker.haulSubproject != nullptr || !actors.isSentient(actorIndex))
			continue;
		assert(std::ranges::find(output, actorIndex) == output.end());
		output.add(actorIndex);
		if(output.size() > 2) //TODO: More then two requires multiple followers for one leader.
			break;
		actorsAndItems.push_back(ActorOrItemIndex::createForActor(actorIndex));
		// If to haul is also haul tool then there is no cargo, we are hauling the haulTool itself.
		Mass mass = toHaul == ActorOrItemIndex::createForItem(haulTool) ? Mass::create(0) : toHaul.getSingleUnitMass(project.m_area);
		Speed speed = Portables::getMoveSpeedForGroupWithAddedMass(project.m_area, actorsAndItems, mass, Mass::create(0));
		if(speed >= project.getMinimumHaulSpeed())
			return output;
	}
	// Cannot form a team to move at acceptable speed.
	output.clear();
	return output;
}
// Class method.
Speed HaulSubproject::getSpeedWithPannierBearerAndPanniers(const Area& area, const ActorIndex& leader, const ActorIndex& pannierBearer, const ItemIndex& panniers, const ActorOrItemIndex& toHaul, const Quantity& quantity)
{
	std::vector<ActorOrItemIndex> actorsAndItems;
	actorsAndItems.push_back(ActorOrItemIndex::createForActor(leader));
	actorsAndItems.push_back(ActorOrItemIndex::createForActor(pannierBearer));
	// actorsAndItems, rollingMass, deadMass
	return Portables::getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, Mass::create(0), (toHaul.getSingleUnitMass(area) * quantity) + area.getItems().getMass(panniers));
}
// Class method.
Quantity HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithPanniersAndAnimal(const Area& area, const ActorIndex& leader, const ActorIndex& pannierBearer, const ItemIndex& panniers, const ActorOrItemIndex& toHaul, const Speed& minimumSpeed)
{
	assert(minimumSpeed != 0);
	Quantity quantity = Quantity::create(0);
	while(getSpeedWithPannierBearerAndPanniers(area, leader, pannierBearer, panniers, toHaul, quantity + 1) >= minimumSpeed)
		++quantity;
	return quantity;
}
//TODO: optimize?
bool AreaHasHaulTools::hasToolToHaulItem(const Area& area, const FactionId& faction, const ItemIndex& item) const
{
	return getToolToHaulItem(area, faction, item).exists();
}
bool AreaHasHaulTools::hasToolToHaulActor(const Area& area, const FactionId& faction, const ActorIndex& actor) const
{
	return getToolToHaulActor(area, faction, actor).exists();
}
bool AreaHasHaulTools::hasToolToHaulPolymorphic(const Area& area, const FactionId& faction, const ActorOrItemIndex& hasShape) const
{
	return getToolToHaulPolymorphic(area, faction, hasShape).exists();
}
ItemIndex AreaHasHaulTools::getToolToHaulItem(const Area& area, const FactionId& faction, const ItemIndex& item) const
{
	Volume volume = area.getItems().getVolume(item);
	return getToolToHaulVolume(area, faction, volume);
}
ItemIndex AreaHasHaulTools::getToolToHaulActor(const Area& area, const FactionId& faction, const ActorIndex& actor) const
{
	Volume volume = area.getActors().getVolume(actor);
	return getToolToHaulVolume(area, faction, volume);
}
ItemIndex AreaHasHaulTools::getToolToHaulPolymorphic(const Area& area, const FactionId& faction, const ActorOrItemIndex& toHaul) const
{
	Volume volume = toHaul.getVolume(area);
	return getToolToHaulVolume(area, faction, volume);
}
ItemIndex AreaHasHaulTools::getToolToHaulVolume(const Area& area, const FactionId& faction, const Volume& volume) const
{
	// Items like panniers with no move type also have internal volume but aren't relevent for this method.
	static MoveTypeId none = MoveType::byName("none");
	const Items& items = area.getItems();
	for(ItemReference item : m_haulTools)
	{
		ItemIndex index = item.getIndex();
		ItemTypeId itemType = area.getItems().getItemType(index);
		if(ItemType::getMoveType(itemType) != none && !items.reservable_isFullyReserved(index, faction) && ItemType::getInternalVolume(itemType) >= volume)
			return index;
	}
	return ItemIndex::null();
}
bool AreaHasHaulTools::hasToolToHaulFluid(const Area& area, const FactionId& faction) const
{
	return getToolToHaulFluid(area, faction).exists();
}
ItemIndex AreaHasHaulTools::getToolToHaulFluid(const Area& area, const FactionId& faction) const
{
	const Items& items = area.getItems();
	for(ItemReference item : m_haulTools)
	{
		ItemIndex index = item.getIndex();
		if(!items.reservable_isFullyReserved(index, faction) && ItemType::getCanHoldFluids(items.getItemType(index)))
			return index;
	}
	return ItemIndex::null();
}
ActorIndex AreaHasHaulTools::getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(const Area& area, const FactionId& faction, const ItemIndex& haulTool, const Mass& cargoMass, const Speed& minimumHaulSpeed) const
{
	[[maybe_unused]] static MoveTypeId rollingType = MoveType::byName("roll");
	assert(area.getItems().getMoveType(haulTool) == rollingType);
	const Actors& actors = area.getActors();
	for(ActorReference actor : m_yolkableActors)
	{
		ActorIndex index = actor.getIndex();
		std::vector<ActorOrItemIndex> shapes = { ActorOrItemIndex::createForActor(index), ActorOrItemIndex::createForItem(haulTool) };
		if(!actors.reservable_isFullyReserved(index, faction) && minimumHaulSpeed <= Portables::getMoveSpeedForGroupWithAddedMass(area, shapes, cargoMass, Mass::create(0)))
			return index;
	}
	return ActorIndex::null();
}
ActorIndex AreaHasHaulTools::getPannierBearerToHaulCargoWithMassWithMinimumSpeed(const Area& area, const FactionId& faction, const ActorOrItemIndex& hasShape, const Speed& minimumHaulSpeed) const
{
	//TODO: Account for pannier mass?
	const Actors& actors = area.getActors();
	for(ActorReference actor : m_yolkableActors)
	{
		ActorIndex index = actor.getIndex();
		if(!actors.reservable_isFullyReserved(index, faction) && minimumHaulSpeed <= actors.canPickUp_speedIfCarryingQuantity(index, hasShape.getMass(area), Quantity::create(1u)))
			return index;
	}
	return ActorIndex::null();
}
ItemIndex AreaHasHaulTools::getPanniersForActorToHaul(const Area& area, const FactionId& faction, const ActorIndex& actor, const ActorOrItemIndex& toHaul) const
{
	const Items& items = area.getItems();
	const Actors& actors = area.getActors();
	for(ItemReference item : m_haulTools)
	{
		ItemIndex index = item.getIndex();
		if(!items.reservable_isFullyReserved(index, faction) && ItemType::getInternalVolume(items.getItemType(index)) >= toHaul.getVolume(area) && actors.equipment_canEquipCurrently(actor, index))
			return index;
	}
	return ItemIndex::null();
}
void AreaHasHaulTools::registerHaulTool(const Area& area, const ItemIndex& item)
{
	const ItemReference ref = area.getItems().getReferenceConst(item);
	assert(!m_haulTools.contains(ref));
	assert(ItemType::getInternalVolume(area.getItems().getItemType(item)) != 0);
	m_haulTools.add(ref);
}
void AreaHasHaulTools::registerYokeableActor(const Area& area, const ActorIndex& actor)
{
	ActorReference ref = area.getActors().getReference(actor);
	assert(!m_yolkableActors.contains(ref));
	m_yolkableActors.add(ref);
}
void AreaHasHaulTools::unregisterHaulTool(const Area& area, const ItemIndex& item)
{
	const ItemReference ref = area.getItems().getReferenceConst(item);
	assert(m_haulTools.contains(ref));
	m_haulTools.remove(ref);
}
void AreaHasHaulTools::unregisterYokeableActor(const Area& area, const ActorIndex& actor)
{
	ActorReference ref = area.getActors().getReference(actor);
	assert(m_yolkableActors.contains(ref));
	m_yolkableActors.remove(ref);
}
