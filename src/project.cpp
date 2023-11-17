#include "project.h"
#include "block.h"
#include "area.h"
#include <algorithm>
ProjectFinishEvent::ProjectFinishEvent(const Step delay, Project& p) : ScheduledEventWithPercent(p.m_location.m_area->m_simulation, delay), m_project(p) {}
void ProjectFinishEvent::execute() { m_project.complete(); }
void ProjectFinishEvent::clearReferences() { m_project.m_finishEvent.clearPointer(); }

ProjectTryToHaulEvent::ProjectTryToHaulEvent(const Step delay, Project& p) : ScheduledEventWithPercent(p.m_location.m_area->m_simulation, delay), m_project(p) { }
void ProjectTryToHaulEvent::execute() { m_project.m_tryToHaulThreadedTask.create(m_project); }
void ProjectTryToHaulEvent::clearReferences() { m_project.m_tryToHaulEvent.clearPointer(); }

ProjectTryToReserveEvent::ProjectTryToReserveEvent(const Step delay, Project& p) : ScheduledEventWithPercent(p.m_location.m_area->m_simulation, delay), m_project(p) { }
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
		// Make copy becasue we can't capture from outside the enclosing scope.
		Actor* pointerToActor = actor;
		std::function<bool(const Block&)> condition = [&, pointerToActor](const Block& block) { return blockContainsDesiredItem(block, *pointerToActor); };
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
			m_project.setDelayOn();
			m_project.m_haulRetries = 0;
		}
		else
			// Nothing haulable found, wait a bit and try again.
			m_project.m_tryToHaulEvent.schedule(Config::stepsFrequencyToLookForHaulSubprojects, m_project);
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
		if(m_project.m_toPickup.at(m_haulProjectParamaters.toHaul).second == m_haulProjectParamaters.quantity)
			m_project.m_toPickup.erase(m_haulProjectParamaters.toHaul);
		else
			m_project.m_toPickup.at(m_haulProjectParamaters.toHaul).second -= m_haulProjectParamaters.quantity;
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
	// location.  Accumulate resources they can path to untill reservations
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
		if(!m_project.reservationsComplete())
		{
			// Verfy the worker can path to the required materials. Cumulative for all candidates in this step but reset if not satisfied.
			std::function<bool(const Block&)> predicate = [&](const Block& block)
			{
				for(Item* item : block.m_hasItems.getAll())
					for(auto& [itemQuery, projectItemCounts] : m_project.m_requiredItems)
					{
						if(projectItemCounts.required == projectItemCounts.reserved)
							continue;
						if(item->m_reservable.isFullyReserved(&m_project.m_faction))
							continue;	
						if(!itemQuery(*item))
							continue;
						uint32_t desiredQuantity = projectItemCounts.required - projectItemCounts.reserved;
						uint32_t quantity = std::min(desiredQuantity, item->m_reservable.getUnreservedCount(m_project.m_faction));
						assert(quantity != 0);
						projectItemCounts.reserved += quantity;
						assert(projectItemCounts.reserved <= projectItemCounts.required);
						if(*item->m_location == m_project.m_location || item->isAdjacentTo(m_project.m_location))
						{
							// Item is already at or next to project location.
							projectItemCounts.delivered += quantity;
							assert(!m_alreadyAtSite.contains(item));
							m_alreadyAtSite[item] = quantity;
						}
						else
						{
							if(m_project.m_toPickup.contains(item))
								m_project.m_toPickup[item].second += quantity;
							else
								m_project.m_toPickup.try_emplace(item, &projectItemCounts, quantity);
						}
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
	}
}
void ProjectTryToAddWorkersThreadedTask::writeStep()
{
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
			hasShape->m_reservable.reserveFor(m_project.m_canReserve, pair.second);
		for(auto& [hasShape, quantity] : m_alreadyAtSite)
			hasShape->m_reservable.reserveFor(m_project.m_canReserve, quantity);
		// Add all actors.
		std::vector<Actor*> notAdded;
		for(auto& [actor, objective] : m_project.m_workerCandidatesAndTheirObjectives)
		{
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
			actor->m_hasObjectives.cannotCompleteTask();
		}
	}
	else
	{
		// Could not find required items / actors, activate delay and release candidates.
		m_project.setDelayOn();
		for(auto& pair : m_project.m_workerCandidatesAndTheirObjectives)
		{
			pair.second->reset();
			pair.first->m_project = nullptr;
			pair.first->m_hasObjectives.cannotCompleteTask();
		}
		m_project.m_workerCandidatesAndTheirObjectives.clear();
		m_project.m_tryToReserveEvent.schedule(Config::stepsToDelayBeforeTryingAgainToReserveItemsAndActorsForAProject, m_project);
	}
	for(Actor* actor : m_cannotPathToJobSite)
		actor->m_hasObjectives.cannotCompleteTask();
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
	return true;
}
void ProjectTryToAddWorkersThreadedTask::resetProjectCounts()
{
	for(auto& [itemQuery, projectItemCounts] : m_project.m_requiredItems)
	{
		projectItemCounts.reserved = 0;
		projectItemCounts.delivered = 0;
	}
	m_project.m_toPickup.clear();
}
// Derived classes are expected to provide getDuration, getConsumedItems, getUnconsumedItems, getByproducts, onDelay, offDelay, and onComplete.
Project::Project(const Faction* f, Block& l, size_t mw) : m_finishEvent(l.m_area->m_simulation.m_eventSchedule), m_tryToHaulEvent(l.m_area->m_simulation.m_eventSchedule), m_tryToReserveEvent(l.m_area->m_simulation.m_eventSchedule), m_tryToHaulThreadedTask(l.m_area->m_simulation.m_threadedTaskEngine), m_tryToAddWorkersThreadedTask(l.m_area->m_simulation.m_threadedTaskEngine), m_canReserve(f), m_maxWorkers(mw), m_delay(false), m_haulRetries(0), m_requirementsLoaded(false), m_location(l), m_faction(*f)
{
	m_location.m_reservable.reserveFor(m_canReserve);
}
bool Project::reservationsComplete() const
{
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
		ProjectItemCounts projectItemCounts(quantity, true);
		m_requiredItems.emplace_back(itemQuery, std::move(projectItemCounts));
	}
	for(auto& [itemQuery, quantity] : getUnconsumed())
	{
		ProjectItemCounts projectItemCounts(quantity, false);
		m_requiredItems.emplace_back(itemQuery, std::move(projectItemCounts));
	}
	for(auto& [actorQuery, quantity] : getActors())
	{
		ProjectItemCounts projectItemCounts(quantity, false);
		m_requiredActors.emplace_back(actorQuery, std::move(projectItemCounts));
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
				m_waiting.insert(&actor);
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
// To be called by Objective::cancel and Objective::delay.
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
		m_workers.at(&actor).haulSubproject->cancel();
	if(m_making.contains(&actor))
		removeFromMaking(actor);
	m_workers.erase(&actor);
	onCancel();
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
	m_canReserve.clearAll();
	for(Item* item : m_toConsume)
		item->destroy();
	for(auto& [itemType, materialType, quantity] : getByproducts())
		m_location.m_hasItems.add(*itemType, *materialType, quantity);
	for(auto& [actor, projectWorker] : m_workers)
		actor->m_project = nullptr;
	onComplete();
}
void Project::cancel()
{
	m_finishEvent.maybeUnschedule();
	for(auto& pair : m_workers)
		pair.first->m_hasObjectives.cannotFulfillObjective(pair.second.objective);
	m_workers.clear();
	m_canReserve.clearAll();
}
void Project::scheduleEvent()
{
	m_finishEvent.maybeUnschedule();
	uint32_t delay = util::scaleByPercent(getDuration(), 100u - m_finishEvent.percentComplete());
	m_finishEvent.schedule(delay, *this);
}
void Project::haulSubprojectComplete(HaulSubproject& haulSubproject)
{
	auto workers = std::move(haulSubproject.m_workers);
	m_haulSubprojects.remove(haulSubproject);
	for(Actor* actor : workers)
		commandWorker(*actor);
}
bool Project::canAddWorker(const Actor& actor) const
{
	assert(!m_workers.contains(&const_cast<Actor&>(actor)));
	return m_maxWorkers > m_workers.size();
}
Project::~Project()
{
	//assert(m_workers.empty());
}
// For testing.
bool Project::hasCandidate(const Actor& actor) const
{
	return std::ranges::find(m_workerCandidatesAndTheirObjectives, &actor, &std::pair<Actor*, Objective*>::first) != m_workerCandidatesAndTheirObjectives.end();
}
