#include "project.h"
ProjectFinishEvent::ProjectFinishEvent(uint32_t delay, Project& p) : ScheduledEventWithPercent(delay), m_project(p) {}
void ProjectFinishEvent::execute() { m_project.complete(); }
~ProjectFinishEvent::ProjectFinishEvent() { m_project.m_finishEvent.clearPointer(); }

void ProjectGoToAdjacentLocationThreadedTask::readStep()
{
	auto condition = [&](Block* block)
	{
		return block.isAdjacentTo(m_project.m_location) && (!m_reserve || !block.m_reservable.isFullyReserved());
	}
	m_route = path::getForActorToPredicate(m_actor, condition);
}
void ProjectGoToAdjacentLocationThreadedTask::writeStep()
{
	if(m_route == nullptr)
		m_actor.m_hasObjectives.cannotFulfillObjective();
	else
	{
		if(m_reserve)
			m_route.back().m_reservable.reserveFor(m_actor.m_canReserve);
		m_actor.setPath(m_route);
	}
}
ProjectFindItemsThreadedTask::ProjectFindItemsThreadedTask(Project& p, Actor& a) : m_project(p), m_actor(a) { }
void ProjectFindItemsThreadedTask::readStep()
{
	auto condition = [&](Block* block) { return m_project.canGatherItemAt(m_actor, block); };
	m_route = path::getForActorToPredicate(m_actor, condition);
}
void ProjectFindItemsThreadedTask::writeStep()
{
	if(m_route.empty())
		m_actor.m_hasObjectives.cannotFulfillObjective();
	else
	{
		Item* item = m_project.gatherableItemAt(m_actor, *m_route.back());
		if(item == nullptr)
		{
			m_project.m_workers[&actor].m_findItemTask.create(*this, actor);
			return;
		}
		uint32_t quantity = std::min({m_actor.m_canPickup.canPickupQuantityOf(item), projectItemCounts.required - projectItemCounts.reserved, item->m_reservable.getUnreservedCount()});
		m_project.m_projectWorkers.at(&m_actor).item = &item;
		m_project.m_projectWorkers.at(&m_actor).reserveQuantity = quantity;
		item.reservable.reserveFor(m_actor.m_canReserve, quantity);
		m_project.m_requiredItems[item.m_itemType].reserved += quantity;
		m_actor.setPath(m_route);
	}
}
ProjectFindActorsThreadedTask::ProjectFindActorsThreadedTask(Project& p, Actor& a) : m_project(p), m_actor(a) { }
void ProjectFindActorsThreadedTask::readStep()
{
	auto condition = [&](Block* block) { return m_project.canGatherActorAt(m_actor, block); };
	m_route = path::getForActorToPredicate(m_actor, condition);
}
void ProjectFindActorsThreadedTask::writeStep()
{
	if(m_route.empty())
		m_actor.m_hasObjectives.cannotFulfillObjective();
	else
	{
		Actor* actor = m_project.gatherableActorAt(m_actor, *m_route.back());
		if(actor == nullptr || actor.m_reservable.isFullyReserved())
		{
			// Try again.
			m_project.m_workers[&actor].m_findActorsTask.create(*this, actor);
			return;
		}
		m_project.m_projectWorkers.at(&m_actor).actor = &actor;
		actor.reservable.reserveFor(m_actor.m_canReserve);
		++m_project.m_requiredActors[actor.m_actorType].reserved;
		m_actor.setPath(m_route);
	}
}
void Subproject::onComplete(){ m_project.subprojectComplete(*this); }
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
Project::Project(Block& l, size_t mw) : m_gatherComplete(false), m_location(l), maxWorker(mw)
{ 
	for(auto& [itemQuery, quantity] : getConsumed())
		m_requiredItems.push_back(itemQuery, quantity);
	for(auto& [itemQuery, quantity] : getUnconsumed())
		m_requiredItems.push_back(itemQuery, quantity);
	for(auto& [actorQuery, quantity] : getActors())
		m_requiredActors.push_back(actorQuery, quantity);
	m_location.m_reservable.reserveFor(m_canReserve);
}
void Project::addWorker(Actor& actor)
{
	assert(actor.m_project == nullptr);
	assert(!m_workers.contains(&actor));
	actor.m_project = this;
	m_workers.insert(&actor);
	commandWorker(actor);
}
// To be called by Objective::execute.
void Project::commandWorker(Actor& actor)
{
	if(m_workers[&actor].haulSubproject != nullptr)
		// The worker has been dispatched to fetch an item.
		m_workers[&actor].haulSubproject.commandWorker(actor);
	else
	{
		// The worker has not been dispatched to fetch an actor or an item.
		if(deliveriesComplete())
		{
			// All items and actors have been delivered, the worker starts making.
			if(actor.m_location.isAdjacentTo(m_location))
				addToMaking(actor);
			else
				m_workers[&actor].m_goToLocationTasks(*this, actor, true);
		}
		else if(reservationsComplete())
		{
			// All items and actors have been reserved with other workers dispatched to fetch them, the worker waits for them.
			if(actor.m_location.isAdjacentTo(m_location))
				m_waiting.insert(&actor);
			else
				m_workers[&actor].m_goToLocationTasks(*this, actor, true);
			//TODO: Schedule end of waiting and cancelation of task.
		}
		else
		{
			// Reservations are not complete, look for a subproject with no workers.
			for(auto& subproject : m_haulItemSubprojects)
				if(subproject.m_workers.size() == 0 && tryToBeginItemHaulSubproject(subproject, actor))
					return;
			// No subproject found to begin, go to project location and wait.
			if(actor.m_location.isAdjacentTo(m_location))
				m_waiting.insert(&actor);
			else
				m_workers[&actor].m_goToLocationTasks(*this, actor, true);
		}
	}
}
// To be called by Objective::interupt.
void Project::removeWorker(Actor& actor)
{
	assert(actor.m_project == this);
	assert(m_workers.contains(&actor));
	actor.m_project = nullptr;
	if(m_projectWorkers[&actor].item != nullptr)
	{
		auto& item = *m_projectWorkers[&actor].item;
		--m_requiredItems[item.m_itemType].reserved;
		if(actor.m_canPickup.m_carrying != nullptr)
		{
			assert(actor.m_canPickup.m_carrying == &item);
			actor.m_canPickup.putDown();
		}
	}
	if(m_making.contains(&actor))
		removeFromMaking(actor);
	m_projectWorkers.remove(&actor);
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
	m_making.remove(&actor);
	scheduleEvent();
}
void Project::complete()
{
	for(Item* item : m_toConsume)
		item->destroy();
	for(auto& [itemType, materialType, quantity] : getByproducts())
		m_location.m_hasItems.add(itemType, materialType, quantity);
	dismissWorkers();
	onComplete();
}
void Project::cancel()
{
	m_finishEvent.maybeCancel();
	dismissWorkers();
}
void Project::dismissWorkers()
{
	for(Actor* actor : m_workers)
		actor->taskComplete();
}
void Project::scheduleEvent()
{
	m_finishEvent.maybeCancel();
	uint32_t delay = util::scaleByPercent(getDelay(), 100u - m_digEvent.percentComplete());
	m_finishEvent.schedule(delay, *this);
}
bool Project::canGatherItemAt(Actor& actor, Block& block) const
{
	return gatherableItemAt(actor, block) != nullptr;
}
Item* Project::gatherableItemAt(Actor& actor, Block& block) const
{
	for(const Item* item : block.m_hasItems.get())
		if(!item->reservable.isFullyReserved())
			for(auto& [itemQuery, projectItemCounts] : m_requiredItems)
				if(projectItemCounts.required > projectItemCounts.reserved)
				{
					if(itemQuery(*item) && actor.m_hasItems.canPickupAny(*item))
						return item;
					if(item->itemType.internalVolume != 0)
						for(Item* i: item.m_containsItemsOrFluids.getItems())
							if(!i->m_reservable.isFullyReserved() && itemQuery(*i) && actor.m_hasItems.canPickupAny(*i))
								return i;
				}
	return nullptr;
}
bool Project::canGatherActorAt(Actor& actor, Block& block) const
{
	return gatherableActorAt(actor, block) != nullptr;
}
Actor* Project::gatherableActorAt(Actor& actor, Block& block) const
{
	for(const Actor* actor : block.m_hasActors.get())
		if(!actor->m_reservable.isFullyReserved())
			for(auto& [actorQuery, projectActorCounts] : m_requiredActors)
				if(projectActorCounts.required > projectActorCounts.reserved)
					// TODO: Check animal handeling skill.
					if(actorQuery(*actor))
						return actor;
	return nullptr;
}
void Project::subprojectComplete(Subproject& subproject)
{
	for(Actor* actor : subproject.m_workers)
		commandWorker(*actor);
	m_subprojects.remove(subproject);
}
bool Project::tryToBeginItemHaulSubproject(Subproject& subproject, Actor& actor)
{
	if(actor.m_canPickup.canPickupAny(subproject.m_item))
	{
		subproject.set
	}
}
virtual ~Project::Project
{
	dismissWorkers();
}
