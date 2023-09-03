#pragma once

#include "eventSchedule.h"
#include "eventSchedule.hpp"

#include <map>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

class Actor;
class WaitEvent;

class Objective
{
public:
	uint32_t m_priority;
	virtual void execute() = 0;
	virtual void cancel() = 0;
	virtual std::string name() = 0;
	Objective(uint32_t p);
	Objective(const Objective&) = delete;
	Objective(Objective&&) = delete;
	bool operator==(const Objective& other) { return &other == this; }
	virtual ~Objective() = default;
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
struct ObjectiveSortByPriority
{
	bool operator()(Objective* const& a, Objective* const& b) const
	{
		return a->m_priority > b->m_priority;
	}
};
class HasObjectives final
{
	Actor& m_actor;
	// Two objective queues, objectives are choosen from which ever has the higher priority.
	// Biological needs like eat, drink, go to safe temperature, and sleep go into needs queue, possibly overiding the current objective in either queue.
	std::map<Objective*, std::unique_ptr<Objective>, ObjectiveSortByPriority> m_needsQueue;
	// Voluntary tasks like harvest, dig, build, craft, guard, station, and kill go into task queue. Station and kill both have higher priority then baseline needs like eat but lower then needs like flee.
	// findNewTask only adds one task at a time so there usually is only once objective in the queue. More then one task objective can be added by the user manually.
	std::list<std::unique_ptr<Objective>> m_tasksQueue;
	Objective* m_currentObjective;
	HasScheduledEvent<WaitEvent> m_waitEvent;

	void maybeUsurpsPriority(Objective& objective);
	void setCurrentObjective(Objective& objective);
public:
	ObjectiveTypePrioritySet m_prioritySet;

	HasObjectives(Actor& a);
	void getNext();
	void addNeed(std::unique_ptr<Objective> objective);
	void addTaskToEnd(std::unique_ptr<Objective> objective);
	void addTaskToStart(std::unique_ptr<Objective> objective);
	void cancel(Objective& objective);
	void objectiveComplete(Objective& objective);
	void taskComplete();
	void cannotFulfillObjective(Objective& objective);
	void cannotCompleteTask();
	void cannotFulfillNeed(Objective& objective);
	void wait(const Step delay);
	Objective& getCurrent();
	bool hasCurrent() { return m_currentObjective != nullptr; }
	friend class ObjectiveTypePrioritySet;
	friend class WaitEvent;
};

class WaitEvent final : public ScheduledEventWithPercent
{
	Actor& m_actor;
public:
	WaitEvent(const Step delay, Actor& a);
	void execute();
	void clearReferences();
};
