#include "haul.h"
#include "actorOrItemIndex.h"
#include "actors/actors.h"
#include "items/items.h"
#include "space/space.h"
#include "config.h"
#include "area/area.h"
#include "project.h"
#include "reference.h"
#include "reservable.h"
#include "simulation/hasItems.h"
#include "simulation/hasActors.h"
#include "simulation/simulation.h"
#include "definitions/itemType.h"
#include "definitions/moveType.h"
#include "definitions/materialType.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"
#include "portables.hpp"

#include <memory>
#include <sys/types.h>
#include <algorithm>
void HaulSubprojectParamaters::reset()
{
	toHaul.clear();
	fluidType.clear();
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
	if(toHaul.isItem())
		assert(projectRequirementCounts != nullptr);
	assert(!workers.empty());
	FactionId faction = area.getActors().getFaction(workers.front());
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
	m_project(p),
	m_projectRequirementCounts(paramaters.projectRequirementCounts),
	m_toHaul(paramaters.toHaul),
	m_fluidType(paramaters.fluidType),
	m_quantity(paramaters.quantity),
	m_strategy(paramaters.strategy)
{
	Area& area = m_project.m_area;
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	assert(!paramaters.workers.empty());
	for(ActorIndex actor : paramaters.workers)
		m_workers.insert(actors.getReference(actor));
	if(paramaters.haulTool.exists())
	{
		m_haulTool.setIndex(paramaters.haulTool, items.m_referenceData);
		std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<HaulSubprojectDishonorCallback>(*this);
		items.reservable_reserve(paramaters.haulTool, m_project.m_canReserve, Quantity::create(1u), std::move(dishonorCallback));
	}
	if(paramaters.beastOfBurden.exists())
	{
		m_beastOfBurden.setIndex(paramaters.beastOfBurden, actors.m_referenceData);
		std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<HaulSubprojectDishonorCallback>(*this);
		actors.reservable_reserve(paramaters.beastOfBurden, m_project.m_canReserve, Quantity::create(1u), std::move(dishonorCallback));
	}
	if(m_toHaul.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData).isGeneric(area))
	{
		m_genericItemType = items.getItemType(ItemIndex::cast(m_toHaul.getIndex(actors.m_referenceData, items.m_referenceData)));
		m_genericMaterialType = items.getMaterialType(ItemIndex::cast(m_toHaul.getIndex(actors.m_referenceData, items.m_referenceData)));
	}
	for(ActorReference actor : m_workers)
	{
		m_project.m_workers[actor].haulSubproject = this;
		commandWorker(actor.getIndex(actors.m_referenceData));
	}
}
HaulSubproject::HaulSubproject(const Json& data, Project& p, DeserializationMemo& deserializationMemo) :
	m_project(p),
	m_projectRequirementCounts(&deserializationMemo.projectRequirementCountsReference(data["requirementCounts"])),
	m_fluidType(data["fluidType"].get<FluidTypeId>()),
	m_quantity(data["quantity"].get<Quantity>()),
	m_strategy(data["haulStrategy"].get<HaulStrategy>()),
	m_itemIsMoving(data["itemIsMoving"].get<bool>())
{
	Area& area = m_project.m_area;
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	if(data.contains("toHaul"))
		m_toHaul.load(data["toHaul"], area);
	if(data.contains("haulTool"))
		m_haulTool.load(data["haulTool"], items.m_referenceData);
	if(data.contains("leader"))
		m_leader.load(data["leader"], actors.m_referenceData);
	if(data.contains("beastOfBurden"))
		m_beastOfBurden.load(data["beastOfBurden"], actors.m_referenceData);
	if(data.contains("genericItemType"))
		m_genericItemType = ItemType::byName(data["genericItemType"].get<std::string>());
	if(data.contains("genericMaterialType"))
		m_genericMaterialType = MaterialType::byName(data["genericMaterialType"].get<std::string>());
	if(data.contains("workers"))
		for(const Json& workerIndex : data["workers"])
			m_workers.insert(actors.getReference(workerIndex.get<ActorIndex>()));
	if(data.contains("nonsentients"))
		for(const Json& actorIndex : data["nonsentients"])
			m_nonsentients.insert(actors.getReference(actorIndex.get<ActorIndex>()));
	if(data.contains("liftPoints"))
		for(const Json& liftPointData : data["liftPoints"])
		{
			ActorIndex actor = liftPointData[0].get<ActorIndex>();
			m_liftPoints[actors.getReference(actor)] = liftPointData[1].get<Point3D>();
		}
	deserializationMemo.m_haulSubprojects[data["address"].get<uintptr_t>()] = this;
}
Json HaulSubproject::toJson() const
{
	Json data({
		{"quantity", m_quantity},
		{"haulStrategy", m_strategy},
		{"itemIsMoving", m_itemIsMoving},
		{"address", reinterpret_cast<uintptr_t>(this)},
		{"requirementCounts", reinterpret_cast<uintptr_t>(m_projectRequirementCounts)},
		{"fluidType", m_fluidType}
	});
	data["toHaul"] = m_toHaul.toJson();
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
		for(auto [actor, point] : m_liftPoints)
			data["liftPoints"].push_back(Json::array({actor, point}));
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
	Space& space =  area.getSpace();
	assert(actors.objective_exists(actor));
	Point3D actorLocation = actors.getLocation(actor);
	FactionId faction = m_project.getFaction();
	ActorOrItemIndex toHaul = m_toHaul.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData);
	ItemIndex haulTool;
	ActorIndex beastOfBurden;
	if(m_haulTool.exists())
		haulTool = m_haulTool.getIndex(items.m_referenceData);
	else
		haulTool.clear();
	switch(m_strategy)
	{
		case HaulStrategy::Individual:
			assert(m_workers.size() == 1);
			hasCargo = m_genericItemType.exists() ?
				actors.canPickUp_isCarryingItemGeneric(actor, m_genericItemType, m_genericMaterialType, m_quantity) :
				actors.canPickUp_isCarryingPolymorphic(actor, m_toHaul.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData));
			if(hasCargo)
			{
				if(m_fluidType.exists())
				{
					// If a fluid type is specified then toHaul must be a contaner which can hold fluids.
					assert(toHaul.isItem());
					ItemIndex toHaulIndex = toHaul.getItem();
					assert(ItemType::getCanHoldFluids(items.getItemType(toHaulIndex)));
					if(items.cargo_getFluidVolume(toHaulIndex) != m_quantity.get())
					{
						// Actor attempts to fill toHaul with fluid from adjacent space.
						// quantity does dual duty as volume when hauling fluids.
						actors.canPickUp_addFluidToContainerFromAdjacentPointsIncludingOtherContainersWithLimit(actor, m_fluidType, CollisionVolume::create(m_quantity.get()));
						if(!items.cargo_containsFluidType(toHaulIndex, m_fluidType))
						{
							// No fluid found in adjacent space, set destination to create a path request.
							actors.move_setDestinationAdjacentToFluidType(actor, m_fluidType);
							return;
						}
					}
				}
				if(actors.isAdjacentToLocation(actor, m_project.m_location))
				{
					// Unload
					ActorOrItemIndex cargo = actors.canPickUp_getCarrying(actor);
					actors.canPickUp_remove(actor, cargo);
					complete(cargo);
				}
				else
					actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
			}
			else
			{
				if(toHaul.isAdjacentToActor(area, actor))
				{
					actors.canReserve_clearAll(actor);
					toHaul.reservable_unreserve(area, m_project.m_canReserve, m_quantity);
					// The actor may pick up a smaller stack of generic items from a bigger one, in whihc case we need to change all our ItemReferences to that smaller stack, so as to release the reservation on the remainder stack.
					toHaul = actors.canPickUp_pickUpPolymorphic(actor, toHaul, m_quantity);
					m_toHaul = toHaul.toReference(area);
					m_project.onPickUpRequired(toHaul);
					commandWorker(actor);
				}
				else
					actors.move_setDestinationAdjacentToPolymorphic(actor, toHaul, detour);
			}
			break;
		case HaulStrategy::IndividualCargoIsCart:
			assert(m_workers.size() == 1);
			if(actors.isLeadingPolymorphic(actor, toHaul))
			{
				Point3D projectLocation = m_project.m_location;
				if(actors.isAdjacentToLocation(actor, projectLocation))
				{
					// Unload
					toHaul.unfollow(area);
					complete(toHaul);
				}
				else
					actors.move_setDestinationAdjacentToLocation(actor, projectLocation);
			}
			else
			{
				if(toHaul.isAdjacentToActor(area, actor))
				{
					actors.canReserve_clearAll(actor);
					// Reservation is held project, not by actor.
					toHaul.reservable_unreserve(area, m_project.m_canReserve, m_quantity);
					toHaul.followActor(area, actor);
					commandWorker(actor);
				}
				else
					actors.move_setDestinationAdjacentToPolymorphic(actor, toHaul, detour);
			}
			break;
		case HaulStrategy::Team:
			// TODO: teams of more then 2, requires muliple followers.
			assert(m_workers.size() == 2);
			if(!m_leader.exists())
				m_leader.setIndex(actor, actors.m_referenceData);
			if(m_itemIsMoving)
			{
				assert(actor == m_leader.getIndex(actors.m_referenceData));
				if(actors.isAdjacentToLocation(actor, m_project.m_location))
				{
					// At destination.
					// TODO: disband sets toHaul as static but then we call clearStatic right after, so setting from dynamic to static and then clearing static is waseful.
					actors.leadAndFollowDisband(actor);
					toHaul.location_clearStatic(area);
					complete(toHaul);
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
						if(allWorkersAreAdjacentTo(toHaul))
						{
							// All actors are at lift points.
							assert(m_quantity == 1);
							assert(toHaul.getQuantity(area) == 1);
							// TODO: make work for generic stacks.
							ActorIndex leaderIndex = m_leader.getIndex(actors.m_referenceData);
							toHaul.followActor(area, leaderIndex);
							assert(!toHaul.isStatic(area));
							for(ActorReference follower : m_workers)
							{
								ActorIndex followerIndex = follower.getIndex(actors.m_referenceData);
								if(follower != m_leader)
									actors.followPolymorphic(followerIndex, toHaul);
								actors.canReserve_clearAll(followerIndex);
							}
							actors.move_setDestinationAdjacentToLocation(leaderIndex, m_project.m_location, detour);
							m_itemIsMoving = true;
							toHaul.reservable_unreserve(area, m_project.m_canReserve, m_quantity);
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
					for(Point3D point : toHaul.getAdjacentPoints(area))
					{
						Facing4 facing = point.getFacingTwords(m_project.m_location);
						if(space.shape_shapeAndMoveTypeCanEnterEverWithFacing(point, actors.getShape(actor), actors.getMoveType(actor), facing) && !space.isReserved(point, faction))
						{
							m_liftPoints.insert(ref, point);
							actors.canReserve_reserveLocation(actor, point);
							// Destination, detour, adjacent, unreserved, reserve
							actors.move_setDestination(actor, point, detour);
							return;
						}
					}
					std::unreachable(); // team strategy should not be choosen if there is not enough space to reserve for lifting.
				}
			}
			break;
			//TODO: Reserve destinations?
		case HaulStrategy::Cart:
			assert(m_haulTool.exists());
			haulTool = m_haulTool.getIndex(items.m_referenceData);
			if(m_toHaul.isItem())
				assert(m_haulTool != m_toHaul.toItemReference());
			if(actors.isLeadingItem(actor, haulTool))
			{
				// Has cart.
				hasCargo = m_genericItemType.exists() ?
					items.cargo_containsItemGeneric(haulTool, m_genericItemType, m_genericMaterialType, m_quantity) :
					items.cargo_containsPolymorphic(haulTool, toHaul);
				if(hasCargo)
				{
					// Cart is loaded.
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						// At drop off point.
						items.unfollow(haulTool);
						items.cargo_remove(haulTool, toHaul);
						// TODO: set rotation?
						complete(toHaul);
					}
					else
						// Go to drop off point.
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					// Cart not loaded.
					if(toHaul.isAdjacentToActor(area, actor))
					{
						// Can load here.
						//TODO: set delay for loading.
						toHaul.reservable_unreserve(area, m_project.m_canReserve, m_quantity);
						actors.canReserve_clearAll(actor);
						toHaul = items.cargo_loadPolymorphic(haulTool, toHaul, m_quantity);
						m_toHaul = toHaul.toReference(area);
						m_project.onPickUpRequired(toHaul);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						// Go somewhere to load.
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaul, detour);
				}
			}
			else
			{
				// Don't have Cart.
				if(actors.isAdjacentToItem(actor, haulTool))
				{
					// Cart is here.
					items.followActor(haulTool, actor);
					actors.canReserve_clearAll(actor);
					actors.move_setDestinationAdjacentToPolymorphic(actor, toHaul, detour);
				}
				else
					// Go to cart.
					actors.move_setDestinationAdjacentToItem(actor, haulTool, detour);
			}
			break;
		case HaulStrategy::Panniers:
			assert(m_beastOfBurden.exists());
			assert(m_haulTool.exists());
			haulTool = m_haulTool.getIndex(items.m_referenceData);
			beastOfBurden = m_beastOfBurden.getIndex(actors.m_referenceData);
			if(actors.equipment_containsItem(beastOfBurden, haulTool))
			{
				// Beast has panniers.
				if(!actors.isFollowing(beastOfBurden))
				{
					// Special case where the beast already had the haul item before the task began.
					// Different from when the actor must fetch the haul item first.
					assert(!actors.isLeading(actor));
					if(actors.isAdjacentToActor(actor, beastOfBurden))
					{
						actors.followActor(beastOfBurden, actor);
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaul, detour);
					}
					else
						actors.move_setDestinationAdjacentToActor(actor, beastOfBurden, detour);
					return;
				}
				else
					assert(actors.getLeader(beastOfBurden).getActor() == actor);
				hasCargo = items.cargo_containsPolymorphic(haulTool, toHaul, m_quantity);
				if(hasCargo)
				{
					// Panniers have cargo.
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						// Actor is at destination.
						//TODO: unloading delay.
						items.cargo_remove(haulTool, toHaul);
						actors.unfollow(beastOfBurden);
						complete(toHaul);
					}
					else
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					// Panniers don't have cargo.
					if(toHaul.isAdjacentToActor(area, actor))
					{
						// Actor is at pickup location.
						// TODO: loading delay.
						toHaul.reservable_maybeUnreserve(area, m_project.m_canReserve, m_quantity);
						actors.canReserve_clearAll(actor);
						toHaul = items.cargo_loadPolymorphic(haulTool, toHaul, m_quantity);
						m_toHaul = toHaul.toReference(area);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaul, detour);
				}
			}
			else
			{
				// Get beast panniers.
				if(actors.canPickUp_isCarryingItem(actor, haulTool))
				{
					// Actor has panniers.
					if(actors.isAdjacentToActor(actor, beastOfBurden))
					{
						// Actor can put on panniers.
						actors.canPickUp_removeItem(actor, haulTool);
						actors.equipment_add(beastOfBurden, haulTool);
						actors.followActor(beastOfBurden, actor);
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaul, detour);
					}
					else
						actors.move_setDestinationAdjacentToActor(actor, beastOfBurden, detour);
				}
				else
				{
					// Bring panniers to beast.
					// TODO: move to adjacent to panniers.
					if(actors.isAdjacentToLocation(actor, items.getLocation(haulTool)))
					{
						actors.canPickUp_pickUpItem(actor, haulTool);
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToActor(actor, beastOfBurden, detour);
					}
					else
						actors.move_setDestinationAdjacentToItem(actor, haulTool, detour);
				}

			}
			break;
		case HaulStrategy::AnimalCart:
			//TODO: what if already attached to a different cart?
			assert(m_beastOfBurden.exists());
			assert(m_haulTool.exists());
			haulTool = m_haulTool.getIndex(items.m_referenceData);
			beastOfBurden = m_beastOfBurden.getIndex(actors.m_referenceData);
			if(actors.isLeadingItem(beastOfBurden, haulTool))
			{
				// Beast has cart.
				hasCargo = m_genericItemType.exists()?
					items.cargo_containsItemGeneric(haulTool, m_genericItemType, m_genericMaterialType, m_quantity) :
					items.cargo_containsPolymorphic(haulTool, toHaul);
				if(hasCargo)
				{
					assert(!toHaul.reservable_exists(area, faction));
					// Cart has cargo.
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						// Actor is at destination.
						//TODO: unloading delay.
						items.cargo_remove(haulTool, toHaul);
						actors.leadAndFollowDisband(actor);
						complete(toHaul);
					}
					else
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					// Cart doesn't have cargo.
					if(toHaul.isAdjacentToActor(area, actor))
					{
						// Actor is at pickup location.
						// TODO: loading delay.
						toHaul.reservable_unreserve(area, m_project.m_canReserve, m_quantity);
						actors.canReserve_clearAll(actor);
						toHaul = items.cargo_loadPolymorphic(haulTool, toHaul, m_quantity);
						assert(!toHaul.reservable_exists(area, faction));
						m_toHaul = toHaul.toReference(area);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaul, detour);
				}
			}
			else
			{
				// Bring beast to cart.
				if(actors.isLeadingActor(actor, beastOfBurden))
				{
					// Actor has beast.
					if(actors.isAdjacentToItem(actor, haulTool))
					{
						// Actor can harness beast to item.
						// Don't check if item is adjacent to beast, allow it to teleport.
						// TODO: Make not teleport.
						items.followActor(haulTool, beastOfBurden);
						// Skip adjacent check, potentially teleport.
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaul, detour);
					}
					else
						actors.move_setDestinationAdjacentToItem(actor, haulTool, detour);
				}
				else
				{
					// Get beast.
					if(actors.isAdjacentToActor(actor, beastOfBurden))
					{
						actors.followActor(beastOfBurden, actor);
						actors.canReserve_clearAll(actor);
						actors.canReserve_clearAll(beastOfBurden);
						actors.move_setDestinationAdjacentToItem(actor, haulTool, detour);
					}
					else
						actors.move_setDestinationAdjacentToActor(actor, beastOfBurden, detour);
				}

			}
			break;
		case HaulStrategy::TeamCart:
			assert(m_haulTool.exists());
			haulTool = m_haulTool.getIndex(items.m_referenceData);
			// TODO: teams of more then 2, requires muliple followers.
			assert(m_workers.size() == 2);
			if(!m_leader.exists())
				m_leader.setIndex(actor, actors.m_referenceData);
			if(actors.isLeadingItem(actor, haulTool))
			{
				assert(actor == m_leader.getIndex(actors.m_referenceData));
				hasCargo = m_genericItemType.exists()?
					items.cargo_containsItemGeneric(haulTool, m_genericItemType, m_genericMaterialType, m_quantity) :
					items.cargo_containsPolymorphic(haulTool, toHaul);
				if(hasCargo)
				{
					if(actors.isAdjacentToLocation(actor, m_project.m_location))
					{
						items.cargo_remove(haulTool, toHaul);
						// TODO: set rotation?
						actors.leadAndFollowDisband(actor);
						complete(toHaul);
					}
					else
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
				}
				else
				{
					if(toHaul.isAdjacentToActor(area, actor))
					{
						//TODO: set delay for loading.
						toHaul.reservable_maybeUnreserve(area, m_project.m_canReserve);
						toHaul = items.cargo_loadPolymorphic(haulTool, toHaul, m_quantity);
						m_toHaul = toHaul.toReference(area);
						actors.canReserve_clearAll(actor);
						actors.move_setDestinationAdjacentToLocation(actor, m_project.m_location, detour);
					}
					else
						actors.move_setDestinationAdjacentToPolymorphic(actor, toHaul, detour);
				}
			}
			else if(actors.isAdjacentToItem(actor, haulTool))
			{
				if(allWorkersAreAdjacentTo(haulTool))
				{
					// All actors are adjacent to the haul tool.
					ActorIndex leaderIndex = m_leader.getIndex(actors.m_referenceData);
					items.followActor(haulTool, leaderIndex);
					for(ActorReference follower : m_workers)
					{
						ActorIndex workerIndex = follower.getIndex(actors.m_referenceData);
						if(follower != m_leader)
							actors.followItem(workerIndex, haulTool);
						actors.canReserve_clearAll(workerIndex);
					}
					actors.move_setDestinationAdjacentToPolymorphic(leaderIndex, toHaul, detour);
				}
			}
			else
			{
				actors.move_setDestinationAdjacentToItem(actor, haulTool, detour);
			}
			break;
		case HaulStrategy::StrongSentient:
			//TODO
			break;
		case HaulStrategy::None:
			std::unreachable(); // this method should only be called after a strategy is choosen.
	}
}
void HaulSubproject::addWorker(const ActorIndex& actor)
{
	ActorReference ref = m_project.m_area.getActors().getReference(actor);
	assert(!m_workers.contains(ref));
	m_workers.insert(ref);
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
	//TODO: use std::all_of
	auto& referenceData = m_project.m_area.getActors().m_referenceData;
	for(ActorReference worker : m_workers)
		if(!actorOrItem.isAdjacentToActor(m_project.m_area, worker.getIndex(referenceData)))
			return false;
	return true;
}
bool HaulSubproject::allWorkersAreAdjacentTo(const ItemIndex& index)
{
	//TODO: use std::all_of
	auto& referenceData = m_project.m_area.getActors().m_referenceData;
	Items& items = m_project.m_area.getItems();
	for(ActorReference worker : m_workers)
		if(!items.isAdjacentToActor(index, worker.getIndex(referenceData)))
			return false;
	return true;
	//return std::all_of(m_workers.begin(), m_workers.end(), [&](const ActorReference& worker) { return m_project.m_area.getItems().isAdjacentToActor(index, worker.getIndex(referenceData)); });
}
HaulSubprojectParamaters HaulSubproject::tryToSetHaulStrategy(Project& project, const ActorOrItemReference& toHaulRef, const ActorIndex& worker, const FluidTypeId& fluidType, const CollisionVolume& fluidVolume)
{
	// TODO: make exception for slow haul if very close.
	Actors& actors = project.m_area.getActors();
	Items& items = project.m_area.getItems();
	FactionId faction = actors.getFaction(worker);
	HaulSubprojectParamaters output;
	output.toHaul = toHaulRef;
	Quantity maxQuantityRequested = Quantity::create(1);
	if(toHaulRef.isItem())
	{
		ItemReference item = toHaulRef.toItemReference();
		if(fluidType.exists())
		{
			output.projectRequirementCounts = &project.m_requiredFluids[fluidType].counts;
		}
		else
		{
			output.projectRequirementCounts = project.m_itemsToPickup[item].first;
			maxQuantityRequested = project.m_itemsToPickup[item].second;
		}
	}
	Speed minimumSpeed = project.getMinimumHaulSpeed();
	SmallSet<ActorIndex> workers;
	// TODO: shouldn't this be m_waiting?
	for(auto& pair : project.m_workers)
		workers.insert(pair.first.getIndex(actors.m_referenceData));
	assert(maxQuantityRequested != 0);
	// toHaul is wheeled. Either do IndividuaCargoIsCart or Team.
	static MoveTypeId wheeled = MoveType::byName("roll");
	ActorOrItemIndex toHaul = toHaulRef.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData);
	if(toHaul.getMoveType(project.m_area) == wheeled)
	{
		assert(toHaulRef.isItem());
		ItemIndex haulableItem = toHaul.getItem();
		ActorOrItemIndex workerPolymorphic = ActorOrItemIndex::createForActor(worker);
		std::vector<ActorOrItemIndex> list{workerPolymorphic, toHaul};
		if(PortablesHelpers::getMoveSpeedForGroup(project.m_area, list) >= minimumSpeed)
		{
			output.strategy = HaulStrategy::IndividualCargoIsCart;
			output.quantity = Quantity::create(1);
			output.workers.insert(worker);
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
	// TODO: Prioritize cart if a large number of items are requested.
	// TODO: Individual haul is both first priority, with full speed, and last priority, with minimum speed.
	// TODO: Consider cargo mass in non-individual hauling cases.
	Mass singleUnitMass = toHaul.getSingleUnitMass(project.m_area);
	if(fluidType.exists())
	{
		assert(toHaul.isItem());
		Mass fluidCargoMass = Mass::create(FluidType::getDensity(fluidType).get() * fluidVolume.get());
		singleUnitMass += fluidCargoMass;
		output.fluidType = fluidType;
		output.quantity = Quantity::create(fluidVolume.get());
	}
	else if(toHaul.isItem() && items.cargo_exists(toHaul.getItem()))
		singleUnitMass += items.cargo_getMass(toHaul.getItem());
	Quantity maxQuantityCanCarry = actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(worker, singleUnitMass, minimumSpeed);
	if(maxQuantityCanCarry != 0)
	{
		output.strategy = HaulStrategy::Individual;
		output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
		output.workers.insert(worker);
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
		if (toHaul.isItem())
			assert(haulTool != toHaul.getItem());
		// Cart
		maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(project.m_area, worker, haulTool, toHaul, minimumSpeed);
		if(maxQuantityCanCarry != 0)
		{
			output.strategy = HaulStrategy::Cart;
			output.haulTool = haulTool;
			output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
			output.workers.insert(worker);
			return output;
		}
		// Animal Cart
		ActorIndex yoked = project.m_area.m_hasHaulTools.getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(project.m_area, actors.getFaction(worker), haulTool, toHaul.getMass(project.m_area), minimumSpeed);
		if(yoked.exists())
		{
			maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(project.m_area, worker, yoked, haulTool, toHaul, minimumSpeed);
			if(maxQuantityCanCarry != 0)
			{
				output.strategy = HaulStrategy::AnimalCart;
				output.haulTool = haulTool;
				output.beastOfBurden = yoked;
				output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
				output.workers.insert(worker);
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
		ItemIndex panniers;
		static const ItemTypeId& panniersItemType = ItemType::byName("panniers");
		if(actors.equipment_containsItemType(pannierBearer, panniersItemType))
			panniers = actors.equipment_getFirstItemWithType(pannierBearer, panniersItemType);
		else
			panniers = project.m_area.m_hasHaulTools.getPanniersForActorToHaul(project.m_area, faction, pannierBearer, toHaul);
		if(panniers.exists())
		{
			maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithPanniersAndAnimal(project.m_area, worker, pannierBearer, panniers, toHaul, minimumSpeed);
			if(maxQuantityCanCarry != 0)
			{
				output.strategy = HaulStrategy::Panniers;
				output.beastOfBurden = pannierBearer;
				output.haulTool = panniers;
				output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
				output.workers.insert(worker);
				return output;
			}
		}
	}
	assert(output.strategy == HaulStrategy::None);
	return output;
}
void HaulSubproject::complete(const ActorOrItemIndex& delivered)
{
	delivered.validate(m_project.m_area);
	Area& area = m_project.m_area;
	assert(delivered.getLocation(area).empty());
	Actors& actors = area.getActors();
	Space& space =  area.getSpace();
	Items& items = area.getItems();
	Point3D location = actors.getLocation(m_leader.exists() ?
		m_leader.getIndex(actors.m_referenceData) :
		m_workers.front().getIndex(actors.m_referenceData)
	);
	const ShapeId& shape = delivered.getCompoundShape(area);
	const MoveTypeId& moveType = delivered.getMoveType(area);
	Facing4 facing = space.shape_canEnterEverWithAnyFacingReturnFacing(location, shape, moveType);
	delivered.maybeSetStatic(area);
	if(facing == Facing4::Null)
	{
		const auto pair = space.shape_getNearestEnterableEverPointWithFacing(location, shape, moveType);
		location = pair.first;
		facing = pair.second;
	}
	assert(location.exists());
	assert(facing != Facing4::Null);
	assert(space.shape_shapeAndMoveTypeCanEnterEverWithFacing(location, shape, moveType, facing));
	for(ActorReference worker : m_workers)
		actors.canReserve_clearAll(worker.getIndex(actors.m_referenceData));
	if(delivered.isItem())
	{
		m_projectRequirementCounts->delivered += m_quantity;
		assert(m_projectRequirementCounts->delivered <= m_projectRequirementCounts->required);
		ItemIndex deliveredIndex = delivered.getItem();
		ItemReference ref = area.getItems().m_referenceData.getReference(deliveredIndex);
		Quantity quantity = delivered.getQuantity(area);
		if(delivered.isGeneric(area))
		{
			ItemIndex existing = space.item_getGeneric(location, items.getItemType(deliveredIndex), items.getMaterialType(deliveredIndex));
			if(existing.exists())
			{
				// Delivery location already contains a stack of this type and material.
				// Update all references prior to setting location.
				ItemReference newRef = items.getReference(existing);
				m_toHaul = ActorOrItemReference::createForItem(newRef);
				m_project.updateRequiredGenericReference(newRef);
				// RequiredItems may contain a copy of ref in an ItemQuery.
				m_project.clearReferenceFromRequiredItems(ref);
				ref = newRef;
			}
		}
		deliveredIndex = items.location_setStatic(deliveredIndex, location, facing);
		// Reserve dropped off item with project.
		// Items are not reserved durring transit because reserved items don't move.
		if(m_fluidType.exists())
		{
			// Reserve fluid container.
			items.reservable_reserve(deliveredIndex, m_project.getCanReserve());
			m_project.m_deliveredItems.insert(ref, Quantity::create(1));
		}
		else
		{
			// Not fluids.
			// If the item is already reserved due to combining with an existing one on drop-off the reserved quantity will be increased.
			items.reservable_reserve(deliveredIndex, m_project.getCanReserve(), m_quantity);
			if(m_projectRequirementCounts->consumed)
				m_project.m_toConsume.getOrInsert(ref, Quantity::create(0)) += m_quantity;
			//TODO: This belongs in CraftProject.
			if(items.isWorkPiece(deliveredIndex))
				items.location_setStatic(deliveredIndex, m_project.m_location, Facing4::North);
			m_project.m_deliveredItems.getOrInsert(ref, Quantity::create(0)) += quantity;
		}
	}
	else
	{
		ActorIndex deliveredIndex = delivered.getActor();
		// Reserve dropped off actor with project.
		actors.reservable_reserve(deliveredIndex, m_project.getCanReserve());
		actors.location_setStatic(deliveredIndex, location, facing);
		m_project.m_deliveredActors.insert(actors.getReference(deliveredIndex));
	}
	m_project.onDelivered(delivered);
	Project& project = m_project;
	auto workers = std::move(m_workers);
	project.m_haulSubprojects.remove(*this);
	for(ActorReference worker : workers)
	{
		assert(!project.m_making.contains(worker));
		project.m_workers[worker].haulSubproject = nullptr;
		project.commandWorker(worker.getIndex(actors.m_referenceData));
	}
}
// Class method.
SmallSet<ActorIndex> HaulSubproject::actorsNeededToHaulAtMinimumSpeed(const Project& project, const ActorIndex& leader, const ActorOrItemIndex& toHaul)
{
	std::vector<ActorOrItemIndex> actorsAndItems;
	SmallSet<ActorIndex> output;
	output.insert(leader);
	actorsAndItems.push_back(ActorOrItemIndex::createForActor(leader));
	actorsAndItems.push_back(toHaul);
	Actors& actors = project.m_area.getActors();
	// For each actor without a sub project add to actors and items and see if the item can be moved fast enough.
	for(auto& [actor, projectWorker] : project.m_workers)
	{
		ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
		if(actorIndex == leader || projectWorker.haulSubproject != nullptr || !actors.isSentient(actorIndex))
			continue;
		assert(!output.contains(actorIndex));
		output.insert(actorIndex);
		if(output.size() > 2) //TODO: More then two requires multiple followers for one leader.
			break;
		actorsAndItems.push_back(ActorOrItemIndex::createForActor(actorIndex));
		Speed speed = PortablesHelpers::getMoveSpeedForGroup(project.m_area, actorsAndItems);
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
	return PortablesHelpers::getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, toHaul.getSingleUnitMass(area) * quantity, Mass::create(0), Mass::create(0));
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
	return PortablesHelpers::getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, toHaul.getSingleUnitMass(area) * quantity, Mass::create(0), Mass::create(0));
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
SmallSet<ActorIndex> HaulSubproject::actorsNeededToHaulAtMinimumSpeedWithTool(const Project& project, const ActorIndex& leader, const ActorOrItemIndex& toHaul, const ItemIndex& haulTool)
{
	std::vector<ActorOrItemIndex> actorsAndItems;
	SmallSet<ActorIndex> output;
	output.insert(leader);
	actorsAndItems.push_back(ActorOrItemIndex::createForActor(leader));
	actorsAndItems.push_back(ActorOrItemIndex::createForItem(haulTool));
	const Actors& actors = project.m_area.getActors();
	// For each actor without a sub project add to actors and items and see if the item can be moved fast enough.
	for(auto& [actor, projectWorker] : project.m_workers)
	{
		ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
		if(actorIndex == leader || projectWorker.haulSubproject != nullptr || !actors.isSentient(actorIndex))
			continue;
		assert(!output.contains(actorIndex));
		output.insert(actorIndex);
		if(output.size() > 2) //TODO: More then two requires multiple followers for one leader.
			break;
		actorsAndItems.push_back(ActorOrItemIndex::createForActor(actorIndex));
		// If to haul is also haul tool then there is no cargo, we are hauling the haulTool itself.
		Mass mass = toHaul == ActorOrItemIndex::createForItem(haulTool) ? Mass::create(0) : toHaul.getSingleUnitMass(project.m_area);
		Speed speed = PortablesHelpers::getMoveSpeedForGroupWithAddedMass(project.m_area, actorsAndItems, mass, Mass::create(0), Mass::create(0));
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
	return PortablesHelpers::getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, Mass::create(0), (toHaul.getSingleUnitMass(area) * quantity) + area.getItems().getMass(panniers), Mass::create(0));
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
	FullDisplacement volume = area.getItems().getVolume(item);
	return getToolToHaulVolume(area, faction, volume);
}
ItemIndex AreaHasHaulTools::getToolToHaulActor(const Area& area, const FactionId& faction, const ActorIndex& actor) const
{
	FullDisplacement volume = area.getActors().getVolume(actor);
	return getToolToHaulVolume(area, faction, volume);
}
ItemIndex AreaHasHaulTools::getToolToHaulPolymorphic(const Area& area, const FactionId& faction, const ActorOrItemIndex& toHaul) const
{
	FullDisplacement volume = toHaul.getVolume(area);
	return getToolToHaulVolume(area, faction, volume);
}
ItemIndex AreaHasHaulTools::getToolToHaulVolume(const Area& area, const FactionId& faction, const FullDisplacement& volume) const
{
	// Items like panniers with no move type also have internal volume but aren't relevent for this method.
	static MoveTypeId none = MoveType::byName("none");
	const Items& items = area.getItems();
	for(ItemReference item : m_haulTools)
	{
		ItemIndex index = item.getIndex(items.m_referenceData);
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
		ItemIndex index = item.getIndex(items.m_referenceData);
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
		ActorIndex index = actor.getIndex(actors.m_referenceData);
		std::vector<ActorOrItemIndex> shapes = { ActorOrItemIndex::createForActor(index), ActorOrItemIndex::createForItem(haulTool) };
		if(!actors.reservable_isFullyReserved(index, faction) && minimumHaulSpeed <= PortablesHelpers::getMoveSpeedForGroupWithAddedMass(area, shapes, cargoMass, Mass::create(0), Mass::create(0)))
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
		ActorIndex index = actor.getIndex(actors.m_referenceData);
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
		ItemIndex index = item.getIndex(items.m_referenceData);
		if(!items.reservable_isFullyReserved(index, faction) && ItemType::getInternalVolume(items.getItemType(index)) >= toHaul.getVolume(area) && actors.equipment_canEquipCurrently(actor, index))
			return index;
	}
	return ItemIndex::null();
}
void AreaHasHaulTools::registerHaulTool(Area& area, const ItemIndex& item)
{
	Items& items = area.getItems();
	const ItemReference ref = items.m_referenceData.getReference(item);
	assert(!m_haulTools.contains(ref));
	assert(ItemType::getInternalVolume(items.getItemType(item)) != 0);
	m_haulTools.insert(ref);
}
void AreaHasHaulTools::registerYokeableActor(Area& area, const ActorIndex& actor)
{
	ActorReference ref = area.getActors().getReference(actor);
	assert(!m_yolkableActors.contains(ref));
	m_yolkableActors.insert(ref);
}
void AreaHasHaulTools::unregisterHaulTool(Area& area, const ItemIndex& item)
{
	const ItemReference ref = area.getItems().getReference(item);
	assert(m_haulTools.contains(ref));
	m_haulTools.erase(ref);
}
void AreaHasHaulTools::unregisterYokeableActor(Area& area, const ActorIndex& actor)
{
	ActorReference ref = area.getActors().getReference(actor);
	assert(m_yolkableActors.contains(ref));
	m_yolkableActors.erase(ref);
}
