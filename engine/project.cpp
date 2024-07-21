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
#include "terrainFacade.h"
#include <algorithm>
#include <memory>
#include <unordered_map>
// ProjectRequirementCounts
ProjectRequirementCounts::ProjectRequirementCounts(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) :
	required(data["required"].get<const Quantity>()), delivered(data["delivered"].get<Quantity>()), 
	reserved(data["reserved"].get<Quantity>()), consumed(data["consumed"].get<Quantity>()) { }
// Project worker.
ProjectWorker::ProjectWorker(const Json& data, DeserializationMemo& deserializationMemo) : 
	objective(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))
{
	assert(deserializationMemo.m_haulSubprojects.contains(data["haulSubproject"].get<uintptr_t>()));
	haulSubproject = deserializationMemo.m_haulSubprojects.at(data["haulSubproject"].get<uintptr_t>());
}
Json ProjectWorker::toJson() const
{
	return {{"haulSubproject", haulSubproject}, {"objective", &objective}};
}
// DishonorCallback.
ProjectRequiredShapeDishonoredCallback::ProjectRequiredShapeDishonoredCallback(const Json& data, DeserializationMemo& deserializationMemo, Area& area) : 
	m_project(*deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()))
{ 
	m_actorOrItem.load(data["actorOrItem"], area);
}
Json ProjectRequiredShapeDishonoredCallback::toJson() const 
{ 
	return {
		{"type", "ProjectRequiredShapeDishonoredCallback"}, 
		{"project", reinterpret_cast<uintptr_t>(&m_project)},
		{"actorOrItem", m_actorOrItem}
	};
}
void ProjectRequiredShapeDishonoredCallback::execute(Quantity oldCount, Quantity newCount)
{
	m_project.onActorOrItemReservationDishonored(m_actorOrItem.getIndexPolymorphic(), oldCount, newCount);
}
ProjectFinishEvent::ProjectFinishEvent(const Step delay, Project& p, const Step start) : 
	ScheduledEvent(p.m_area.m_simulation, delay, start), m_project(p) { }
void ProjectFinishEvent::execute(Simulation&, Area*) { m_project.complete(); }
void ProjectFinishEvent::clearReferences(Simulation&, Area*) { m_project.m_finishEvent.clearPointer(); }
ProjectTryToHaulEvent::ProjectTryToHaulEvent(const Step delay, Project& p, const Step start) : 
	ScheduledEvent(p.m_area.m_simulation, delay, start), m_project(p) { }
void ProjectTryToHaulEvent::execute(Simulation&, Area*) { m_project.m_tryToHaulThreadedTask.create(m_project); }
void ProjectTryToHaulEvent::clearReferences(Simulation&, Area*) { m_project.m_tryToHaulEvent.clearPointer(); }
ProjectTryToReserveEvent::ProjectTryToReserveEvent(const Step delay, Project& p, const Step start) : 
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
		ActorIndex actorIndex = actor.getIndex();
		std::function<bool(BlockIndex, Facing)> condition = [this, actorIndex](BlockIndex block, Facing) { return blockContainsDesiredItem(block, actorIndex); };

		// Path result is unused, running only for condition side effect.
		[[maybe_unused]] FindPathResult result = m_project.m_area.m_hasTerrainFacades.at(actors.getMoveType(actorIndex)).findPathAdjacentToConditionAndUnreserved(actors.getLocation(actorIndex), actors.getShape(actorIndex), actors.getFacing(actorIndex), condition, m_project.m_faction);
		// Only make at most one per step.
		// TODO: Would it be better to do them all at once instead of one per step? Better temporal locality, higher risk of lag spike.
		if(m_haulProjectParamaters.strategy != HaulStrategy::None)
			return;
	}
}
void ProjectTryToMakeHaulSubprojectThreadedTask::writeStep(Simulation&, Area*)
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
	else if(!m_haulProjectParamaters.validate(m_project.m_area))
	{
		ActorOrItemReference ref = m_haulProjectParamaters.toHaul;
		if(m_project.m_toPickup.at(ref).second >= m_haulProjectParamaters.quantity)
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
			m_project.removeToPickup(m_haulProjectParamaters.toHaul.getIndexPolymorphic(), m_haulProjectParamaters.quantity);
			m_project.onSubprojectCreated(subproject);
		}
	}
}
void ProjectTryToMakeHaulSubprojectThreadedTask::clearReferences(Simulation&, Area*) { m_project.m_tryToHaulThreadedTask.clearPointer(); }
bool ProjectTryToMakeHaulSubprojectThreadedTask::blockContainsDesiredItem(const BlockIndex block, ActorIndex actor)
{
	auto& blocks = m_project.m_area.getBlocks();
	Items& items = m_project.m_area.getItems();
	for(ItemIndex item : blocks.item_getAll(block))
	{
		ActorOrItemReference ref = ActorOrItemReference::createForItem(items.getReferenceTarget(item));
		if(m_project.m_toPickup.contains(ref))
		{
			m_haulProjectParamaters = HaulSubproject::tryToSetHaulStrategy(m_project, ref.getIndexPolymorphic(), actor);
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
	// location.  Accumulate resources that they can path to untill reservations
	// are complete.
	// If reservations are not complete flush the data from project.
	// TODO: Unwisely modifing data out side the object durring read step.
	std::unordered_set<ActorOrItemIndex, ActorOrItemIndex::Hash> recordedShapes;
	Actors& actors = m_project.m_area.getActors();
	Items& items = m_project.m_area.getItems();
	for(auto& [candidate, objective] : m_project.m_workerCandidatesAndTheirObjectives)
	{
		ActorIndex candidateIndex = candidate.getIndex();
		assert(!m_project.getWorkers().contains(candidate));
		// Verify the worker can path to the job site.
		TerrainFacade& terrainFacade = m_project.m_area.m_hasTerrainFacades.at(actors.getMoveType(candidateIndex));
		FindPathResult result = terrainFacade.findPathAdjacentToAndUnreserved(actors.getLocation(candidateIndex), actors.getShape(candidateIndex), actors.getFacing(candidateIndex), m_project.m_location, m_project.m_faction);
		if(result.path.empty() && !result.useCurrentPosition)
		{
			m_cannotPathToJobSite.insert(candidate);
			continue;
		}
		if(!m_project.reservationsComplete())
		{
			// Check if the worker can get to any required item or actor.
			for(ItemIndex item : actors.equipment_getAll(candidateIndex))
				for(auto& [itemQuery, projectRequirementCounts] : m_project.m_requiredItems)
				{
					if(projectRequirementCounts.required == projectRequirementCounts.reserved)
						continue;
					assert(!items.reservable_exists(item, m_project.m_faction));
					if(!itemQuery.query(m_project.m_area, item))
						continue;
					Quantity desiredQuantity = projectRequirementCounts.required - projectRequirementCounts.reserved;
					Quantity quantity = std::min(desiredQuantity, items.reservable_getUnreservedCount(item, m_project.m_faction));
					assert(quantity != 0);
					projectRequirementCounts.reserved += quantity;
					assert(projectRequirementCounts.reserved <= projectRequirementCounts.required);
					ItemReference itemRef = items.getReference(item);
					m_reservedEquipment[candidate].emplace_back(&projectRequirementCounts, itemRef);
				}
			auto recordItemOnGround = [&](ActorOrItemIndex actorOrItem, ProjectRequirementCounts& counts)
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
					m_alreadyAtSite[ref] = quantity;
				}
				else
					// TODO: project shoud be read only here, requires tracking reservationsComplete seperately for task.
					m_project.addToPickup(actorOrItem, counts, quantity);
			};
			// Verfy the worker can path to the required materials. Cumulative for all candidates in this step but reset if not satisfied.
			std::function<bool(BlockIndex, Facing)> predicate = [&](BlockIndex block, Facing)
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
							return true;
					}
				}
				for(ActorIndex actor : blocks.actor_getAll(block))
				{
					ActorOrItemIndex polymorphicActor = ActorOrItemIndex::createForItem(actor);
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
							return true;
					}
				}
				return false;
			};
			// TODO: Path is not used, find path is run for side effects of predicate.
			[[maybe_unused]] FindPathResult result = terrainFacade.findPathAdjacentToConditionAndUnreserved(actors.getLocation(candidateIndex), actors.getShape(candidateIndex), actors.getFacing(candidateIndex), predicate, m_project.m_faction);
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
		std::function<void()> callback = [this]{ resetProjectCounts(); };
		m_hasOnDestroy.setCallback(callback);
		for(auto& pair : m_reservedEquipment)
			for(std::pair<ProjectRequirementCounts*, ItemReference> pair2 : pair.second)
				items.onDestroy_subscribe(pair2.second.getIndex(), m_hasOnDestroy);
		for(auto& pair : m_alreadyAtSite)
			if(pair.first.isActor())
				actors.onDestroy_subscribe(pair.first.getIndex(), m_hasOnDestroy);
			else
				items.onDestroy_subscribe(pair.first.getIndex(), m_hasOnDestroy);
		// Accessing project here because we are modifing project from the readStep, probably unwisely.
		for(auto& pair : m_project.m_toPickup)
			if(pair.first.isActor())
				actors.onDestroy_subscribe(pair.first.getIndex(), m_hasOnDestroy);
			else
				items.onDestroy_subscribe(pair.first.getIndex(), m_hasOnDestroy);
	}
}
void ProjectTryToAddWorkersThreadedTask::writeStep(Simulation&, Area*)
{
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
				std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<ProjectRequiredShapeDishonoredCallback>(m_project, actorOrItem);
				actorOrItem.getIndexPolymorphic().reservable_reserve(m_project.m_area, m_project.m_canReserve, pair.second, std::move(dishonorCallback));
			}
			for(auto& [actorOrItem, quantity] : m_alreadyAtSite)
			{
				std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<ProjectRequiredShapeDishonoredCallback>(m_project, actorOrItem);
				actorOrItem.getIndexPolymorphic().reservable_reserve(m_project.m_area, m_project.m_canReserve, quantity, std::move(dishonorCallback));
			}
			m_project.m_alreadyAtSite = m_alreadyAtSite;
			for(auto& pair : m_reservedEquipment)
				for(std::pair<ProjectRequirementCounts*, ItemReference> pair2 : pair.second)
				{
					ActorOrItemReference ref;
					ref.setItem(m_project.m_area.getItems().getReferenceTarget(pair2.second.getIndex()));
					std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<ProjectRequiredShapeDishonoredCallback>(m_project, ref);
					items.reservable_reserve(pair2.second.getIndex(), m_project.m_canReserve, 1, std::move(dishonorCallback));
				}
			m_project.m_reservedEquipment = m_reservedEquipment;
		}
		// Add all actors.
		for(auto& [actor, objective] : m_project.m_workerCandidatesAndTheirObjectives)
		{
			ActorIndex actorIndex = actor.getIndex();
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
			toReleaseWithProjectDelay.insert(workers.begin(), workers.end());
			m_project.m_workers.clear();
			m_project.m_workerCandidatesAndTheirObjectives.clear();
			m_project.reset();
		}
		else
			m_project.cancel();
	}
	// These workers are not allowed to attempt to rejoin this project for a period, they may not be able to path to it or reserved required items.
	for(const ActorReference& actor : toReleaseWithProjectDelay)
		actors.objective_projectCannotReserve(actor.getIndex());
}
void ProjectTryToAddWorkersThreadedTask::clearReferences(Simulation&, Area*) { m_project.m_tryToAddWorkersThreadedTask.clearPointer(); }
bool ProjectTryToAddWorkersThreadedTask::validate()
{
	Items& items = m_project.m_area.getItems();
	// Require some workers can reach the job site.
	if(m_project.m_workerCandidatesAndTheirObjectives.empty())
		return false;
	// Ensure all items selected to be reserved are still reservable.
	for(auto& [actorOrItem, pair] : m_project.m_toPickup)
		if(actorOrItem.getIndexPolymorphic().reservable_getUnreservedCount(m_project.m_area, m_project.m_faction) < pair.second)
			return false;
	for(auto& [actorOrItem, quantity] : m_alreadyAtSite)
		if(actorOrItem.getIndexPolymorphic().reservable_getUnreservedCount(m_project.m_area, m_project.m_faction) < quantity)
			return false;
	for(auto& pair : m_reservedEquipment)
		for(const std::pair<ProjectRequirementCounts*, ItemReference>& pair2 : pair.second)
			if(items.reservable_exists(pair2.second.getIndex(), m_project.m_faction))
				return false;
	return true;
}
void ProjectTryToAddWorkersThreadedTask::resetProjectCounts()
{
	for(auto& pair : m_project.m_requiredItems)
	{
		pair.second.reserved = 0;
		pair.second.delivered = 0;
	}
	for(auto& pair : m_project.m_requiredActors)
	{
		pair.second.reserved = 0;
		pair.second.delivered = 0;
	}
	m_project.m_toPickup.clear();
}
// Derived classes are expected to provide getDuration, getConsumedItems, getUnconsumedItems, getByproducts, onDelay, offDelay, and onComplete.
Project::Project(FactionId f, Area& a, BlockIndex l, size_t mw, std::unique_ptr<DishonorCallback> locationDishonorCallback) : 
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
Project::Project(const Json& data, DeserializationMemo& deserializationMemo) : 
	m_finishEvent(deserializationMemo.m_simulation.m_eventSchedule), 
	m_tryToHaulEvent(deserializationMemo.m_simulation.m_eventSchedule), 
	m_tryToReserveEvent(deserializationMemo.m_simulation.m_eventSchedule), 
	m_tryToHaulThreadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), 
	m_tryToAddWorkersThreadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), 
	m_canReserve(data["faction"].get<FactionId>()), 
	m_area(deserializationMemo.area(data["area"])), m_faction(data["faction"].get<FactionId>()), 
	m_location(data["location"].get<BlockIndex>()), m_haulRetries(data["haulRetries"].get<Quantity>()), 
	m_minimumMoveSpeed(data["minimumMoveSpeed"].get<Speed>()),
	m_maxWorkers(data["maxWorkers"].get<Quantity>()), 
	m_requirementsLoaded(data["requirementsLoaded"].get<bool>()),
	m_delay(data["delay"].get<bool>()) 
{ 
	m_canReserve.load(data["canReserve"], deserializationMemo);
	if(data.contains("requiredItems"))
		for(const Json& pair : data["requiredItems"])
		{
			ItemQuery itemQuery(pair[0], m_area);
			ProjectRequirementCounts requirementCounts(pair[1], deserializationMemo);
			m_requiredItems.emplace_back(std::make_pair(itemQuery, requirementCounts));
			deserializationMemo.m_projectRequirementCounts[pair[1]["address"].get<uintptr_t>()] = &m_requiredItems.back().second;
		}
	if(data.contains("requiredActors"))
		for(const Json& pair : data["requiredActors"])
		{
			ActorQuery actorQuery(pair[0], m_area);
			ProjectRequirementCounts requirementCounts(pair[1], deserializationMemo);
			m_requiredActors.emplace_back(std::make_pair(actorQuery, requirementCounts));
			deserializationMemo.m_projectRequirementCounts[pair[1]["address"].get<uintptr_t>()] = &m_requiredActors.back().second;
		}
	if(data.contains("toConsume"))
		for(const Json& itemIndex : data["toConsume"])
			m_toConsume.add(itemIndex.get<ItemIndex>(), m_area);
	if(data.contains("toPickup"))
		for(const Json& tuple : data["toPickup"])
		{
			ActorOrItemIndex actorOrItem;
			if(tuple[0].contains("item"))
			{
				ItemIndex item = tuple[0]["item"].get<ItemIndex>();
				actorOrItem = ActorOrItemIndex::createForItem(item);
			}
			else
			{
				ActorIndex actor = tuple[0]["actor"].get<ActorIndex>();
				actorOrItem = ActorOrItemIndex::createForActor(actor);
			}
			ProjectRequirementCounts& projectRequirementCounts = *deserializationMemo.m_projectRequirementCounts.at(tuple[1].get<uintptr_t>());
			Quantity quantity = tuple[2].get<Quantity>();
			ActorOrItemReference ref = actorOrItem.toReference(m_area);
			m_toPickup[ref] = std::make_pair(&projectRequirementCounts, quantity);
		}
	if(data.contains("haulSubprojects"))
		for(const Json& haulSubprojectData : data["haulSubprojects"])
			m_haulSubprojects.emplace_back(haulSubprojectData, *this, deserializationMemo);
	m_canReserve.load(data["canReserve"], deserializationMemo);
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
			ActorIndex actor = pair[0].get<ActorIndex>();
			m_workers.try_emplace(actors.getReference(actor), pair[1], deserializationMemo);
			actors.project_set(actor, *this);
		}
	if(data.contains("candidates"))
		for(const Json& pair : data["candidates"])
		{
			ActorIndex actor = pair[0].get<ActorIndex>();
			assert(deserializationMemo.m_objectives.contains(pair[1].get<uintptr_t>()));
			Objective& objective = *deserializationMemo.m_objectives.at(pair[1].get<uintptr_t>());
			m_workerCandidatesAndTheirObjectives.emplace_back(actors.getReference(actor), &objective);
			actors.project_set(actor, *this);
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
			Json pair({actorQuery, requiredCounts});
			data["requiredActors"].push_back(pair);
		}
	}
	if(!m_toConsume.empty())
	{
		data["toConsume"] = Json::array();
		for(ItemReference item : m_toConsume)
			data["toConsume"].push_back(item);
	}
	if(!m_toPickup.empty())
	{
		data["toPickup"] = Json::array();
		for(auto& [actorOrItem, pair] : m_toPickup)
			data["toPickup"].emplace_back(actorOrItem, *pair.first, pair.second);
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
void Project::addWorker(ActorIndex actor, Objective& objective)
{
	assert(!m_workers.contains(m_area.getActors().getReference(actor)));
	assert(m_area.getActors().isSentient(actor));
	assert(canAddWorker(actor));
	// Initalize a ProjectWorker for this worker.
	m_workers.emplace(actor, objective);
	commandWorker(actor);
}
void Project::addWorkerCandidate(ActorIndex actor, Objective& objective)
{
	Actors& actors = m_area.getActors();
	assert(actors.project_exists(actor));
	assert(canAddWorker(actor));
	actors.project_set(actor, *this);
	if(!m_requirementsLoaded)
		recordRequiredActorsAndItems();
	ActorReference actorRef = actors.getReference(actor);
	assert(std::ranges::find(m_workerCandidatesAndTheirObjectives, actorRef, &std::pair<ActorReference, Objective*>::first) == m_workerCandidatesAndTheirObjectives.end());
	m_workerCandidatesAndTheirObjectives.emplace_back(actorRef, &objective);
	if(!m_tryToAddWorkersThreadedTask.exists())
		m_tryToAddWorkersThreadedTask.create(*this);
}
void Project::removeWorkerCandidate(ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	auto iter = std::ranges::find(m_workerCandidatesAndTheirObjectives, actorRef, &std::pair<ActorReference, Objective*>::first);
	assert(iter != m_workerCandidatesAndTheirObjectives.end());
	m_workerCandidatesAndTheirObjectives.erase(iter);
}
// To be called by Objective::execute.
void Project::commandWorker(ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	if(!m_workers.contains(actorRef))
	{
		assert(std::ranges::find(m_workerCandidatesAndTheirObjectives, actorRef, &std::pair<ActorReference, Objective*>::first) != m_workerCandidatesAndTheirObjectives.end());
		// Candidates do nothing but wait.
		return;
	}
	if(m_workers.at(actorRef).haulSubproject != nullptr)
		// The worker has been dispatched to fetch an item.
		m_workers.at(actorRef).haulSubproject->commandWorker(actor);
	else
	{
		// The worker has not been dispatched to fetch an actor or an item.
		if(deliveriesComplete())
		{
			// All items and actors have been delivered, the worker starts making.
			if(actors.isAdjacentToLocation(actor, m_location))
				addToMaking(actor);
			else
				actors.move_setDestinationAdjacentToLocation(actor, m_location, m_workers.at(actorRef).objective.m_detour);
		}
		else if(m_toPickup.empty())
		{
			if(actors.isAdjacentToLocation(actor, m_location))
			{
				if(canRecruitHaulingWorkersOnly())
				{
					assert(!m_reservedEquipment.contains(actorRef));
					// We have no use for this worker so release them.
					actors.objective_canNotCompleteObjective(actor, m_workers.at(actorRef).objective);
				}
				else
				{
					// Any tools being carried are marked delivered.
					if(m_reservedEquipment.contains(actorRef))
					{
						for(auto& pair : m_reservedEquipment.at(actorRef))
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
					m_waiting.add(actorRef);
				}
			}
			else
				actors.move_setDestinationAdjacentToLocation(actor, m_location, m_workers.at(actorRef).objective.m_detour);
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
void Project::removeWorker(ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	Items& items = m_area.getItems();
	assert(actors.project_get(actor) == this);
	if(hasCandidate(actor))
	{
		removeWorkerCandidate(actor);
		return;
	}
	assert(m_workers.contains(actorRef));
	actors.project_unset(actor);
	if(m_workers.at(actorRef).haulSubproject != nullptr)
		m_workers.at(actorRef).haulSubproject->removeWorker(actor);
	if(m_making.contains(actorRef))
		removeFromMaking(actor);
	m_workers.erase(actorRef);
	if(m_reservedEquipment.contains(actorRef))
	{
		ItemReference item = m_reservedEquipment.at(actorRef).front().second;
		items.reservable_unreserveAll(item.getIndex());
	}
	else if(m_workers.empty())
	{
		if(canReset())
			reset();
		else
			cancel();
	}
}
void Project::addToMaking(ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	assert(m_workers.contains(actorRef));
	assert(!m_making.contains(actorRef));
	m_making.add(actorRef);
	onAddToMaking(actor);
	scheduleFinishEvent();
}
void Project::removeFromMaking(ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	ActorReference actorRef = actors.getReference(actor);
	assert(m_workers.contains(actorRef));
	assert(m_making.contains(actorRef));
	m_making.remove(actorRef);
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
	for(ItemReference item : m_toConsume)
		items.destroy(item.getIndex());
	if(!m_area.m_hasStockPiles.contains(m_faction))
		m_area.m_hasStockPiles.registerFaction(m_faction);
	for(auto& [itemType, materialType, quantity] : getByproducts())
	{
		ItemIndex item = blocks.item_addGeneric(m_location, *itemType, *materialType, quantity);
		// Item may be newly created or it may be prexisting, and thus already designated for stockpileing.
		if(!items.stockpile_canBeStockPiled(item, m_faction))
			m_area.m_hasStockPiles.at(m_faction).addItem(item);
	}
	for(auto& [actor, projectWorker] : m_workers)
		actors.project_unset(actor.getIndex());
	onComplete();
}
void Project::cancel()
{
	m_canReserve.deleteAllWithoutCallback();
	m_area.getBlocks().project_remove(m_location, *this);
	//for(auto& [actor, projectWorker] : m_workers)
		//actor->m_project = nullptr;
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
		delay = util::scaleByPercent(delay, 100 - complete);
	}
	m_finishEvent.schedule(delay, *this, start);
}
void Project::haulSubprojectComplete(HaulSubproject& haulSubproject)
{
	auto workers = std::move(haulSubproject.m_workers);
	m_haulSubprojects.remove(haulSubproject);
	for(ActorReference actor : workers)
	{
		m_workers.at(actor).haulSubproject = nullptr;
		commandWorker(actor.getIndex());
	}
}
void Project::haulSubprojectCancel(HaulSubproject& haulSubproject)
{
	Actors& actors = m_area.getActors();
	addToPickup(haulSubproject.m_toHaul.getIndexPolymorphic(), haulSubproject.m_projectRequirementCounts, haulSubproject.m_quantity);
	for(ActorReference actor : haulSubproject.m_workers)
	{
		ActorIndex index = actor.getIndex();
		actors.canPickUp_putDownIfAny(index, actors.getLocation(index));
		actors.unfollowIfAny(index);
		actors.move_clearPath(index);
		actors.canReserve_clearAll(index);
		m_workers.at(actor).haulSubproject = nullptr;
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
void Project::addToPickup(ActorOrItemIndex actorOrItem, ProjectRequirementCounts& counts, Quantity quantity)
{
	ActorOrItemReference ref = actorOrItem.toReference(m_area);
	if(m_toPickup.contains(ref))
		m_toPickup[ref].second += quantity;
	else
		m_toPickup.try_emplace(ref, &counts, quantity);
}
void Project::removeToPickup(ActorOrItemIndex actorOrItem, Quantity quantity)
{
	ActorOrItemReference ref = actorOrItem.toReference(m_area);
	if(m_toPickup.at(ref).second == quantity)
		m_toPickup.erase(ref);
	else
		m_toPickup.at(ref).second -= quantity;
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
	m_haulRetries = 0;
	m_requiredItems.clear();
	m_requiredActors.clear();
	m_toPickup.clear();
	// I guess we are doing this in case requirements change. Probably not needed.
	m_requirementsLoaded = false;
	std::vector<ActorIndex> workersAndCandidates = getWorkersAndCandidates();
	m_canReserve.deleteAllWithoutCallback();
	m_workers.clear();
	m_workerCandidatesAndTheirObjectives.clear();
	m_making.clear();
	for(ActorIndex actor : workersAndCandidates)
	{
		actors.objective_reset(actor);
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
bool Project::canAddWorker([[maybe_unused]] const ActorIndex actor) const
{
	ActorReference ref = m_area.getActors().getReference(actor);
	assert(!m_workers.contains(ref));
	return m_maxWorkers > m_workers.size();
}
void Project::clearReservations()
{
	m_canReserve.deleteAllWithoutCallback();
}
// For testing.
bool Project::hasCandidate(const ActorIndex actor) const
{
	ActorReference ref = m_area.getActors().getReference(actor);
	return std::ranges::find(m_workerCandidatesAndTheirObjectives, ref, &std::pair<ActorReference, Objective*>::first) != m_workerCandidatesAndTheirObjectives.end();
}
std::vector<ActorIndex> Project::getWorkersAndCandidates()
{
	std::vector<ActorIndex> output;
	output.reserve(m_workers.size() + m_workerCandidatesAndTheirObjectives.size());
	for(auto& pair : m_workers)
		output.push_back(pair.first.getIndex());
	for(auto& pair : m_workerCandidatesAndTheirObjectives)
		output.push_back(pair.first.getIndex());
	return output;
}
std::vector<std::pair<ActorIndex, Objective*>> Project::getWorkersAndCandidatesWithObjectives()
{
	std::vector<std::pair<ActorIndex, Objective*>> output;
	output.reserve(m_workers.size() + m_workerCandidatesAndTheirObjectives.size());
	for(auto& pair : m_workers)
		output.emplace_back(pair.first.getIndex(), &pair.second.objective);
	for(auto& pair : m_workerCandidatesAndTheirObjectives)
		output.emplace_back(pair.first.getIndex(), pair.second);
	return output;
}
void BlockHasProjects::add(Project& project)
{
	FactionId faction = project.getFaction();
	assert(!m_data.contains(faction) || !m_data.at(faction).contains(&project));
	m_data[faction].insert(&project);
}
void BlockHasProjects::remove(Project& project)
{
	FactionId faction = project.getFaction();
	assert(m_data.contains(faction) && m_data.at(faction).contains(&project));
	if(m_data[faction].size() == 1)
		m_data.erase(faction);
	else
		m_data[faction].erase(&project);
}
Percent BlockHasProjects::getProjectPercentComplete(FactionId faction) const
{
	if(!m_data.contains(faction))
		return 0;
	for(Project* project : m_data.at(faction))
		if(project->getPercentComplete())
			return project->getPercentComplete();
	return 0;
}
Project* BlockHasProjects::get(FactionId faction) const
{
	if(!m_data.contains(faction))
		return nullptr;
	for(Project* project : m_data.at(faction))
		if(project->finishEventExists())
			return project;
	return nullptr;
}
