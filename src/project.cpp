#include "project.h"
#include "path.h"
#include "block.h"
#include <algorithm>
ProjectFinishEvent::ProjectFinishEvent(uint32_t delay, Project& p) : ScheduledEventWithPercent(delay), m_project(p) {}
void ProjectFinishEvent::execute() { m_project.complete(); }
ProjectFinishEvent::~ProjectFinishEvent() { m_project.m_finishEvent.clearPointer(); }

void ProjectTryToHaulEvent::execute() { m_project.m_tryToHaulThreadedTask.create(m_project); }
ProjectTryToHaulEvent::~ProjectTryToHaulEvent() { m_project.m_finishEvent.clearPointer(); }

void ProjectTryToMakeHaulSubprojectThreadedTask::readStep()
{
	for(Actor* actor : m_project.m_waiting)
	{
		auto condition = [&](Block& block)
		{
			for(Item* item : block.m_hasItems.getAll())
				for(auto& [itemQuery, projectItemCounts] : m_project.m_requiredItems)
					if(projectItemCounts.required >= projectItemCounts.reserved)
					{
						m_haulProjectParamaters = HaulSubproject::tryToSetHaulStrategy(m_project, *item, *actor);
						if(m_haulProjectParamaters.strategy != HaulStrategy::None)
							return true;
					}
			return false;
		};
		path::getForActorToPredicateReturnEndOnly(*actor, condition);
		// Only make at most one per step.
		if(m_haulProjectParamaters.strategy != HaulStrategy::None)
			return;
	}
}
void ProjectTryToMakeHaulSubprojectThreadedTask::writeStep()
{
	if(m_haulProjectParamaters.strategy == HaulStrategy::None)
		// Nothing haulable found, wait awhile before we try again.
		m_project.m_tryToHaulEvent.schedule(Config::stepsFrequencyToLookForHaulSubprojects, m_project);
	else if(!m_haulProjectParamaters.validate())
	{
		// Something that was avalible during read step is now reserved. Try again, unless we were going to anyway.
		if(!m_project.m_tryToHaulThreadedTask.exists())
			m_project.m_tryToHaulThreadedTask.create(m_project);
	}
	else
		// All guards passed, create subproject and dispatch workers.
		m_project.m_haulSubprojects.emplace_back(m_project, m_haulProjectParamaters);
}
// Derived classes are expected to provide getDelay, getConsumedItems, getUnconsumedItems, getByproducts, and onComplete.
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
void Project::addWorker(Actor& actor)
{
	assert(actor.m_project == nullptr);
	assert(!m_workers.contains(&actor));
	actor.m_project = this;
	// Initalize a ProjectWorker for this worker.
	m_workers[&actor];
	commandWorker(actor);
}
// To be called by Objective::execute.
void Project::commandWorker(Actor& actor)
{
	if(m_workers[&actor].haulSubproject != nullptr)
		// The worker has been dispatched to fetch an item.
		m_workers[&actor].haulSubproject->commandWorker(actor);
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
// To be called by Objective::cancel.
void Project::removeWorker(Actor& actor)
{
	assert(actor.m_project == this);
	assert(m_workers.contains(&actor));
	actor.m_project = nullptr;
	if(m_workers[&actor].haulSubproject != nullptr)
		m_workers[&actor].haulSubproject->cancel();
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
	dismissWorkers();
	onComplete();
}
void Project::cancel()
{
	m_finishEvent.maybeUnschedule();
	dismissWorkers();
}
void Project::dismissWorkers()
{
	for(auto& pair : m_workers)
		pair.first->m_hasObjectives.taskComplete();
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
	dismissWorkers();
}
