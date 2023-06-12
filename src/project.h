#pragma once

#include <unordered_set>
#include <unordered_map>

template<class Project>
class ProjectFinishEvent : public ScheduledEventWithPercent
{
	Project& m_project;
	ProjectFinishEvent(uint32_t step, Project& p) : ScheduledEventWithPercent(step), m_project(p) {}
	void execute() { m_project.complete(); }
	~ProjectFinishEvent() { m_project.m_finishEvent.clearPointer(); }
};
template<class Project, class Actor>
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
template<class Project, class Block, class Actor>
class ProjectFindItemsThreadedTask : ThreadedTask
{
	Project& m_project;
	Actor& m_actor;
	std::vector<Block*> m_route;
public:
	ProjectFindItemsThreadedTask(Project& p, Actor& a) : m_project(p), m_actor(a) { }
	void readStep()
	{
		auto condition = [&](Block* block) { return m_project.canGatherItemsAt(block); };
		m_route = path::getForActorToPredicate(m_actor, condition);
	}
	void writeStep()
	{
		if(m_route.empty())
			m_actor.m_hasObjectives.cannotFulfillObjective();
		else
		{
			for(Item& item : m_route.back()->m_items)
				if(m_project.m_requiredItems.contains(item.itemType) && !item.m_reservable.isFullyReserved())
				{
					uint32_t quantity = m_project.m_workers[&m_actor].reserveQuantity = m_actor.m_canPickup.canPickupQuantityOf(item);
					m_project.m_projectWorkers.at(&m_actor).item = &item;
					item.reservable.reserveFor(m_actor.m_canReserve, quantity);
					m_project.m_requiredItems[item.m_itemType].reserved += quantity;
					m_actor.setPath(m_route);
					return;
				}
			m_project.m_workers[&actor].m_findItemsTask.create(*this, actor);
		}
	}
}
template<class Item>
class ProjectWorker
{
	HasThreadedTask<ProjectGoToAdjacentLocationThreadedTask> goToTask;
	HasThreadedTask<ProjectFindItemsThreadedTask> findItemsTask;
	Item* item;
	uint32_t itemQuantity;
	ProjectWorker() : item(nullptr) { }
}
class ProjectItemCounts
{
	uint8_t required;
	uint8_t reserved;
	uint8_t delivered;
	ProjectItemCounts(uint8_t r) : required(r), reserved(0), delivered(0) { }
}
// Derived classes are expected to provide getDelay, m_consumedItems, m_unconsumedItems, and onComplete.
template<class Block, class Actor, class Item>
class Project
{
	HasScheduledEventPausable<ProjectFinishEvent> m_finishEvent;
	bool m_gatherComplete;
	Block& m_location;
	CanReserve m_canReserveItems;
	std::unordered_map<const ItemType*, ProjectItemCounts> m_requiredItems;
	std::unordered_set<Item*> m_toConsume;
	size_t maxWorkers;
	Project(Block& l, size_t mw) : m_gatherComplete(false), m_location(l), maxWorker(mw)
       	{ 
		for(auto& [itemType, quantity] : m_consumedItems)
		{
			assert(!m_requiredItems.contains(itemType));
			m_requiredItems[itemType](quantity);
		}
		for(auto& [itemType, quantity] : m_unconsumedItems)
		{
			assert(!m_requiredItems.contains(itemType));
			m_requiredItems[itemType](quantity);
		}
	}
	bool reservationsComplete() const
	{
		for(auto& pair : m_requiredItems)
			if(pair.second.required > pair.second.reserved)
				return false;
		return true;
	}
	bool deliveriesComplete() const
	{
		for(auto& pair : m_requiredItems)
			if(pair.second.required > pair.second.delivered)
				return false;
		return true;
	}
protected:
	std::unordered_map<Actor*, ProjectWorker> m_workers;
	std::unordered_set<Actor*> m_waiting;
	std::unordered_set<Actor*> m_making;
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
		if(m_workers[&actor].item != nullptr)
		{
			if(actor.m_canPickup.empty())
			{
				actor.m_canPickup.pickUp(item, m_workers[&actor].itemQuantity);
				m_workers[&actor].m_goToLocationTasks(*this, actor, true);
				return;
			}
			else
			{
				assert(actor.m_location->isAdjacentTo(m_location));
				auto& item = actor.m_canPickup.m_carrying;
				actor.m_canPickup.putDown();
				item.m_reservable.unreserve(actor.m_canReserve);
				item.m_reservable.reserve(m_canReserve);
				++m_requiredItems.at(&item.itemType).delivered;
				if(getConsumedItems().contains(item.itemType))
					m_toConsume.insert(&item);
				if(deliveriesComplete())
				{
					assert(m_making.empty());
					m_making.swap(m_waiting);
					scheduleEvent();
				}
			}
		}
		if(deliveriesComplete())
		{
			if(actor.m_location.isAdjacentTo(m_location))
				addToMaking(actor);
			else
				m_workers[&actor].m_goToLocationTasks(*this, actor, true);
		}
		else if(reservationsComplete)
		{
			if(actor.m_location.isAdjacentTo(m_location))
				m_waiting.insert(&actor);
			else
				m_workers[&actor].m_goToLocationTasks(*this, actor, true);
		}
		else
			m_workers[&actor].m_findItemsTask.create(*this, actor);
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
	virtual ~Project
	{
		dismissWorkers();
	}
	virtual uint32_t getDelay() const;
	virtual void onComplete();
	virtual std::unordered_map<const ItemType*, uint8_t> getConsumed();
	virtual std::unordered_map<const ItemType*, uint8_t> getUnconsumed();
};
