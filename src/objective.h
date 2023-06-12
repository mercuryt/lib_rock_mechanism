#pragma once

template<class Actor>
struct ObjectiveType
{
	std::string name;
	virtual bool canBeAssigned(Actor& actor) = 0;
	virtual void makeFor(Actor& actor) = 0;
};
template<class Actor>
class ObjectiveTypePrioritySet
{
	std::set<const ObjectiveType<Actor>*, uint8_t> m_map;
	std::vector<const ObjectiveType<Actor>*> m_vector;
public:
	void setPriority(const ObjectiveType<Actor>& objectiveType, uint8_t priority)
	{
		if(m_map.contains(&objectiveType))
		{
			auto& found = std::ranges::find_if(m_vector, [&](const ObjectiveType* ot){ return ot == &objectiveType; });
			assert(found != m_vector.end());
			found.second = priority;
		}
		else
			m_vector.push_back(&objectiveType, priority);
		m_map[&objectiveType] = priority;
		// Sort by priority stored in m_map, high to low.
		std::ranges::sort(m_vector, [&](auto& a, auto& b){ return m_map.at(a) > m_map.at(b); });
	}
	void remove(const ObjectiveType<Actor>& objectiveType)
	{
		m_map.remove(&objectiveType);
		std::erase_if(m_vector, [&](const ObjectiveType* ot){ return ot == &objectiveType; });
	}
	void setObjectiveFor(Actor& actor)
	{
		assert(actor.m_hasObjectives.m_currentObjective == nullptr);
		assert(actor.m_hasObjectives.m_needsQueue.empty());
		assert(actor.m_hasObjectives.m_tasksQueue.empty());
		for(auto& objectiveType : m_vector)
			if(objectiveType->canBeAssigned(actor))
			{
				actor.m_hasObjectives.addTaskToStart(objectiveType.makeFor(actor));
				return;
			}
		actor.m_eventPublisher.publish(PublishedEventType::NoObjectivesAvailible);
	}
}
class ObjectiveSort
{
	bool operator()(Objective& a, Objective& b){ return a.priority < b.priority; }
};
class Objective
{
	uint32_t m_priority;
	void execute();
	Objective(uint32_t p) : m_priority(p) {}
};
template<class Actor>
class HasObjectives
{
	Actor& m_actor;
	// Two objective queues, objectives are choosen from which ever has the higher priority.
	// Biological needs like eat, drink, go to safe temperature, and sleep go here, possibly overiding the current objective in either queue.
	std::priority_queue<std::unique_ptr<Objective>, std::vector<std::unique_ptr<Objective>, decltype(ObjectiveSort)> m_needsQueue;
	// Voluntary tasks like harvest, dig, build, craft, guard, station, and kill go here. Station and kill both have higher priority then baseline needs.
	// findNewTask only adds one task at a time so there usually is only once objective in the queue. More then one task objective can be added by the user manually.
	std::deque<std::unique_ptr<Objective>> m_tasksQueue;
	Objective* m_currentObjective;

	void getNext()
	{
		m_currentObjective = nullptr;
		if(m_needsQueue.empty())
		{
			if(m_tasksQueue.empty())
				m_prioritySet.setObjectiveFor(m_actor);
			else
				setCurrentObjective(m_tasksQueue.front());
		}
		else
		{
			if(m_tasksQueue.empty())
				setCurrentObjective(m_needsQueue.top());
			else
			{
				if(m_tasksQueue.front().priority > m_needsQueue.top().priority)
					setCurrentObjective(m_tasksQueue.front());
				else
					setCurrentObjective(m_needsQueue.top());
			}
		}
	}
	void maybeUsurpsPriority(Objective& objective)
	{
		if(m_currentObjective == nullptr)
			setCurrentObjective(objective);
		else
		{
			if(m_currentObjective.priority < objective.priority)
			{
				m_currentObjective.interupt();
				setCurrentObjective(objective);
			}
		}
	}
	void setCurrentObjective(Objective& objective)
	{
		m_currentObjective = objective;
		objective.execute();
	}
public:
	ObjectiveTypePrioritySet m_prioritySet;

	HasObjectives(Actor& a) : m_actor(a), m_currentObjective(nullptr) { }
	void addNeed(std::unique_ptr<Objective>& objective)
	{
		Objective& o = objective.get();
		m_needsQueue.insert(std::move(objective));
		maybeUsurpsPriority(o);
	}
	void addTaskToEnd(std::unique_ptr<Objective>& objective)
	{
		Objective& o = objective.get();
		m_tasksQueue.push_back(std::move(objective));
		if(m_currentObjective == nullptr)
			setCurrentObjective(o);
	}
	void addTaskToStart(std::unique_ptr<Objective>& objective)
	{
		Objective& o = objective.get();
		m_tasksQueue.push_front(std::move(objective));
		maybeUserpsPriority(o);
	}
	void cancel(Objective& objective)
	{
		auto found = std::find_if(m_needsQueue.begin(), m_needsQueue.end(), [&](auto& o){ return &objective == &o; });
		if(found)
			m_needsQueue.erase(found);
		else
			std::erase_if(m_tasksQueue, [&](auto& o){ return &objective == &o; });
		if(m_currentObjective == &objective)
			getNext();
	}
	void complete()
	{
		getNext();
	}
};
