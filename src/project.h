#pragma once

#include <vector>
#include <pair>
#include <unordered_set>
#include <unordered_map>

class ProjectFinishEvent : public ScheduledEventWithPercent
{
	Project& m_project;
	ProjectFinishEvent(uint32_t step, Project& p) : ScheduledEventWithPercent(step), m_project(p) {}
	void execute() { m_project.complete(); }
	~ProjectFinishEvent() { m_project.m_finishEvent.clearPointer(); }
};
class ProjectGoToAdjacentLocationThreadedTask : ThreadedTask
{
	Project& m_project;
	Actor& m_actor;
	bool m_reserve; // Set reserve to true for going to a position to start making, set to false for going to drop off materials.
	std::vector<Block*> m_route;
public:
	ProjectGoToAdjacentLocationThreadedTask(Project& p, Actor& a, bool r) : m_project(p), m_actor(a), m_reserve(r) { }
	void readStep()
	{
		auto condition = [&](Block* block)
		{
			return block.isAdjacentTo(m_project.m_location) && (!m_reserve || !block.m_reservable.isFullyReserved());
		}
		m_route = path::getForActorToPredicate(m_actor, condition);
	}
	void writeStep()
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
}
class ProjectFindItemsThreadedTask : ThreadedTask
{
	Project& m_project;
	Actor& m_actor;
	std::vector<Block*> m_route;
public:
	ProjectFindItemsThreadedTask(Project& p, Actor& a) : m_project(p), m_actor(a) { }
	void readStep()
	{
		auto condition = [&](Block* block) { return m_project.canGatherItemAt(m_actor, block); };
		m_route = path::getForActorToPredicate(m_actor, condition);
	}
	void writeStep()
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
}
class ProjectFindActorsThreadedTask : ThreadedTask
{
	Project& m_project;
	Actor& m_actor;
	std::vector<Block*> m_route;
public:
	ProjectFindActorsThreadedTask(Project& p, Actor& a) : m_project(p), m_actor(a) { }
	void readStep()
	{
		auto condition = [&](Block* block) { return m_project.canGatherActorAt(m_actor, block); };
		m_route = path::getForActorToPredicate(m_actor, condition);
	}
	void writeStep()
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
};
struct ProjectWorker
{
	HasThreadedTask<ProjectGoToAdjacentLocationThreadedTask> goToTask;
	HaulSubproject* haulSubproject;
	ProjectWorker() : haulSubproject(nullptr) { }
};
struct ProjectItemCounts
{
	uint8_t required;
	uint8_t reserved;
	uint8_t delivered;
	ProjectItemCounts(uint8_t r) : required(r), reserved(0), delivered(0) { }
};
class Subproject
{
	Project& m_project;
public:
	std::unordered_set<Actor*> m_workers;
	Subproject(Project& p) : m_project(p) { }
	void commandWorker(Actor& actor) = 0;
	void onComplete(){ m_project.subprojectComplete(*this); }
};
// Derived classes are expected to provide getDelay, getConsumedItems, getUnconsumedItems, getByproducts, and onComplete.
class Project
{
	HasScheduledEventPausable<ProjectFinishEvent> m_finishEvent;
	bool m_gatherComplete;
	Block& m_location;
	CanReserve m_canReserve;
	std::vector<std::pair<ItemQuery, ProjectItemCounts>> m_requiredItems;
	std::vector<std::pair<ActorQuery, ProjectItemCounts>> m_requiredActors;
	std::unordered_set<Item*> m_toConsume;
	size_t maxWorkers;
	bool reservationsComplete() const
	{
		for(auto& pair : m_requiredItems)
			if(pair.second.required > pair.second.reserved)
				return false;
		for(auto& pair : m_requiredActors)
			if(pair.second.required > pair.second.reserved)
				return false;
		return true;
	}
	bool deliveriesComplete() const
	{
		for(auto& pair : m_requiredItems)
			if(pair.second.required > pair.second.delivered)
				return false;
		for(auto& pair : m_requiredActors)
			if(pair.second.required > pair.second.delivered)
				return false;
		return true;
	}
protected:
	std::unordered_map<Actor*, ProjectWorker> m_workers;
	std::unordered_set<Actor*> m_waiting;
	std::unordered_set<Actor*> m_making;
	std::list<HaulSubproject<Item>> m_haulItemSubprojects;
	std::list<HaulSubproject<Actor>> m_haulActorSubprojects;
	Project(Block& l, size_t mw) : m_gatherComplete(false), m_location(l), maxWorker(mw)
       	{ 
		for(auto& [itemQuery, quantity] : getConsumed())
			m_requiredItems.push_back(itemQuery, quantity);
		for(auto& [itemQuery, quantity] : getUnconsumed())
			m_requiredItems.push_back(itemQuery, quantity);
		for(auto& [actorQuery, quantity] : getActors())
			m_requiredActors.push_back(actorQuery, quantity);
		m_location.m_reservable.reserveFor(m_canReserve);
	}
public:
	void addWorker(Actor& actor)
	{
		assert(actor.m_project == nullptr);
		assert(!m_workers.contains(&actor));
		actor.m_project = this;
		m_workers.insert(&actor);
		commandWorker(actor);
	}
	// To be called by Objective::execute.
	void commandWorker(Actor& actor)
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
	void removeWorker(Actor& actor)
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
	void addToMaking(Actor& actor)
	{
		assert(m_workers.contains(&actor));
		assert(!m_making.contains(&actor));
		m_making.insert(&actor);
		scheduleEvent();
	}
	void removeFromMaking(Actor& actor)
	{
		assert(m_workers.contains(&actor));
		assert(m_making.contains(&actor));
		m_making.remove(&actor);
		scheduleEvent();
	}
	void complete()
	{
		for(Item* item : m_toConsume)
			item->destroy();
		for(auto& [itemType, materialType, quantity] : getByproducts())
			m_location.m_hasItems.add(itemType, materialType, quantity);
		dismissWorkers();
		onComplete();
	}
	void cancel()
	{
		m_finishEvent.maybeCancel();
		dismissWorkers();
	}
	void dismissWorkers()
	{
		for(Actor* actor : m_workers)
			actor->taskComplete();
	}
	void scheduleEvent()
	{
		m_finishEvent.maybeCancel();
		uint32_t delay = util::scaleByPercent(getDelay(), 100u - m_digEvent.percentComplete());
		m_finishEvent.schedule(::s_step + delay, *this);
	}
	bool canGatherItemAt(Actor& actor, Block& block) const
	{
		return gatherableItemAt(actor, block) != nullptr;
	}
	Item* gatherableItemAt(Actor& actor, Block& block) const
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
	bool canGatherActorAt(Actor& actor, Block& block) const
	{
		return gatherableActorAt(actor, block) != nullptr;
	}
	Actor* gatherableActorAt(Actor& actor, Block& block) const
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
	void subprojectComplete(Subproject& subproject)
	{
		for(Actor* actor : subproject.m_workers)
			commandWorker(*actor);
		m_subprojects.remove(subproject);
	}
	bool tryToBeginItemHaulSubproject(Subproject& subproject, Actor& actor)
	{
		if(actor.m_canPickup.canPickupAny(subproject.m_item))
		{
			subproject.set
		}
	}
	virtual ~Project
	{
		dismissWorkers();
	}
	virtual uint32_t getDelay() const = 0;
	virtual void onComplete() = 0;
	virtual std::vector<std::pair<ItemQuery, uint32_t> getConsumed() const = 0;
	virtual std::vector<std::pair<ItemQuery, uint32_t> getUnconsumed() const = 0;
	virtual std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const = 0;
	virtual std::vector<std::pair<ActorQuery, uint32_t> getActors() const = 0;
};
