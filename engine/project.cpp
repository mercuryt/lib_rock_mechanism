#include "project.h"
#include "block.h"
#include "area.h"
#include "config.h"
#include "deserializationMemo.h"
#include "hasShape.h"
#include "haul.h"
#include "reservable.h"
#include "simulation.h"
#include <algorithm>
#include <memory>
// ProjectRequirementCounts
ProjectRequirementCounts::ProjectRequirementCounts(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) :
	required(data["required"].get<const uint8_t>()), delivered(data["delivered"].get<uint8_t>()), 
	reserved(data["reserved"].get<uint8_t>()), consumed(data["consumed"].get<uint8_t>()) { }
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
ProjectRequiredShapeDishonoredCallback::ProjectRequiredShapeDishonoredCallback(const Json& data, DeserializationMemo& deserializationMemo) : 
	m_project(*deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())), 
	m_hasShape(data.contains("hasShapeItem") ? static_cast<HasShape&>(deserializationMemo.itemReference(data["hasShapeItem"])) : static_cast<HasShape&>(deserializationMemo.actorReference(data["hasShapeActor"]))) { }
Json ProjectRequiredShapeDishonoredCallback::toJson() const 
{ 
	Json data{
			{"type", "ProjectRequiredShapeDishonoredCallback"}, 
			{"project", reinterpret_cast<uintptr_t>(&m_project)}
		};
	if(m_hasShape.isItem())
		data["hasShapeItem"] = static_cast<Item&>(m_hasShape).m_id;
	else
	{
		assert(m_hasShape.isActor());
		data["hasShapeActor"] = static_cast<Actor&>(m_hasShape).m_id;
	}
	return data;
}
void ProjectRequiredShapeDishonoredCallback::execute(uint32_t oldCount, uint32_t newCount)
{
	m_project.onHasShapeReservationDishonored(m_hasShape, oldCount, newCount);
}
ProjectFinishEvent::ProjectFinishEvent(const Step delay, Project& p, const Step start) : ScheduledEvent(p.m_location.m_area->m_simulation, delay, start), m_project(p) {}
void ProjectFinishEvent::execute() { m_project.complete(); }
void ProjectFinishEvent::clearReferences() { m_project.m_finishEvent.clearPointer(); }

ProjectTryToHaulEvent::ProjectTryToHaulEvent(const Step delay, Project& p, const Step start) : ScheduledEvent(p.m_location.m_area->m_simulation, delay, start), m_project(p) { }
void ProjectTryToHaulEvent::execute() { m_project.m_tryToHaulThreadedTask.create(m_project); }
void ProjectTryToHaulEvent::clearReferences() { m_project.m_tryToHaulEvent.clearPointer(); }

ProjectTryToReserveEvent::ProjectTryToReserveEvent(const Step delay, Project& p, const Step start) : ScheduledEvent(p.m_location.m_area->m_simulation, delay, start), m_project(p) { }
void ProjectTryToReserveEvent::execute() 
{ 
	m_project.setDelayOff();
}
void ProjectTryToReserveEvent::clearReferences() { m_project.m_tryToReserveEvent.clearPointer(); }

ProjectTryToMakeHaulSubprojectThreadedTask::ProjectTryToMakeHaulSubprojectThreadedTask(Project& p) : ThreadedTask(p.m_location.m_area->m_simulation.m_threadedTaskEngine), m_project(p) { }
void ProjectTryToMakeHaulSubprojectThreadedTask::readStep()
{
	for(auto& [actor, projectWorker] : m_project.m_workers)
	{
		if(projectWorker.haulSubproject != nullptr)
			continue;
		std::function<bool(const Block&)> condition = [this, actor](const Block& block) { return blockContainsDesiredItem(block, *actor); };
		FindsPath findsPath(*actor, false);
		findsPath.pathToUnreservedAdjacentToPredicate(condition, *actor->getFaction());
		// Only make at most one per step.
		if(m_haulProjectParamaters.strategy != HaulStrategy::None)
			return;
	}
}
void ProjectTryToMakeHaulSubprojectThreadedTask::writeStep()
{
	if(m_haulProjectParamaters.strategy == HaulStrategy::None)
	{
		if(++m_project.m_haulRetries == Config::projectTryToMakeSubprojectRetriesBeforeProjectDelay)
		{
			// Enough retries, delay.
			m_project.m_haulRetries = 0;
			m_project.setDelayOn();
		}
		else
		{
			// Nothing haulable found, wait a bit and try again.
			if(m_project.m_minimumMoveSpeed > 1)
				--m_project.m_minimumMoveSpeed;
			m_project.m_tryToHaulEvent.schedule(Config::stepsFrequencyToLookForHaulSubprojects, m_project);
		}
	}
	else if(!m_haulProjectParamaters.validate() && m_project.m_toPickup.at(m_haulProjectParamaters.toHaul).second >= m_haulProjectParamaters.quantity)
	{
		// Something that was avalible during read step is now reserved. Ensure that there is a new threaded task to try again.
		if(!m_project.m_tryToHaulThreadedTask.exists())
			m_project.m_tryToHaulThreadedTask.create(m_project);
	}
	else
	{
		// All guards passed, create subproject.
		assert(!m_haulProjectParamaters.workers.empty());
		HaulSubproject& subproject = m_project.m_haulSubprojects.emplace_back(m_project, m_haulProjectParamaters);
		// Remove the target of the new haulSubproject from m_toPickup;
		m_project.removeToPickup(*m_haulProjectParamaters.toHaul, m_haulProjectParamaters.quantity);
		m_project.onSubprojectCreated(subproject);
	}
}
void ProjectTryToMakeHaulSubprojectThreadedTask::clearReferences() { m_project.m_tryToHaulThreadedTask.clearPointer(); }
bool ProjectTryToMakeHaulSubprojectThreadedTask::blockContainsDesiredItem(const Block& block, Actor& actor)
{
	for(Item* item : block.m_hasItems.getAll())
		if(m_project.m_toPickup.contains(item))
		{
			m_haulProjectParamaters = HaulSubproject::tryToSetHaulStrategy(m_project, *item, actor);
			if(m_haulProjectParamaters.strategy != HaulStrategy::None)
				return true;
		}
	return false;
};
ProjectTryToAddWorkersThreadedTask::ProjectTryToAddWorkersThreadedTask(Project& p) : ThreadedTask(p.m_location.m_area->m_simulation.m_threadedTaskEngine), m_project(p) { }
void ProjectTryToAddWorkersThreadedTask::readStep()
{
	// Iterate candidate workers, verify that they can path to the project
	// location.  Accumulate resources that they can path to untill reservations
	// are complete.
	// If reservations are not complete flush the data from project.
	// TODO: Unwisely modifing data out side the object durring read step.
	for(auto& [candidate, objective] : m_project.m_workerCandidatesAndTheirObjectives)
	{
		FindsPath findsPath(*candidate, false);
		// Verify the worker can path to the job site.
		findsPath.pathAdjacentToBlock(m_project.m_location);
		if(!findsPath.found())
		{
			m_cannotPathToJobSite.insert(candidate);
			continue;
		}
		for(Item* item : candidate->m_equipmentSet.getAll())
			for(auto& [itemQuery, projectRequirementCounts] : m_project.m_requiredItems)
			{
				if(projectRequirementCounts.required == projectRequirementCounts.reserved)
					continue;
				assert(!item->m_reservable.hasAnyReservationsWith(m_project.m_faction));
				if(!itemQuery.query(*item))
					continue;
				uint32_t desiredQuantity = projectRequirementCounts.required - projectRequirementCounts.reserved;
				uint32_t quantity = std::min(desiredQuantity, item->m_reservable.getUnreservedCount(m_project.m_faction));
				assert(quantity != 0);
				projectRequirementCounts.reserved += quantity;
				assert(projectRequirementCounts.reserved <= projectRequirementCounts.required);
				m_reservedEquipment[candidate].emplace_back(&projectRequirementCounts, item);
			}

		if(!m_project.reservationsComplete())
		{
			auto recordItemOnGround = [&](HasShape& hasShape, ProjectRequirementCounts& counts)
			{
				uint32_t desiredQuantity = counts.required - counts.reserved;
				uint32_t quantity = std::min(desiredQuantity, hasShape.m_reservable.getUnreservedCount(m_project.m_faction));
				assert(quantity != 0);
				counts.reserved += quantity;
				assert(counts.reserved <= counts.required);
				if(*hasShape.m_location == m_project.m_location || hasShape.isAdjacentTo(m_project.m_location))
				{
					// Item is already at or next to project location.
					counts.delivered += quantity;
					assert(counts.delivered <= counts.reserved);
					assert(!m_alreadyAtSite.contains(&hasShape));
					m_alreadyAtSite[&hasShape] = quantity;
				}
				else
					// TODO: project shoud be read only here, requires tracking reservationsComplete seperately for task.
					m_project.addToPickup(hasShape, counts, quantity);
			};
			// Verfy the worker can path to the required materials. Cumulative for all candidates in this step but reset if not satisfied.
			std::function<bool(const Block&)> predicate = [&](const Block& block)
			{
				for(Item* item : block.m_hasItems.getAll())
					for(auto& [itemQuery, projectRequirementCounts] : m_project.m_requiredItems)
					{
						if(projectRequirementCounts.required == projectRequirementCounts.reserved)
							continue;
						if(item->m_reservable.isFullyReserved(&m_project.m_faction))
							continue;	
						if(!itemQuery.query(*item))
							continue;
						recordItemOnGround(*item, projectRequirementCounts);
						if(m_project.reservationsComplete())
							return true;
					}
				for(Actor* actor : block.m_hasActors.getAll())
					for(auto& [actorQuery, projectRequirementCounts] : m_project.m_requiredActors)
					{
						if(projectRequirementCounts.required == projectRequirementCounts.reserved)
							continue;
						if(actor->m_reservable.isFullyReserved(&m_project.m_faction))
							continue;	
						if(!actorQuery.query(*actor))
							continue;
						recordItemOnGround(*actor, projectRequirementCounts);
						if(m_project.reservationsComplete())
							return true;
					}
				return false;
			};
			// TODO: Path is not used.
			findsPath.reset();
			findsPath.pathToAdjacentToPredicate(predicate);
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
			for(std::pair<ProjectRequirementCounts*, Item*> pair2 : pair.second)
				m_hasOnDestroy.subscribe(pair2.second->m_onDestroy);
		for(auto& pair : m_alreadyAtSite)
			if(pair.first->isItem())
				m_hasOnDestroy.subscribe(pair.first->m_onDestroy);
		// Accessing project here because we are modifing project from the readStep, probably unwisely.
		for(auto& pair : m_project.m_toPickup)
			if(pair.first->isItem())
				m_hasOnDestroy.subscribe(pair.first->m_onDestroy);
	}
}
void ProjectTryToAddWorkersThreadedTask::writeStep()
{
	m_hasOnDestroy.unsubscribeAll();
	if(m_project.reservationsComplete() && !m_project.m_workerCandidatesAndTheirObjectives.empty())
	{
		// Requirements satisfied, verifiy they are all still avalible.
		if(!validate())
		{
			// Some selected hasShape was probably reserved or destroyed, try again.
			resetProjectCounts();
			m_project.m_tryToAddWorkersThreadedTask.create(m_project);
			return;
		}
		// Reserve all required.
		for(auto& [hasShape, pair] : m_project.m_toPickup)
		{
			std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<ProjectRequiredShapeDishonoredCallback>(m_project, *hasShape);
			hasShape->m_reservable.reserveFor(m_project.m_canReserve, pair.second, std::move(dishonorCallback));
		}
		for(auto& [hasShape, quantity] : m_alreadyAtSite)
		{
			std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<ProjectRequiredShapeDishonoredCallback>(m_project, *hasShape);
			hasShape->m_reservable.reserveFor(m_project.m_canReserve, quantity, std::move(dishonorCallback));
		}
		for(auto& pair : m_reservedEquipment)
			for(std::pair<ProjectRequirementCounts*, Item*> pair2 : pair.second)
			{
				std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<ProjectRequiredShapeDishonoredCallback>(m_project, *pair2.second);
				pair2.second->m_reservable.reserveFor(m_project.m_canReserve, 1, std::move(dishonorCallback));
			}
		m_project.m_reservedEquipment = m_reservedEquipment;
		// Add all actors.
		std::vector<Actor*> notAdded;
		for(auto& [actor, objective] : m_project.m_workerCandidatesAndTheirObjectives)
		{
			// TODO: Reserve a work location for the actor, prevent deadlocks due to lack of access by a tool holder.
			if(m_project.canAddWorker(*actor))
				m_project.addWorker(*actor, *objective);
			else
				notAdded.push_back(actor);
		}
		m_project.m_workerCandidatesAndTheirObjectives.clear();
		m_project.onReserve();
		for(Actor* actor : notAdded)
		{
			actor->m_hasObjectives.getCurrent().reset();
			actor->m_project = nullptr;
			actor->m_hasObjectives.cannotCompleteSubobjective();
		}
	}
	else
	{
		// Could not find required items / actors, activate delay and reset.
		if(m_project.canReset())
		{
			m_project.setDelayOn();
			m_project.reset();
			m_project.m_tryToReserveEvent.schedule(Config::stepsToDelayBeforeTryingAgainToReserveItemsAndActorsForAProject, m_project);
		}
		else
			m_project.cancel();
	}
	for(Actor* actor : m_cannotPathToJobSite)
		actor->m_hasObjectives.cannotCompleteSubobjective();
}
void ProjectTryToAddWorkersThreadedTask::clearReferences() { m_project.m_tryToAddWorkersThreadedTask.clearPointer(); }
bool ProjectTryToAddWorkersThreadedTask::validate()
{
	// Ensure all items selected to be reserved are still reservable.
	for(auto& [hasShape, pair] : m_project.m_toPickup)
		if(hasShape->m_reservable.getUnreservedCount(m_project.m_faction) < pair.second)
			return false;
	for(auto& [hasShape, quantity] : m_alreadyAtSite)
		if(hasShape->m_reservable.getUnreservedCount(m_project.m_faction) < quantity)
			return false;
	for(auto& pair : m_reservedEquipment)
		for(const std::pair<ProjectRequirementCounts*, Item*>& pair2 : pair.second)
			if(pair2.second->m_reservable.hasAnyReservationsWith(m_project.m_faction))
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
Project::Project(const Faction* f, Block& l, size_t mw, std::unique_ptr<DishonorCallback> locationDishonorCallback) : m_finishEvent(l.m_area->m_simulation.m_eventSchedule), m_tryToHaulEvent(l.m_area->m_simulation.m_eventSchedule), m_tryToReserveEvent(l.m_area->m_simulation.m_eventSchedule), m_tryToHaulThreadedTask(l.m_area->m_simulation.m_threadedTaskEngine), m_tryToAddWorkersThreadedTask(l.m_area->m_simulation.m_threadedTaskEngine), m_canReserve(f), m_maxWorkers(mw), m_delay(false), m_haulRetries(0), m_requirementsLoaded(false), m_minimumMoveSpeed(Config::minimumHaulSpeedInital), m_location(l), m_faction(*f)
{
	m_location.m_reservable.reserveFor(m_canReserve, 1u, std::move(locationDishonorCallback));
	m_location.m_hasProjects.add(*this);
}
Project::Project(const Json& data, DeserializationMemo& deserializationMemo) : m_finishEvent(deserializationMemo.m_simulation.m_eventSchedule), m_tryToHaulEvent(deserializationMemo.m_simulation.m_eventSchedule), m_tryToReserveEvent(deserializationMemo.m_simulation.m_eventSchedule), m_tryToHaulThreadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), m_tryToAddWorkersThreadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), 
	m_canReserve(&deserializationMemo.faction(data["faction"].get<std::wstring>())),
	m_maxWorkers(data["maxWorkers"].get<uint32_t>()), m_delay(data["delay"].get<bool>()), 
	m_haulRetries(data["haulRetries"].get<uint8_t>()), m_requirementsLoaded(data["requirementsLoaded"].get<bool>()), 
	m_minimumMoveSpeed(data["minimumMoveSpeed"].get<uint32_t>()),
	m_location(deserializationMemo.m_simulation.getBlockForJsonQuery(data["location"])), 
	m_faction(deserializationMemo.faction(data["faction"].get<std::wstring>())) 
{ 
	if(data.contains("requiredItems"))
		for(const Json& pair : data["requiredItems"])
		{
			ItemQuery itemQuery(pair[0], deserializationMemo);
			ProjectRequirementCounts requirementCounts(pair[1], deserializationMemo);
			m_requiredItems.emplace_back(std::make_pair(itemQuery, requirementCounts));
			deserializationMemo.m_projectRequirementCounts[pair[1]["address"].get<uintptr_t>()] = &m_requiredItems.back().second;
		}
	if(data.contains("requiredActors"))
		for(const Json& pair : data["requiredActors"])
		{
			ActorQuery actorQuery(pair[0], deserializationMemo);
			ProjectRequirementCounts requirementCounts(pair[1], deserializationMemo);
			m_requiredActors.emplace_back(std::make_pair(actorQuery, requirementCounts));
			deserializationMemo.m_projectRequirementCounts[pair[1]["address"].get<uintptr_t>()] = &m_requiredActors.back().second;
		}
	if(data.contains("toConsume"))
		for(const Json& itemId : data["toConsume"])
			m_toConsume.insert(&deserializationMemo.m_simulation.getItemById(itemId.get<ItemId>()));
	if(data.contains("toPickup"))
		for(const Json& tuple : data["toPickup"])
		{
			HasShape* hasShape = nullptr;
			if(tuple[0].contains("item"))
			{
				Item& item = deserializationMemo.itemReference(tuple[0]["item"]);
				hasShape = static_cast<HasShape*>(&item);
			}
			else
			{
				Actor& actor = deserializationMemo.actorReference(tuple[0]["actor"]);
				hasShape = static_cast<HasShape*>(&actor);
			}
			ProjectRequirementCounts& projectRequirementCounts = *deserializationMemo.m_projectRequirementCounts.at(tuple[1].get<uintptr_t>());
			uint32_t quantity = tuple[2].get<uint32_t>();
			m_toPickup[hasShape] = std::make_pair(&projectRequirementCounts, quantity);
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
	m_location.m_hasProjects.add(*this);
}
void Project::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	if(data.contains("workers"))
		for(const Json& pair : data["workers"])
		{
			Actor& actor = deserializationMemo.actorReference(pair[0]);
			m_workers.try_emplace(&actor, pair[1], deserializationMemo);
			actor.m_project = this;
		}
	if(data.contains("candidates"))
		for(const Json& pair : data["candidates"])
		{
			Actor& actor = deserializationMemo.actorReference(pair[0]);
			assert(deserializationMemo.m_objectives.contains(pair[1].get<uintptr_t>()));
			Objective& objective = *deserializationMemo.m_objectives.at(pair[1].get<uintptr_t>());
			m_workerCandidatesAndTheirObjectives.emplace_back(&actor, &objective);
			actor.m_project = this;
		}
}
Json Project::toJson() const
{
	Json data({
		{"address", reinterpret_cast<uintptr_t>(this)},
		{"faction", m_faction.name},
		{"maxWorkers", m_maxWorkers},
		{"delay", m_delay},
		{"haulRetries", m_haulRetries},
		{"location", m_location.positionToJson()},
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
			Json pair({itemQuery.toJson(), requiredCounts});
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
		for(Item* item : m_toConsume)
			data["toConsume"].push_back(item->m_id);
	}
	if(!m_toPickup.empty())
	{
		data["toPickup"] = Json::array();
		for(auto& [hasShape, pair] : m_toPickup)
		{
			Json jsonTuple({nullptr, *pair.first, pair.second});
			if(hasShape->isItem())
				jsonTuple[0] = {{"item", static_cast<Item*>(hasShape)->m_id}};
			else
			{
				assert(hasShape->isActor());
				jsonTuple[0] = {{"actor",static_cast<Actor*>(hasShape)->m_id}};
			}
			data["toPickup"].emplace_back(jsonTuple);
		}
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
	for(auto& pair : m_reservedEquipment)
		if(!m_waiting.contains(pair.first))
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
void Project::addWorker(Actor& actor, Objective& objective)
{
	assert(!m_workers.contains(&actor));
	assert(actor.isSentient());
	assert(canAddWorker(actor));
	// Initalize a ProjectWorker for this worker.
	m_workers.emplace(&actor, objective);
	commandWorker(actor);
}
void Project::addWorkerCandidate(Actor& actor, Objective& objective)
{
	assert(actor.m_project == nullptr);
	assert(canAddWorker(actor));
	actor.m_project = this;
	if(!m_requirementsLoaded)
		recordRequiredActorsAndItems();
	assert(std::ranges::find(m_workerCandidatesAndTheirObjectives, &actor, &std::pair<Actor*, Objective*>::first) == m_workerCandidatesAndTheirObjectives.end());
	m_workerCandidatesAndTheirObjectives.emplace_back(&actor, &objective);
	if(!m_tryToAddWorkersThreadedTask.exists())
		m_tryToAddWorkersThreadedTask.create(*this);
}
void Project::removeWorkerCandidate(Actor& actor)
{
	auto iter = std::ranges::find(m_workerCandidatesAndTheirObjectives, &actor, &std::pair<Actor*, Objective*>::first);
	assert(iter != m_workerCandidatesAndTheirObjectives.end());
	m_workerCandidatesAndTheirObjectives.erase(iter);
}
// To be called by Objective::execute.
void Project::commandWorker(Actor& actor)
{
	if(!m_workers.contains(&actor))
	{
		assert(std::ranges::find(m_workerCandidatesAndTheirObjectives, &actor, &std::pair<Actor*, Objective*>::first) != m_workerCandidatesAndTheirObjectives.end());
		// Candidates do nothing but wait.
		return;
	}
	if(m_workers.at(&actor).haulSubproject != nullptr)
		// The worker has been dispatched to fetch an item.
		m_workers.at(&actor).haulSubproject->commandWorker(actor);
	else
	{
		// The worker has not been dispatched to fetch an actor or an item.
		if(deliveriesComplete())
		{
			// All items and actors have been delivered, the worker starts making.
			if(actor.isAdjacentTo(m_location))
				addToMaking(actor);
			else
				actor.m_canMove.setDestinationAdjacentTo(m_location, m_workers.at(&actor).objective.m_detour);
		}
		else if(m_toPickup.empty())
		{
			// All items and actors have been reserved with other workers dispatched to fetch them, the worker waits for them.
			if(actor.isAdjacentTo(m_location))
			{
				m_waiting.insert(&actor);
				// Any tools being carried are marked delivered.
				if(m_reservedEquipment.contains(&actor))
				{
					for(auto& pair : m_reservedEquipment.at(&actor))
					{
						++pair.first->delivered;	
						assert(pair.first->delivered <= pair.first->reserved);
					}
					if(deliveriesComplete())
						addToMaking(actor);
				}
			}
			else
				actor.m_canMove.setDestinationAdjacentTo(m_location, m_workers.at(&actor).objective.m_detour);
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
void Project::removeWorker(Actor& actor)
{
	assert(actor.m_project == this);
	if(hasCandidate(actor))
	{
		removeWorkerCandidate(actor);
		return;
	}
	assert(m_workers.contains(&actor));
	actor.m_project = nullptr;
	if(m_workers.at(&actor).haulSubproject != nullptr)
		m_workers.at(&actor).haulSubproject->removeWorker(actor);
	if(m_making.contains(&actor))
		removeFromMaking(actor);
	// If this is the only worker or the worker has a reserved tool then reset.
	if(m_workers.size() == 1 || actor.m_equipmentSet.hasAnyEquipmentWithReservations())
	{
		if(canReset())
			reset();
		else
			cancel();
		assert(!actor.m_equipmentSet.hasAnyEquipmentWithReservations());
	}
	else
		m_workers.erase(&actor);
}
void Project::addToMaking(Actor& actor)
{
	assert(m_workers.contains(&actor));
	assert(!m_making.contains(&actor));
	m_making.insert(&actor);
	scheduleEvent();
}
void Project::removeFromMaking(Actor& actor)
{
	assert(m_workers.contains(&actor));
	assert(m_making.contains(&actor));
	m_making.erase(&actor);
	scheduleEvent();
}
void Project::complete()
{
	m_canReserve.deleteAllWithoutCallback();
	for(Item* item : m_toConsume)
		item->destroy();
	for(auto& [itemType, materialType, quantity] : getByproducts())
		m_location.m_hasItems.addGeneric(*itemType, *materialType, quantity);
	for(auto& [actor, projectWorker] : m_workers)
		actor->m_project = nullptr;
	onComplete();
}
void Project::cancel()
{
	m_canReserve.deleteAllWithoutCallback();
	//for(auto& [actor, projectWorker] : m_workers)
		//actor->m_project = nullptr;
	onCancel();
}
void Project::scheduleEvent(Step start)
{
	if(start == 0)
		start = m_location.m_area->m_simulation.m_step;
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
	for(Actor* actor : workers)
	{
		m_workers.at(actor).haulSubproject = nullptr;
		commandWorker(*actor);
	}
}
void Project::haulSubprojectCancel(HaulSubproject& haulSubproject)
{
	addToPickup(haulSubproject.m_toHaul, haulSubproject.m_projectRequirementCounts, haulSubproject.m_quantity);
	for(Actor* actor : haulSubproject.m_workers)
	{
		actor->m_canPickup.putDownIfAny(*actor->m_location);
		actor->m_canFollow.unfollowIfFollowing();
		actor->m_canMove.clearPath();
		actor->m_canReserve.deleteAllWithoutCallback();
		m_workers.at(actor).haulSubproject = nullptr;
		commandWorker(*actor);
	}
	m_haulSubprojects.remove(haulSubproject);
}
void Project::setLocationDishonorCallback(std::unique_ptr<DishonorCallback> dishonorCallback)
{
	m_location.m_reservable.setDishonorCallbackFor(m_canReserve, std::move(dishonorCallback));
}
void Project::addToPickup(HasShape& hasShape, ProjectRequirementCounts& counts, uint32_t quantity)
{
	if(m_toPickup.contains(&hasShape))
		m_toPickup[&hasShape].second += quantity;
	else
		m_toPickup.try_emplace(&hasShape, &counts, quantity);
}
void Project::removeToPickup(HasShape& hasShape, uint32_t quantity)
{
	if(m_toPickup.at(&hasShape).second == quantity)
		m_toPickup.erase(&hasShape);
	else
		m_toPickup.at(&hasShape).second -= quantity;
}
void Project::reset()
{
	m_finishEvent.maybeUnschedule();
	m_tryToHaulEvent.maybeUnschedule();
	m_tryToReserveEvent.maybeUnschedule();
	m_tryToHaulThreadedTask.maybeCancel();
	m_toConsume.clear();
	m_haulRetries = 0;
	m_requiredItems.clear();
	m_requiredActors.clear();
	m_toPickup.clear();
	// I guess we are doing this in case requirements change. Probably not needed.
	m_requirementsLoaded = false;
	for(Actor* actor : getWorkersAndCandidates())
	{
		actor->m_hasObjectives.getCurrent().reset();
		actor->m_hasObjectives.cannotCompleteSubobjective();
	}
	m_canReserve.deleteAllWithoutCallback();
	m_workers.clear();
	m_workerCandidatesAndTheirObjectives.clear();
}
bool Project::canAddWorker(const Actor& actor) const
{
	assert(!m_workers.contains(&const_cast<Actor&>(actor)));
	return m_maxWorkers > m_workers.size();
}
void Project::clearReservations()
{
	m_canReserve.deleteAllWithoutCallback();
}
Project::~Project()
{
	//assert(m_workers.empty());
	m_location.m_hasProjects.remove(*this);
}
// For testing.
bool Project::hasCandidate(const Actor& actor) const
{
	return std::ranges::find(m_workerCandidatesAndTheirObjectives, &actor, &std::pair<Actor*, Objective*>::first) != m_workerCandidatesAndTheirObjectives.end();
}
std::vector<Actor*> Project::getWorkersAndCandidates()
{
	std::vector<Actor*> output;
	output.reserve(m_workers.size() + m_workerCandidatesAndTheirObjectives.size());
	for(auto& pair : m_workers)
		output.push_back(pair.first);
	for(auto& pair : m_workerCandidatesAndTheirObjectives)
		output.push_back(pair.first);
	return output;
}
std::vector<std::pair<Actor*, Objective*>> Project::getWorkersAndCandidatesWithObjectives()
{
	std::vector<std::pair<Actor*, Objective*>> output;
	output.reserve(m_workers.size() + m_workerCandidatesAndTheirObjectives.size());
	for(auto& pair : m_workers)
		output.emplace_back(pair.first, &pair.second.objective);
	for(auto& pair : m_workerCandidatesAndTheirObjectives)
		output.push_back(pair);
	return output;
}
void BlockHasProjects::add(Project& project)
{
	const Faction* faction = &project.getFaction();
	assert(!m_data.contains(faction) || !m_data.at(faction).contains(&project));
	m_data[faction].insert(&project);
}
void BlockHasProjects::remove(Project& project)
{
	const Faction* faction = &project.getFaction();
	assert(m_data.contains(faction) && m_data.at(faction).contains(&project));
	if(m_data[faction].size() == 1)
		m_data.erase(faction);
	else
		m_data[faction].erase(&project);
}
Percent BlockHasProjects::getProjectPercentComplete(Faction& faction) const
{
	if(!m_data.contains(&faction))
		return 0;
	for(Project* project : m_data.at(&faction))
		if(project->getPercentComplete())
			return project->getPercentComplete();
	return 0;
}
