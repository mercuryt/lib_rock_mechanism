#pragma once

#include <map>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

class Actor;

class Objective
{
public:
	uint32_t m_priority;
	virtual void execute() = 0;
	virtual void cancel() = 0;
	Objective(uint32_t p);
	Objective(const Objective&) = delete;
	Objective(Objective&&) = delete;
};
struct ObjectiveType
{
	virtual bool canBeAssigned(Actor& actor) const = 0;
	virtual std::unique_ptr<Objective> makeFor(Actor& actor) const = 0;
};
class ObjectiveTypePrioritySet
{
	std::vector<std::pair<const ObjectiveType*, uint8_t>> m_data;
public:
	void setPriority(const ObjectiveType& objectiveType, uint8_t priority);
	void remove(const ObjectiveType& objectiveType);
	void setObjectiveFor(Actor& actor);
};
class HasObjectives final
{
	Actor& m_actor;
	// Two objective queues, objectives are choosen from which ever has the higher priority.
	// Biological needs like eat, drink, go to safe temperature, and sleep go here, possibly overiding the current objective in either queue.
	std::vector<std::pair<uint32_t, std::unique_ptr<Objective>>> m_needsQueue;
	// Voluntary tasks like harvest, dig, build, craft, guard, station, and kill go here. Station and kill both have higher priority then baseline needs but lower then needs like flee.
	// findNewTask only adds one task at a time so there usually is only once objective in the queue. More then one task objective can be added by the user manually.
	std::deque<std::unique_ptr<Objective>> m_tasksQueue;
	Objective* m_currentObjective;

	void getNext();
	void maybeUsurpsPriority(Objective& objective);
	void setCurrentObjective(Objective& objective);
public:
	ObjectiveTypePrioritySet m_prioritySet;

	HasObjectives(Actor& a);
	void addNeed(std::unique_ptr<Objective>&& objective);
	void addTaskToEnd(std::unique_ptr<Objective>&& objective);
	void addTaskToStart(std::unique_ptr<Objective>&& objective);
	void cancel(Objective& objective);
	void objectiveComplete(Objective& objective);
	void taskComplete();
	void cannotFulfillObjective(Objective& objective);
	void cannotCompleteTask();
	friend class ObjectiveTypePrioritySet;
};
