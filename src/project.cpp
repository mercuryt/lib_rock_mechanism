#include "project.h"
#include "path.h"
#include "block.h"
#include "area.h"
#include <algorithm>
void ProjectFinishEvent::execute() { m_project.complete(); }
void ProjectFinishEvent::clearReferences() { m_project.m_finishEvent.clearPointer(); }

void ProjectTryToHaulEvent::execute() { m_project.m_tryToHaulThreadedTask.create(m_project); }
void ProjectTryToHaulEvent::clearReferences() { m_project.m_finishEvent.clearPointer(); }

void ProjectTryToMakeHaulSubprojectThreadedTask::readStep()
{
	for(auto& [actor, projectWorker] : m_project.m_workers)
	{
		if(projectWorker.haulSubproject != nullptr)
			continue;
		// Make copy becasue we can't capture from outside the enclosing scope.
		Actor* pointerToActor = actor;
		std::function<bool(const Block&)> condition = [&, pointerToActor](const Block& block)
		{
			for(Item* item : block.m_hasItems.getAll())
				for(auto& [itemQuery, projectItemCounts] : m_project.m_requiredItems)
					if(itemQuery(*item) && projectItemCounts.required > projectItemCounts.reserved)
					{
						m_haulProjectParamaters = HaulSubproject::tryToSetHaulStrategy(m_project, *item, *pointerToActor, projectItemCounts);
						if(m_haulProjectParamaters.strategy != HaulStrategy::None)
							return true;
					}
			return false;
		};
		m_findsPath.pathToPredicate(*actor, condition);
		// Only make at most one per step.
		if(m_haulProjectParamaters.strategy != HaulStrategy::None)
			return;
	}
}
void ProjectTryToMakeHaulSubprojectThreadedTask::writeStep()
{
	if(m_haulProjectParamaters.strategy == HaulStrategy::None)
		// Nothing haulable found, wait a bit before we try again.
		m_project.m_tryToHaulEvent.schedule(Config::stepsFrequencyToLookForHaulSubprojects, m_project);
	else if(!m_haulProjectParamaters.validate())
	{
		// Something that was avalible during read step is now reserved. Ensure that there is a new threaded task to try again.
		if(!m_project.m_tryToHaulThreadedTask.exists())
			m_project.m_tryToHaulThreadedTask.create(m_project);
	}
	else
	{
		// All guards passed, create subproject and dispatch workers.
		assert(!m_haulProjectParamaters.workers.empty());
		m_project.m_haulSubprojects.emplace_back(m_project, m_haulProjectParamaters);
	}
}
void ProjectTryToMakeHaulSubprojectThreadedTask::clearReferences() { m_project.m_tryToHaulThreadedTask.clearPointer(); }
// Derived classes are expected to provide getDelay, getConsumedItems, getUnconsumedItems, getByproducts, and onComplete.
Project::Project(const Faction* f, Block& l, size_t mw) : m_finishEvent(l.m_area->m_simulation.m_eventSchedule), m_tryToHaulEvent(l.m_area->m_simulation.m_eventSchedule), m_tryToHaulThreadedTask(l.m_area->m_simulation.m_threadedTaskEngine), m_gatherComplete(false), m_canReserve(f), m_maxWorkers(mw), m_loadedRequirements(false) , m_location(l) { }
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
void Project::recordRequiredActorsAndItemsAndReserveLocation()
{ 
	for(auto& [itemQuery, quantity] : getConsumed())
		m_requiredItems.emplace_back(itemQuery, quantity);
	for(auto& [itemQuery, quantity] : getUnconsumed())
		m_requiredItems.emplace_back(itemQuery, quantity);
	for(auto& [actorQuery, quantity] : getActors())
		m_requiredActors.emplace_back(actorQuery, quantity);
	m_location.m_reservable.reserveFor(m_canReserve, 1);
}
void Project::addWorker(Actor& actor, Objective& objective)
{
	assert(actor.m_project == nullptr);
	assert(!m_workers.contains(&actor));
	assert(actor.isSentient());
	if(!m_loadedRequirements)
	{
		recordRequiredActorsAndItemsAndReserveLocation();
		m_loadedRequirements = true;
	}
	actor.m_project = this;
	// Initalize a ProjectWorker for this worker.
	m_workers.emplace(&actor, objective);
	commandWorker(actor);
}
// To be called by Objective::execute.
void Project::commandWorker(Actor& actor)
{
	if(m_workers.at(&actor).haulSubproject != nullptr)
		// The worker has been dispatched to fetch an item.
		m_workers.at(&actor).haulSubproject->commandWorker(actor);
	else
	{
		// The worker has not been dispatched to fetch an actor or an item.
		if(deliveriesComplete())
		{
			// All items and actors have been delivered, the worker starts making.
			if(actor.m_location->isAdjacentTo(m_location))
				addToMaking(actor);
			else
				actor.m_canMove.setDestinationAdjacentTo(m_location);
		}
		else if(reservationsComplete())
		{
			// All items and actors have been reserved with other workers dispatched to fetch them, the worker waits for them.
			if(actor.m_location->isAdjacentTo(m_location))
				m_waiting.insert(&actor);
			else
				actor.m_canMove.setDestinationAdjacentTo(m_location);
			//TODO: Schedule end of waiting and cancelation of task.
		}
		else
		{
			// Reservations are not complete, try to create a haul subproject unless one exists.
			if(!m_tryToHaulThreadedTask.exists())
				m_tryToHaulThreadedTask.create(*this);
		}
	}
}
// To be called by Objective::cancel and Objective::delay.
void Project::removeWorker(Actor& actor)
{
	assert(actor.m_project == this);
	assert(m_workers.contains(&actor));
	actor.m_project = nullptr;
	if(m_workers.at(&actor).haulSubproject != nullptr)
		m_workers.at(&actor).haulSubproject->cancel();
	if(m_making.contains(&actor))
		removeFromMaking(actor);
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
	for(Item* item : m_toConsume)
		item->destroy();
	for(auto& [itemType, materialType, quantity] : getByproducts())
		m_location.m_hasItems.add(*itemType, *materialType, quantity);
	for(auto& [actor, projectWorker] : m_workers)
		actor->m_hasObjectives.objectiveComplete(projectWorker.objective);
	m_workers.clear();
	onComplete();
}
void Project::cancel()
{
	m_finishEvent.maybeUnschedule();
	for(auto& pair : m_workers)
		pair.first->m_hasObjectives.cannotFulfillObjective(pair.second.objective);
	m_workers.clear();
}
void Project::scheduleEvent()
{
	m_finishEvent.maybeUnschedule();
	uint32_t delay = util::scaleByPercent(getDelay(), 100u - m_finishEvent.percentComplete());
	m_finishEvent.schedule(delay, *this);
}
void Project::haulSubprojectComplete(HaulSubproject& haulSubproject)
{
	for(Actor* actor : haulSubproject.m_workers)
		commandWorker(*actor);
	m_haulSubprojects.remove(haulSubproject);
}
Project::~Project()
{
	assert(m_workers.empty());
}
ProjectFinishEvent::ProjectFinishEvent(const Step delay, Project& p) : ScheduledEventWithPercent(p.m_location.m_area->m_simulation, delay), m_project(p) {}
ProjectTryToHaulEvent::ProjectTryToHaulEvent(const Step delay, Project& p) : ScheduledEventWithPercent(p.m_location.m_area->m_simulation, delay), m_project(p) { }
ProjectTryToMakeHaulSubprojectThreadedTask::ProjectTryToMakeHaulSubprojectThreadedTask(Project& p) : ThreadedTask(p.m_location.m_area->m_simulation.m_threadedTaskEngine), m_project(p) { }
