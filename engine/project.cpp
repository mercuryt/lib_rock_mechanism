#include "project.h"
#include "actorOrItemIndex.h"
#include "area.h"
#include "config.h"
#include "deserializationMemo.h"
#include "haul.h"
#include "reference.h"
#include "reservable.h"
#include "simulation.h"
#include "simulation/hasItems.h"
#include "actors/actors.h"
#include "items/items.h"
#include "blocks/blocks.h"
#include "path/terrainFacade.hpp"
#include "types.h"
#include "objectives/wander.h"
#include "portables.hpp"
#include <algorithm>
#include <memory>
// Project worker.
ProjectWorker::ProjectWorker(const Json& data, DeserializationMemo& deserializationMemo) :
	objective(deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))
{
	if(data.contains("haulSubproject"))
	{
		uintptr_t address = data["haulSubproject"].get<uintptr_t>();
		assert(deserializationMemo.m_haulSubprojects.contains(address));
		haulSubproject = deserializationMemo.m_haulSubprojects.at(address);
	}
}
Json ProjectWorker::toJson() const
{
	Json output {{"objective", objective}};
	if(haulSubproject != nullptr)
		output["haulSubproject"] = (uintptr_t)haulSubproject;
	return output;
}
// DishonorCallback.
ProjectRequiredShapeDishonoredCallback::ProjectRequiredShapeDishonoredCallback(const Json& data, DeserializationMemo& deserializationMemo) :
	m_project(*deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) { }
Json ProjectRequiredShapeDishonoredCallback::toJson() const
{
	return {
		{"type", "ProjectRequiredShapeDishonoredCallback"},
		{"project", reinterpret_cast<uintptr_t>(&m_project)}
	};
}
void ProjectRequiredShapeDishonoredCallback::execute(const Quantity&, const Quantity&)
{
	m_project.onActorOrItemReservationDishonored();
}
ProjectFinishEvent::ProjectFinishEvent(const Step& delay, Project& p, const Step start) :
	ScheduledEvent(p.m_area.m_simulation, delay, start), m_project(p) { }
void ProjectFinishEvent::execute(Simulation&, Area*) { m_project.complete(); }
void ProjectFinishEvent::clearReferences(Simulation&, Area*) { m_project.m_finishEvent.clearPointer(); }
ProjectTryToHaulEvent::ProjectTryToHaulEvent(const Step& delay, Project& p, const Step start) :
	ScheduledEvent(p.m_area.m_simulation, delay, start), m_project(p) { }
void ProjectTryToHaulEvent::execute(Simulation&, Area*) { m_project.m_tryToHaulThreadedTask.create(m_project); }
void ProjectTryToHaulEvent::clearReferences(Simulation&, Area*) { m_project.m_tryToHaulEvent.clearPointer(); }
ProjectTryToReserveEvent::ProjectTryToReserveEvent(const Step& delay, Project& p, const Step start) :
	ScheduledEvent(p.m_area.m_simulation, delay, start), m_project(p) { }
void ProjectTryToReserveEvent::execute(Simulation&, Area*)
{
	m_project.setDelayOff();
}
void ProjectTryToReserveEvent::clearReferences(Simulation&, Area*) { m_project.m_tryToReserveEvent.clearPointer(); }

ProjectTryToMakeHaulSubprojectThreadedTask::ProjectTryToMakeHaulSubprojectThreadedTask(Project& p) : m_project(p) { }
void ProjectTryToMakeHaulSubprojectThreadedTask::readStep(Simulation&, Area*)
{
	Actors& actors = m_project.m_area.getActors();
	for(auto& [actor, projectWorker] : m_project.m_workers)
	{
		if(projectWorker.haulSubproject != nullptr)
			continue;
		ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
		auto destinationConditon = [this, actorIndex](const BlockIndex& block, const Facing4&) ->std::pair<bool, BlockIndex>
		{
			if(blockContainsDesiredItem(block, actorIndex))
				return std::make_pair(true, block);
			return std::make_pair(false, BlockIndex::null());
		};
		// Path result is unused, running only for condition side effect.
		constexpr bool anyOccupiedBlock = true;
		constexpr bool adjacent = true;
		[[maybe_unused]] FindPathResult result = m_project.m_area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(actorIndex)).findPathToConditionBreadthFirstWithoutMemo<anyOccupiedBlock, decltype(destinationConditon)>(destinationConditon, actors.getLocation(actorIndex), actors.getFacing(actorIndex), actors.getShape(actorIndex), projectWorker.objective->m_detour, adjacent, actors.getFaction(actorIndex));
		// Only make at most one per step.
		// TODO: Would it be better to do them all at once instead of one per step? Better temporal locality, higher risk of lag spike.
		if(m_haulProjectParamaters.strategy != HaulStrategy::None)
			return;
	}
}
void ProjectTryToMakeHaulSubprojectThreadedTask::writeStep(Simulation&, Area* area)
{
	if(m_haulProjectParamaters.strategy == HaulStrategy::None)
	{
		if(++m_project.m_haulRetries == Config::projectTryToMakeSubprojectRetriesBeforeProjectDelay)
		{
			// Enough retries, delay.
			// setDelayOn might destroy the project, so store canReset first.
			bool canReset = m_project.canReset();
			m_project.setDelayOn();
			if(canReset)
				m_project.reset();
		}
		else
		{
			// Nothing haulable found, wait a bit and try again.
			if(m_project.m_minimumMoveSpeed > 1)
				--m_project.m_minimumMoveSpeed;
			m_project.m_tryToHaulEvent.schedule(Config::stepsFrequencyToLookForHaulSubprojects, m_project);
		}
	}
	else
	{
		ActorOrItemReference ref = m_haulProjectParamaters.toHaul;
		assert(m_project.m_toPickup.contains(ref));
		if(
			m_project.m_toPickup[ref].second < m_haulProjectParamaters.quantity ||
			!m_haulProjectParamaters.validate(m_project.m_area)
		)
		{
			// Something that was avalible during read step is now reserved. Ensure that there is a new threaded task to try again.
			if(!m_project.m_tryToHaulThreadedTask.exists())
				m_project.m_tryToHaulThreadedTask.create(m_project);
		}
		else
		{
			// All guards passed, create subproject.
			assert(!m_haulProjectParamaters.workers.empty());
			//TODO: move this logic into Project::createSubproject.
			HaulSubproject& subproject = m_project.m_haulSubprojects.emplace_back(m_project, m_haulProjectParamaters);
			// Remove the target of the new haulSubproject from m_toPickup.
			const Actors& actors = m_project.getArea().getActors();
			const Items& items = m_project.getArea().getItems();
			m_project.removeToPickup(m_haulProjectParamaters.toHaul.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData), m_haulProjectParamaters.quantity);
			m_project.onSubprojectCreated(subproject);
			if(m_project.m_toPickup.empty())
			{
				// All subprojects have been dispatched, any remaining workers are sent to the project site.
				// TODO: could be optimized maybe.
				Actors& actors = area->getActors();
				for(auto [ workerRef, projectWorker] : m_project.m_workers)
				{
					ActorIndex worker = workerRef.getIndex(actors.m_referenceData);
					if(projectWorker.haulSubproject == nullptr)
					{
						if(m_project.canRecruitHaulingWorkersOnly())
						{
							projectWorker.objective->reset(*area, worker);
							actors.objective_canNotCompleteSubobjective(worker);
						}
						else if(
							!actors.isAdjacentToLocation(worker, m_project.m_location) &&
							!actors.move_hasEvent(worker) &&
							actors.move_getPath(worker).empty()
						)
							actors.move_setDestinationAdjacentToLocation(worker, m_project.m_location);
					}
				}
			}
		}
	}
}
void ProjectTryToMakeHaulSubprojectThreadedTask::clearReferences(Simulation&, Area*) { m_project.m_tryToHaulThreadedTask.clearPointer(); }
bool ProjectTryToMakeHaulSubprojectThreadedTask::blockContainsDesiredItem(const BlockIndex& block, const ActorIndex& actor)
{
	auto& blocks = m_project.m_area.getBlocks();
	Actors& actors = m_project.m_area.getActors();
	Items& items = m_project.m_area.getItems();
	for(ItemIndex item : blocks.item_getAll(block))
	{
		ActorOrItemReference ref = ActorOrItemReference::createForItem(items.m_referenceData.getReference(item));
		if(m_project.m_toPickup.contains(ref))
		{
			m_haulProjectParamaters = HaulSubproject::tryToSetHaulStrategy(m_project, ref.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData), actor);
			if(m_haulProjectParamaters.strategy != HaulStrategy::None)
				return true;
		}
	}
	return false;
};
ProjectTryToAddWorkersThreadedTask::ProjectTryToAddWorkersThreadedTask(Project& p) : m_project(p) { }
void ProjectTryToAddWorkersThreadedTask::readStep(Simulation&, Area*)
{
	// Iterate candidate workers, verify that they can path to the project
	// location. Accumulate resources that they can path to untill reservations
	// are complete.
	// If reservations are not complete flush the data from project.
	// TODO: Unwisely modifing data out side the object durring read step.
	SmallSet<ActorOrItemIndex> recordedShapes;
	Actors& actors = m_project.m_area.getActors();
	Items& items = m_project.m_area.getItems();
	for(auto& [candidate, objective] : m_project.m_workerCandidatesAndTheirObjectives)
	{
		assert(!m_project.m_making.contains(candidate));
		ActorIndex candidateIndex = candidate.getIndex(actors.m_referenceData);
		assert(!m_project.getWorkers().contains(candidate));
		if(!actors.isAdjacentToLocation(candidateIndex, m_project.m_location))
		{
			// Verify the worker can path to the job site.
			TerrainFacade& terrainFacade = m_project.m_area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(candidateIndex));
			constexpr bool adjacent = true;
			FindPathResult result = terrainFacade.findPathToWithoutMemo(actors.getLocation(candidateIndex), actors.getFacing(candidateIndex), actors.getShape(candidateIndex), m_project.m_location, objective->m_detour, adjacent);
			if(result.path.empty() && !result.useCurrentPosition)
			{
				m_cannotPathToJobSite.insert(candidate);
				continue;
			}
		}
		if(!m_project.reservationsComplete())
		{
			// Check if the worker can get to any required item or actor.
			for(ItemReference item : actors.equipment_getAll(candidateIndex))
				for(auto& [itemQuery, projectRequirementCounts] : m_project.m_requiredItems)
				{
					if(projectRequirementCounts.required == projectRequirementCounts.reserved)
						continue;
					ItemIndex itemIndex = item.getIndex(items.m_referenceData);
					assert(!items.reservable_exists(itemIndex, m_project.m_faction));
					if(!itemQuery.query(m_project.m_area, itemIndex))
						continue;
					Quantity desiredQuantity = projectRequirementCounts.required - projectRequirementCounts.reserved;
					Quantity quantity = std::min(desiredQuantity, items.reservable_getUnreservedCount(itemIndex, m_project.m_faction));
					assert(quantity != 0);
					projectRequirementCounts.reserved += quantity;
					assert(projectRequirementCounts.reserved <= projectRequirementCounts.required);
					ItemReference itemRef = items.getReference(itemIndex);
					m_reservedEquipment.getOrCreate(candidate).insert(&projectRequirementCounts, itemRef);
				}
			// capture by reference is used here because the pathing is being done immideatly instead of batched.
			auto recordItemOnGround = [&](const ActorOrItemIndex& actorOrItem, ProjectRequirementCounts& counts)
			{
				recordedShapes.insert(actorOrItem);
				Quantity desiredQuantity = counts.required - counts.reserved;
				Quantity quantity = std::min(desiredQuantity, actorOrItem.reservable_getUnreservedCount(m_project.m_area, m_project.m_faction));
				assert(quantity != 0);
				counts.reserved += quantity;
				assert(counts.reserved <= counts.required);
				if(actorOrItem.getLocation(m_project.m_area) == m_project.m_location || actorOrItem.isAdjacentToLocation(m_project.m_area, m_project.m_location))
				{
					// Item is already at or next to project location.
					counts.delivered += quantity;
					assert(counts.delivered <= counts.reserved);
					ActorOrItemReference ref = actorOrItem.toReference(m_project.m_area);
					assert(!m_alreadyAtSite.contains(ref));
					m_alreadyAtSite.insert(ref, quantity);
				}
				else
					// TODO: project shoud be read only here, requires tracking reservationsComplete seperately for task.
					m_project.addToPickup(actorOrItem, counts, quantity);
			};
			// Verfy the worker can path to the required materials. Cumulative for all candidates in this step but reset if not satisfied.
			// capture by reference is used here because the pathing is being done immideatly instead of batched.
			auto destinationCondition = [&](const BlockIndex& block, const Facing4&) -> std::pair<bool, BlockIndex>
			{
				auto& blocks = m_project.m_area.getBlocks();
				for(ItemIndex item : blocks.item_getAll(block))
				{
					ActorOrItemIndex polymorphicItem = ActorOrItemIndex::createForItem(item);
					for(auto& [itemQuery, projectRequirementCounts] : m_project.m_requiredItems)
					{
						if(projectRequirementCounts.required == projectRequirementCounts.reserved)
							continue;
						if(items.reservable_isFullyReserved(item, m_project.m_faction))
							continue;
						if(!itemQuery.query(m_project.m_area, item))
							continue;
						if(recordedShapes.contains(polymorphicItem))
							continue;
						recordItemOnGround(polymorphicItem, projectRequirementCounts);
						if(m_project.reservationsComplete())
							return std::make_pair(true, block);
					}
				}
				for(ActorIndex actor : blocks.actor_getAll(block))
				{
					ActorOrItemIndex polymorphicActor = ActorOrItemIndex::createForActor(actor);
					for(auto& [actorQuery, projectRequirementCounts] : m_project.m_requiredActors)
					{
						if(projectRequirementCounts.required == projectRequirementCounts.reserved)
							continue;
						if(actors.reservable_isFullyReserved(actor, m_project.m_faction))
							continue;
						if(!actorQuery.query(m_project.m_area, actor))
							continue;
						if(recordedShapes.contains(polymorphicActor))
							continue;
						// TODO: Shoud this be record actor on ground?
						recordItemOnGround(polymorphicActor, projectRequirementCounts);
						if(m_project.reservationsComplete())
							return std::make_pair(true, block);
					}
				}
				return std::make_pair(false, BlockIndex::null());
			};
			// TODO: Path is not used, find path is run for side effects of predicate.
			TerrainFacade& terrainFacade = m_project.m_area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(candidateIndex));
			constexpr bool anyOccupiedBlock = true;
			constexpr bool detour = false;
			constexpr bool adjacent = true;
			[[maybe_unused]] FindPathResult result = terrainFacade.findPathToConditionBreadthFirstWithoutMemo<anyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, actors.getLocation(candidateIndex), actors.getFacing(candidateIndex), actors.getShape(candidateIndex), detour, adjacent);
		}
	}
	if(!m_project.reservationsComplete())
	{
		// Failed to find all, reset data.
		resetProjectCounts();
	}
	else
	{
		// Found all materials, remove any workers which cannot path to the project location.
		std::erase_if(m_project.m_workerCandidatesAndTheirObjectives, [&](auto& pair) { return m_cannotPathToJobSite.contains(pair.first); });
		// Set onDestoy callbacks for all items which will be reserved, in case they are destroyed before writeStep.
		// Actors get marked dead rather then destroyed outright so no need for callbacks on them.
		m_hasOnDestroy.setCallback(std::make_unique<ResetProjectOnDestroyCallBack>(m_project));
		for(auto& pair : m_reservedEquipment)
			for(std::pair<ProjectRequirementCounts*, ItemReference> pair2 : pair.second)
				items.onDestroy_subscribe(pair2.second.getIndex(items.m_referenceData), m_hasOnDestroy);
		for(auto& pair : m_alreadyAtSite)
			if(pair.first.isActor())
				actors.onDestroy_subscribe(pair.first.getIndex(actors.m_referenceData, items.m_referenceData).toActor(), m_hasOnDestroy);
			else
				items.onDestroy_subscribe(pair.first.getIndex(actors.m_referenceData, items.m_referenceData).toItem(), m_hasOnDestroy);
		// Accessing project here because we are modifing project from the readStep, probably unwisely.
		for(auto& pair : m_project.m_toPickup)
			if(pair.first.isActor())
				actors.onDestroy_subscribe(pair.first.getIndex(actors.m_referenceData, items.m_referenceData).toActor(), m_hasOnDestroy);
			else
				items.onDestroy_subscribe(pair.first.getIndex(actors.m_referenceData, items.m_referenceData).toItem(), m_hasOnDestroy);
	}
}
void ProjectTryToAddWorkersThreadedTask::writeStep(Simulation&, Area*)
{
	#ifndef NDEBUG
		for(auto& [actor, objective] : m_project.m_workerCandidatesAndTheirObjectives)
			assert(!m_project.m_making.contains(actor));
	#endif
	m_hasOnDestroy.unsubscribeAll();
	auto toReleaseWithProjectDelay = m_cannotPathToJobSite;
	Items& items = m_project.m_area.getItems();
	Actors& actors = m_project.m_area.getActors();
	if(m_project.reservationsComplete())
	{
		// If workers exist already then required are already reserved.
		if(m_project.m_workers.empty())
		{
			// Requirements satisfied, verifiy they are all still avalible.
			if(!validate())
			{
				// Some selected actorOrItem was probably reserved or destroyed, try again.
				resetProjectCounts();
				m_project.m_tryToAddWorkersThreadedTask.create(m_project);
				return;
			}
			// Reserve all required.
			for(auto& [actorOrItem, pair] : m_project.m_toPickup)
			{
				std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<ProjectRequiredShapeDishonoredCallback>(m_project);
				actorOrItem.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData).reservable_reserve(m_project.m_area, m_project.m_canReserve, pair.second, std::move(dishonorCallback));
			}
			for(auto& [actorOrItem, quantity] : m_alreadyAtSite)
			{
				std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<ProjectRequiredShapeDishonoredCallback>(m_project);
				actorOrItem.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData).reservable_reserve(m_project.m_area, m_project.m_canReserve, quantity, std::move(dishonorCallback));
			}
			m_project.m_alreadyAtSite = m_alreadyAtSite;
			for(auto& pair : m_reservedEquipment)
				for(std::pair<ProjectRequirementCounts*, ItemReference> pair2 : pair.second)
				{
					ActorOrItemReference ref;
					ref.setItem(pair2.second);
					std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<ProjectRequiredShapeDishonoredCallback>(m_project);
					items.reservable_reserve(pair2.second.getIndex(items.m_referenceData), m_project.m_canReserve, Quantity::create(1), std::move(dishonorCallback));
				}
			m_project.m_reservedEquipment = m_reservedEquipment;
		}
		// Add all actors.
		for(auto& [actor, objective] : m_project.m_workerCandidatesAndTheirObjectives)
		{
			ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
			// TODO: Reserve a work location for the actor in order to prevent deadlocks due to lack of access by a tool holder.
			// 	Alternatively, allow workers to swap positions if one is waiting.
			if(m_project.canAddWorker(actorIndex))
				m_project.addWorker(actorIndex, *objective);
			else
			{
				// Release without project delay: allow to rejoin as soon as the number of workers is less then max.
				actors.objective_reset(actorIndex);
				actors.objective_canNotCompleteSubobjective(actorIndex);
			}
		}
		m_project.m_workerCandidatesAndTheirObjectives.clear();
		m_project.onReserve();
	}
	else
	{
		// Could not find required items / actors, reset
		if(m_project.canReset())
		{
			// Remove workers here rather then allowing them to be dispatched by reset so we can call onProjectCannotReserve with toRelase.
			auto workers = m_project.getWorkersAndCandidates();
			for(ActorIndex actor : workers)
			{
				ActorReference ref = m_project.m_area.getActors().getReference(actor);
				toReleaseWithProjectDelay.maybeInsert(ref);
			}
			m_project.m_workers.clear();
			m_project.m_workerCandidatesAndTheirObjectives.clear();
			m_project.reset();
		}
		else
			m_project.cancel();
	}
	// These workers are not allowed to attempt to rejoin this project for a period, they may not be able to path to it or reserved required items.
	for(const ActorReference& actor : toReleaseWithProjectDelay)
		actors.objective_projectCannotReserve(actor.getIndex(actors.m_referenceData));
}
void ProjectTryToAddWorkersThreadedTask::clearReferences(Simulation&, Area*) { m_project.m_tryToAddWorkersThreadedTask.clearPointer(); }
bool ProjectTryToAddWorkersThreadedTask::validate()
{
	Actors& actors = m_project.m_area.getActors();
	Items& items = m_project.m_area.getItems();
	// Require some workers can reach the job site.
	if(m_project.m_workerCandidatesAndTheirObjectives.empty())
		return false;
	// Ensure all items selected to be reserved are still reservable.
	for(auto& [actorOrItem, pair] : m_project.m_toPickup)
		if(actorOrItem.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData).reservable_getUnreservedCount(m_project.m_area, m_project.m_faction) < pair.second)
			return false;
	for(auto& [actorOrItem, quantity] : m_alreadyAtSite)
		if(actorOrItem.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData).reservable_getUnreservedCount(m_project.m_area, m_project.m_faction) < quantity)
			return false;
	for(auto& pair : m_reservedEquipment)
		for(const std::pair<ProjectRequirementCounts*, ItemReference>& pair2 : pair.second)
			if(items.reservable_exists(pair2.second.getIndex(items.m_referenceData), m_project.m_faction))
				return false;
	return true;
}
void ProjectTryToAddWorkersThreadedTask::resetProjectCounts()
{
	for(auto& pair : m_project.m_requiredItems)
	{
		pair.second.reserved = Quantity::create(0);
		pair.second.delivered = Quantity::create(0);
	}
	for(auto& pair : m_project.m_requiredActors)
	{
		pair.second.reserved = Quantity::create(0);
		pair.second.delivered = Quantity::create(0);
	}
	m_project.m_toPickup.clear();
}
// Derived classes are expected to provide getDuration, getConsumedItems, getUnconsumedItems, getByproducts, onDelay, offDelay, and onComplete.
Project::Project(const FactionId& f, Area& a, const BlockIndex& l, const Quantity& mw, std::unique_ptr<DishonorCallback> locationDishonorCallback) :
	m_finishEvent(a.m_eventSchedule),
	m_tryToHaulEvent(a.m_eventSchedule),
	m_tryToReserveEvent(a.m_eventSchedule),
	m_tryToHaulThreadedTask(a.m_threadedTaskEngine),
	m_tryToAddWorkersThreadedTask(a.m_threadedTaskEngine),
	m_canReserve(f),
	m_area(a),
	m_faction(f),
	m_location(l),
	m_minimumMoveSpeed(Config::minimumHaulSpeedInital),
	m_maxWorkers(mw)
{
	m_area.getBlocks().reserve(m_location, m_canReserve, std::move(locationDishonorCallback));
	m_area.getBlocks().project_add(m_location, *this);
}
Project::Project(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	m_finishEvent(area.m_eventSchedule),
	m_tryToHaulEvent(area.m_eventSchedule),
	m_tryToReserveEvent(area.m_eventSchedule),
	m_tryToHaulThreadedTask(area.m_threadedTaskEngine),
	m_tryToAddWorkersThreadedTask(area.m_threadedTaskEngine),
	m_canReserve(data["faction"].get<FactionId>()),
	m_area(deserializationMemo.area(data["area"])), m_faction(data["faction"].get<FactionId>()),
	m_location(data["location"].get<BlockIndex>()), m_haulRetries(data["haulRetries"].get<Quantity>()),
	m_minimumMoveSpeed(data["minimumMoveSpeed"].get<Speed>()),
	m_maxWorkers(data["maxWorkers"].get<Quantity>()),
	m_requirementsLoaded(data["requirementsLoaded"].get<bool>()),
	m_delay(data["delay"].get<bool>())
{
	Items& items = m_area.getItems();
	if(data.contains("requiredItems"))
		for(const Json& pair : data["requiredItems"])
		{
			ItemQuery itemQuery(pair[0], area);
			ProjectRequirementCounts requirementCounts(pair[1]);
			m_requiredItems.emplace_back(std::make_pair(itemQuery, requirementCounts));
			deserializationMemo.m_projectRequirementCounts[pair[1]["address"].get<uintptr_t>()] = &m_requiredItems.back().second;
		}
	if(data.contains("requiredActors"))
		for(const Json& pair : data["requiredActors"])
		{
			ActorQuery actorQuery(pair[0], area);
			ProjectRequirementCounts requirementCounts(pair[1]);
			m_requiredActors.emplace_back(std::make_pair(actorQuery, requirementCounts));
			deserializationMemo.m_projectRequirementCounts[pair[1]["address"].get<uintptr_t>()] = &m_requiredActors.back().second;
		}
	if(data.contains("toConsume"))
		for(const Json& pair : data["toConsume"])
		{
			ItemReference ref(pair[0], items.m_referenceData);
			m_toConsume.insert(ref, pair[1].get<Quantity>());
		}
	if(data.contains("toPickup"))
		for(const Json& tuple : data["toPickup"])
		{
			ActorOrItemReference ref(tuple[0], area);
			ProjectRequirementCounts& projectRequirementCounts = *deserializationMemo.m_projectRequirementCounts.at(tuple[1].get<uintptr_t>());
			Quantity quantity = tuple[2].get<Quantity>();
			m_toPickup.insert(ref, std::make_pair(&projectRequirementCounts, quantity));
		}
	if(data.contains("haulSubprojects"))
		for(const Json& haulSubprojectData : data["haulSubprojects"])
			m_haulSubprojects.emplace_back(haulSubprojectData, *this, deserializationMemo);
	m_canReserve.load(data["canReserve"], deserializationMemo, m_area);
	if(data.contains("finishEventStart"))
		m_finishEvent.schedule(data["finishEventDuration"].get<Step>(), *this, data["finishEventStart"].get<Step>());
	if(data.contains("tryToHaulEventStart"))
		m_tryToHaulEvent.schedule(Config::stepsFrequencyToLookForHaulSubprojects, *this, data["tryToHaulEventStart"].get<Step>());
	if(data.contains("tryToReserveEventStart"))
		m_tryToReserveEvent.schedule(Config::stepsToDelayBeforeTryingAgainToReserveItemsAndActorsForAProject, *this, data["tryToReserveEventStart"].get<Step>());
	if(data.contains("tryToHaulThreadedTask"))
		m_tryToHaulThreadedTask.create(*this);
	if(data.contains("tryToAddWorkersThreadedTask"))
		m_tryToAddWorkersThreadedTask.create(*this);
	deserializationMemo.m_projects[data["address"].get<uintptr_t>()] = this;
	m_area.getBlocks().project_add(m_location, *this);
}
void Project::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	Actors& actors = m_area.getActors();
	if(data.contains("workers"))
		for(const Json& pair : data["workers"])
		{
			ActorReference ref(pair[0], actors.m_referenceData);
			m_workers.emplace(ref, pair[1], deserializationMemo);
			actors.project_set(ref.getIndex(actors.m_referenceData), *this);
		}
	if(data.contains("candidates"))
		for(const Json& pair : data["candidates"])
		{
			ActorReference ref(pair[0], actors.m_referenceData);
			assert(deserializationMemo.m_objectives.contains(pair[1].get<uintptr_t>()));
			Objective& objective = *deserializationMemo.m_objectives.at(pair[1].get<uintptr_t>());
			m_workerCandidatesAndTheirObjectives.emplace_back(ref, &objective);
			actors.project_set(ref.getIndex(actors.m_referenceData), *this);
		}
}
Json Project::toJson() const
{
	Json data({
		{"address", reinterpret_cast<uintptr_t>(this)},
		{"faction", m_faction},
		{"maxWorkers", m_maxWorkers},
		{"delay", m_delay},
		{"haulRetries", m_haulRetries},
		{"location", m_location},
		{"area", m_area},
		{"requirementsLoaded", m_requirementsLoaded},
		{"minimumMoveSpeed", m_minimumMoveSpeed},
	});
	if(!m_workerCandidatesAndTheirObjectives.empty())
	{
		data["candidates"] = Json::array();
		for(auto& [actor, objective] : m_workerCandidatesAndTheirObjectives)
			data["candidates"].push_back({actor, objective});
	}
	if(!m_workers.empty())
	{
		data["workers"] = Json::array();
		for(auto& [actor, projectWorker] : m_workers)
			data["workers"].push_back({actor, projectWorker.toJson()});
	}
	if(!m_requiredItems.empty())
	{
		data["requiredItems"] = Json::array();
		for(auto& [itemQuery, requiredCounts] : m_requiredItems)
		{
			Json pair({itemQuery, requiredCounts});
			pair[1]["address"] = reinterpret_cast<uintptr_t>(&requiredCounts);
			data["requiredItems"].push_back(pair);
		}
	}
	if(!m_requiredActors.empty())
	{
		data["requiredActors"] = Json::array();
		for(auto& [actorQuery, requiredCounts] : m_requiredActors)
		{
			Json pair({actorQuery.toJson(), requiredCounts});
			data["requiredActors"].push_back(pair);
		}
	}
	if(!m_toConsume.empty())
	{
		data["toConsume"] = Json::array();
		for(auto& [item, quantity] : m_toConsume)
			data["toConsume"].push_back({item, quantity});
	}
	if(!m_toPickup.empty())
	{
		data["toPickup"] = Json::array();
		for(auto& [actorOrItem, pair] : m_toPickup)
			data["toPickup"].push_back({actorOrItem, (uintptr_t)pair.first, pair.second});
	}
	if(!m_haulSubprojects.empty())
	{
		data["haulSubprojects"] = Json::array();
		for(const HaulSubproject& subproject : m_haulSubprojects)
			data["haulSubprojects"].push_back(subproject.toJson());
	}
	data["canReserve"] = m_canReserve.toJson();
	if(m_finishEvent.exists())
	{
		data["finishEventStart"] = m_finishEvent.getStartStep();
		data["finishEventDuration"] = m_finishEvent.duration();
	}
	if(m_tryToHaulEvent.exists())
		data["tryToHaulEventStart"] = m_tryToHaulEvent.getStartStep();
	if(m_tryToReserveEvent.exists())
		data["tryToReserveEventStart"] = m_tryToReserveEvent.getStartStep();
	if(m_tryToHaulThreadedTask.exists())
		data["tryToHaulThreadedTask"] = true;
	if(m_tryToAddWorkersThreadedTask.exists())
		data["tryToAddWorkersThreadedTask"] = true;
	return data;
}
bool Project::reservationsComplete() const
{
	if(!m_requirementsLoaded)
		return false;
	for(auto& pair : m_requiredItems)
		if(pair.second.required > pair.second.reserved)
			return false;
	for(auto& pair : m_requiredActors)
		if(pair.second.required > pair.second.reserved)
			return false;
	return true;
}
bool Project::deliveriesComplete() const
{
	for(auto& pair : m_requiredItems)
		if(pair.second.required > pair.second.delivered)
			return false;
	for(auto& pair : m_requiredActors)
		if(pair.second.required > pair.second.delivered)
			return false;
	return true;
}
void Project::recordRequiredActorsAndItems()
{
	m_requirementsLoaded = true;
	for(auto& [itemQuery, quantity] : getConsumed())
	{
		ProjectRequirementCounts projectRequirementCounts(quantity, true);
		m_requiredItems.emplace_back(itemQuery, std::move(projectRequirementCounts));
	}
	for(auto& [itemQuery, quantity] : getUnconsumed())
	{
		ProjectRequirementCounts projectRequirementCounts(quantity, false);
		m_requiredItems.emplace_back(itemQuery, std::move(projectRequirementCounts));
	}
	for(auto& [actorQuery, quantity] : getActors())
	{
		ProjectRequirementCounts projectRequirementCounts(quantity, false);
		m_requiredActors.emplace_back(actorQuery, std::move(projectRequirementCounts));
	}
}
void Project::addWorker(const ActorIndex& actor, Objective& objective)
{
	assert(!m_workers.contains(m_area.getActors().getReference(actor)));
	assert(m_area.getActors().isSentient(actor));
	assert(canAddWorker(actor));
	// Initalize a ProjectWorker for this worker.
	ActorReference ref = m_area.getActors().getReference(actor);
	m_workers.emplace(ref, objective);
	commandWorker(actor);
}
void Project::addWorkerCandidate(const ActorIndex& actor, Objective& objective)
{
	assert(!hasCandidate(actor));
	assert(canAddWorker(actor));
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	assert(!m_making.contains(actorRef));
	actors.project_set(actor, *this);
	if(!m_requirementsLoaded)
		recordRequiredActorsAndItems();
	m_workerCandidatesAndTheirObjectives.emplace_back(actorRef, &objective);
	if(!m_tryToAddWorkersThreadedTask.exists())
		m_tryToAddWorkersThreadedTask.create(*this);
}
void Project::removeWorkerCandidate(const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	auto iter = std::ranges::find(m_workerCandidatesAndTheirObjectives, actorRef, &std::pair<ActorReference, Objective*>::first);
	assert(iter != m_workerCandidatesAndTheirObjectives.end());
	m_workerCandidatesAndTheirObjectives.erase(iter);
}
// To be called by Objective::execute.
void Project::commandWorker(const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	assert(!m_making.contains(actorRef));
	if(!m_workers.contains(actorRef))
	{
		assert(std::ranges::find(m_workerCandidatesAndTheirObjectives, actorRef, &std::pair<ActorReference, Objective*>::first) != m_workerCandidatesAndTheirObjectives.end());
		// Candidates do nothing but wait.
		return;
	}
	if(m_workers[actorRef].haulSubproject != nullptr)
		// The worker has been dispatched to fetch an item.
		m_workers[actorRef].haulSubproject->commandWorker(actor);
	else
	{
		// The worker has not been dispatched to fetch an actor or an item.
		if(deliveriesComplete())
		{
			// All items and actors have been delivered, the worker starts making.
			if(actors.isAdjacentToLocation(actor, m_location))
				addToMaking(actor);
			else
				actors.move_setDestinationAdjacentToLocation(actor, m_location, m_workers[actorRef].objective->m_detour);
		}
		else if(m_toPickup.empty())
		{
			if(actors.isAdjacentToLocation(actor, m_location))
			{
				if(canRecruitHaulingWorkersOnly())
				{
					assert(!m_reservedEquipment.contains(actorRef));
					// We have no use for this worker so release them.
					actors.objective_canNotCompleteObjective(actor, *m_workers[actorRef].objective);
				}
				else
				{
					// Any tools being carried are marked delivered.
					if(m_reservedEquipment.contains(actorRef))
					{
						for(auto& pair : m_reservedEquipment[actorRef])
						{
							++pair.first->delivered;
							assert(pair.first->delivered <= pair.first->reserved);
						}
						if(deliveriesComplete())
						{
							addToMaking(actor);
							return;
						}
					}
					// All items and actors have been reserved with other workers dispatched to fetch them, the worker waits for them.
					m_waiting.insert(actorRef);
				}
			}
			else
				actors.move_setDestinationAdjacentToLocation(actor, m_location, m_workers[actorRef].objective->m_detour);
			//TODO: Schedule end of waiting and cancelation of task.
		}
		else
		{
			// Pickup phase is not complete, try to create a haul subproject unless one exists.
			if(!m_tryToHaulThreadedTask.exists())
				m_tryToHaulThreadedTask.create(*this);
		}
	}
}
// To be called by Objective::cancel, Objective::delay.
void Project::removeWorker(const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	Items& items = m_area.getItems();
	assert(actors.project_get(actor) == this);
	actors.project_unset(actor);
	if(hasCandidate(actor))
	{
		assert(!m_workers.contains(actorRef));
		removeWorkerCandidate(actor);
		return;
	}
	assert(m_workers.contains(actorRef));
	if(m_workers[actorRef].haulSubproject != nullptr)
		m_workers[actorRef].haulSubproject->removeWorker(actor);
	if(m_making.contains(actorRef))
		removeFromMaking(actor);
	m_workers.erase(actorRef);
	auto found = m_reservedEquipment.find(actorRef);
	if(found != m_reservedEquipment.end())
	{
		for(auto& pair : found.second())
		{
			ItemReference item = pair.second;
			items.reservable_unreserveAll(item.getIndex(items.m_referenceData));
		}
	}
	else if(m_workers.empty())
	{
		if(canReset())
			reset();
		else
			cancel();
	}
}
void Project::addToMaking(const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	assert(m_workers.contains(actorRef));
	assert(!m_making.contains(actorRef));
	m_making.insert(actorRef);
	onAddToMaking(actor);
	scheduleFinishEvent();
}
void Project::removeFromMaking(const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	assert(m_workers.contains(actorRef));
	assert(m_making.contains(actorRef));
	m_making.erase(actorRef);
	if(m_making.empty())
		m_finishEvent.unschedule();
	else
		scheduleFinishEvent();
}
void Project::complete()
{
	Items& items = m_area.getItems();
	Actors& actors = m_area.getActors();
	m_canReserve.deleteAllWithoutCallback();
	auto& blocks = m_area.getBlocks();
	blocks.project_remove(m_location, *this);
	for(auto& [item, quantity] : m_toConsume)
	{
		item.validate(items.m_referenceData);
		ItemIndex index = item.getIndex(items.m_referenceData);
		// Consumed items are not delivered, destroy reference before item is destroyed.
		m_deliveredItems.maybeErase(item);
		item.clear();
		items.removeQuantity(index, quantity);
	}
	if(!m_area.m_hasStockPiles.contains(m_faction))
		m_area.m_hasStockPiles.registerFaction(m_faction);
	for(auto& [itemType, materialType, quantity] : getByproducts())
	{
		ItemIndex item = blocks.item_addGeneric(m_location, itemType, materialType, quantity);
		// Item may be newly created or it may be prexisting, and thus already designated for stockpileing.
		if(!items.stockpile_canBeStockPiled(item, m_faction))
			m_area.m_hasStockPiles.getForFaction(m_faction).addItem(item);
	}
	for(auto& [actor, projectWorker] : m_workers)
		actors.project_unset(actor.getIndex(actors.m_referenceData));
	// If onComplete sets the location as impassible the unconsumed items delivered will be distributed around the block.
	// Spawn contained updates it's item reference when generics combine.
	onComplete();
}
void Project::cancel()
{
	m_canReserve.deleteAllWithoutCallback();
	m_area.getBlocks().project_remove(m_location, *this);
	Actors& actors = m_area.getActors();
	for(auto& [actor, projectWorker] : m_workers)
		actors.project_unset(actor.getIndex(actors.m_referenceData));
	m_tryToAddWorkersThreadedTask.maybeCancel(m_area.m_simulation, &m_area);
	m_tryToHaulThreadedTask.maybeCancel(m_area.m_simulation, &m_area);
	onCancel();
}
void Project::scheduleFinishEvent(Step start)
{
	assert(!m_making.empty());
	if(start == 0)
		start = m_area.m_simulation.m_step;
	Step delay = getDuration();
	if(m_finishEvent.exists())
	{
		Percent complete = m_finishEvent.percentComplete();
		m_finishEvent.unschedule();
		assert(complete < 100);
		delay = Step::create(util::scaleByInversePercent(delay.get(), complete));
	}
	m_finishEvent.schedule(delay, *this, start);
}
void Project::haulSubprojectComplete(HaulSubproject& haulSubproject)
{
	auto workers = std::move(haulSubproject.m_workers);
	m_haulSubprojects.remove(haulSubproject);
	Actors& actors = m_area.getActors();
	for(ActorReference actor : workers)
	{
		m_workers[actor].haulSubproject = nullptr;
		commandWorker(actor.getIndex(actors.m_referenceData));
	}
}
void Project::haulSubprojectCancel(HaulSubproject& haulSubproject)
{
	Actors& actors = m_area.getActors();
	Items& items = m_area.getItems();
	addToPickup(haulSubproject.m_toHaul.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData), haulSubproject.m_projectRequirementCounts, haulSubproject.m_quantity);
	for(ActorReference actor : haulSubproject.m_workers)
	{
		ActorIndex index = actor.getIndex(actors.m_referenceData);
		actors.unfollowIfAny(index);
		actors.move_clearPath(index);
		actors.canReserve_clearAll(index);
		m_workers[actor].haulSubproject = nullptr;
		if(actors.canPickUp_exists(index))
		{
			ActorOrItemIndex wasCarrying = actors.canPickUp_tryToPutDownPolymorphic(index, actors.getLocation(index));
			if(wasCarrying.empty())
			{
				// Could not find a place to put down carrying.
				// Wander somewhere else, hopefully we can put down there.
				actors.objective_addTaskToStart(index, std::make_unique<WanderObjective>());
			}
			else
				commandWorker(index);
		}
		else
			commandWorker(index);
	}
	m_haulSubprojects.remove(haulSubproject);
}
void Project::setLocationDishonorCallback(std::unique_ptr<DishonorCallback> dishonorCallback)
{
	m_area.getBlocks().setReservationDishonorCallback(m_location, m_canReserve, std::move(dishonorCallback));
}
void Project::setDelayOn()
{
	m_delay = true;
	bool reset = canReset();
	onDelay();
	if(reset)
		m_tryToReserveEvent.schedule(Config::projectDelayAfterExauhstingSubprojectRetries, *this);
}
void Project::setDelayOff()
{
	m_delay = false;
	offDelay();
}
void Project::addToPickup(const ActorOrItemIndex& actorOrItem, ProjectRequirementCounts& counts, const Quantity& quantity)
{
	ActorOrItemReference ref = actorOrItem.toReference(m_area);
	if(m_toPickup.contains(ref))
		m_toPickup[ref].second += quantity;
	else
		m_toPickup.emplace(ref, &counts, quantity);
}
void Project::removeToPickup(const ActorOrItemIndex& actorOrItem, const Quantity& quantity)
{
	ActorOrItemReference ref = actorOrItem.toReference(m_area);
	if(m_toPickup[ref].second == quantity)
		m_toPickup.erase(ref);
	else
		m_toPickup[ref].second -= quantity;
}
void Project::reset()
{
	Actors& actors = m_area.getActors();
	assert(canReset());
	m_finishEvent.maybeUnschedule();
	m_tryToHaulEvent.maybeUnschedule();
	m_tryToReserveEvent.maybeUnschedule();
	m_tryToHaulThreadedTask.maybeCancel(m_area.m_simulation, &m_area);
	m_tryToAddWorkersThreadedTask.maybeCancel(m_area.m_simulation, &m_area);
	m_toConsume.clear();
	m_haulRetries = Quantity::create(0);
	m_requiredItems.clear();
	m_requiredActors.clear();
	m_toPickup.clear();
	// I guess we are doing this in case requirements change. Probably not needed.
	m_requirementsLoaded = false;
	ActorIndices workersAndCandidates = getWorkersAndCandidates();
	m_canReserve.deleteAllWithoutCallback();
	m_workers.clear();
	m_workerCandidatesAndTheirObjectives.clear();
	m_making.clear();
	m_haulSubprojects.clear();
	for(ActorIndex actor : workersAndCandidates)
	{
		actors.objective_reset(actor);
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
bool Project::canAddWorker(const ActorIndex& actor) const
{
	ActorReference ref = m_area.getActors().getReference(actor);
	assert(!m_making.contains(ref));
	assert(!m_workers.contains(ref));
	return m_maxWorkers > m_workers.size();
}
void Project::clearReservations()
{
	m_canReserve.deleteAllWithoutCallback();
}
void Project::clearReferenceFromRequiredItems(const ItemReference& ref)
{
	std::erase_if(m_requiredItems, [&](const auto& pair){ return pair.first.m_item.exists() && pair.first.m_item == ref; });
}
// For testing.
bool Project::hasCandidate(const ActorIndex& actor) const
{
	ActorReference ref = m_area.getActors().getReference(actor);
	return std::ranges::find(m_workerCandidatesAndTheirObjectives, ref, &std::pair<ActorReference, Objective*>::first) != m_workerCandidatesAndTheirObjectives.end();
}
bool Project::hasWorker(const ActorIndex& actor) const
{
	ActorReference ref = m_area.getActors().getReference(actor);
	return m_workers.contains(ref);
}
ActorIndices Project::getWorkersAndCandidates()
{
	ActorIndices output;
	output.reserve(m_workers.size() + m_workerCandidatesAndTheirObjectives.size());
	Actors& actors = m_area.getActors();
	for(auto& pair : m_workers)
		output.add(pair.first.getIndex(actors.m_referenceData));
	for(auto& pair : m_workerCandidatesAndTheirObjectives)
		output.add(pair.first.getIndex(actors.m_referenceData));
	return output;
}
std::vector<std::pair<ActorIndex, Objective*>> Project::getWorkersAndCandidatesWithObjectives()
{
	std::vector<std::pair<ActorIndex, Objective*>> output;
	output.reserve(m_workers.size() + m_workerCandidatesAndTheirObjectives.size());
	Actors& actors = m_area.getActors();
	for(auto& pair : m_workers)
		output.emplace_back(pair.first.getIndex(actors.m_referenceData), pair.second.objective);
	for(auto& pair : m_workerCandidatesAndTheirObjectives)
		output.emplace_back(pair.first.getIndex(actors.m_referenceData), pair.second);
	return output;
}
void BlockHasProjects::add(Project& project)
{
	FactionId faction = project.getFaction();
	assert(!m_data.contains(faction) || !m_data[faction].contains(&project));
	m_data[faction].insert(&project);
}
void BlockHasProjects::remove(Project& project)
{
	FactionId faction = project.getFaction();
	assert(m_data.contains(faction) && m_data[faction].contains(&project));
	if(m_data[faction].size() == 1)
		m_data.erase(faction);
	else
		m_data[faction].erase(&project);
}
Percent BlockHasProjects::getProjectPercentComplete(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return Percent::create(0);
	for(Project* project : m_data[faction])
		if(project->getPercentComplete() != 0)
			return project->getPercentComplete();
	return Percent::create(0);
}
Project* BlockHasProjects::get(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return nullptr;
	for(Project* project : m_data[faction])
		if(project->finishEventExists())
			return project;
	return nullptr;
}