//TODO: Objectives can use reservations from other objectives on the same actor, or make delay call reset.
#pragma once

#include "eventSchedule.h"
#include "eventSchedule.hpp"

#include <map>
#include <memory>
#include <queue>
#include <utility>
#include <vector>
#include <unordered_map>

class Actor;
class WaitEvent;
class Objective;
class SupressedNeedEvent;

enum class ObjectiveTypeId { Construct, Craft, Dig, Drink, Eat, GetToSafeTemperature, GivePlantsFluid, GoTo, Harvest, Haul, Kill, Rest, Sleep, Station, SowSeeds, StockPile, Wait, Wander };
struct ObjectiveType
{
	[[nodiscard]] virtual bool canBeAssigned(Actor& actor) const = 0;
	[[nodiscard]] virtual std::unique_ptr<Objective> makeFor(Actor& actor) const = 0;
	[[nodiscard]] virtual ObjectiveTypeId getObjectiveTypeId() const = 0;
	ObjectiveType() = default;
	ObjectiveType(const ObjectiveType&) = delete;
	ObjectiveType(ObjectiveType&&) = delete;
	[[nodiscard]] bool operator==(const ObjectiveType& other) const { return &other == this; }
};
class Objective
{
public:
	uint32_t m_priority;
	bool m_detour = false;
	virtual void execute() = 0;
	virtual void cancel() = 0;
	virtual void delay() = 0;
	virtual void reset() = 0;
	void detour() { m_detour = true; execute(); }
	[[nodiscard]] virtual std::string name() const = 0;
	[[nodiscard]] virtual ObjectiveTypeId getObjectiveTypeId() const = 0;
	Objective(uint32_t p);
	Objective(const Objective&) = delete;
	Objective(Objective&&) = delete;
	bool operator==(const Objective& other) const { return &other == this; }
	virtual ~Objective() = default;
};
struct ObjectivePriority
{
	const ObjectiveType* objectiveType;
	uint8_t priority;
	Step doNotAssignAginUntil;
};
class ObjectiveTypePrioritySet final
{
	Actor& m_actor;
	std::vector<ObjectivePriority> m_data;
public:
	ObjectiveTypePrioritySet(Actor& a) : m_actor(a) { }
	void setPriority(const ObjectiveType& objectiveType, uint8_t priority);
	void remove(const ObjectiveType& objectiveType);
	void setObjectiveFor(Actor& actor);
	void setDelay(ObjectiveTypeId objectiveTypeId);
	// For testing.
	[[nodiscard, maybe_unused]] bool isOnDelay(ObjectiveTypeId);
};
class SupressedNeed final
{
	Actor& m_actor;
	std::unique_ptr<Objective> m_objective;
	HasScheduledEvent<SupressedNeedEvent> m_event;
public:
	SupressedNeed(std::unique_ptr<Objective> o, Actor& a);
	void callback();
	friend class SupressedNeedEvent;
	bool operator==(const SupressedNeed& supressedNeed){ return &supressedNeed == this; }
};
class SupressedNeedEvent final : public ScheduledEventWithPercent
{
	SupressedNeed& m_supressedNeed;
public:
	SupressedNeedEvent(SupressedNeed& sn);
	void execute();
	void clearReferences();
};
class HasObjectives final
{
	Actor& m_actor;
	// Two objective queues, objectives are choosen from which ever has the higher priority.
	// Biological needs like eat, drink, go to safe temperature, and sleep go into needs queue, possibly overiding the current objective in either queue.
	std::list<std::unique_ptr<Objective>> m_needsQueue;
	// Prevent duplicate Objectives in needs queue.
	std::unordered_set<ObjectiveTypeId> m_idsOfObjectivesInNeedsQueue;
	// Voluntary tasks like harvest, dig, build, craft, guard, station, and kill go into task queue. Station and kill both have higher priority then baseline needs like eat but lower then needs like flee.
	// findNewTask only adds one task at a time so there usually is only once objective in the queue. More then one task objective can be added by the user manually.
	std::list<std::unique_ptr<Objective>> m_tasksQueue;
	Objective* m_currentObjective;
	std::unordered_map<ObjectiveTypeId, SupressedNeed> m_supressedNeeds;

	void maybeUsurpsPriority(Objective& objective);
	void setCurrentObjective(Objective& objective);
public:
	ObjectiveTypePrioritySet m_prioritySet;

	HasObjectives(Actor& a);
	void getNext();
	void addNeed(std::unique_ptr<Objective> objective);
	void addTaskToEnd(std::unique_ptr<Objective> objective);
	void addTaskToStart(std::unique_ptr<Objective> objective);
	void destroy(Objective& objective);
	void cancel(Objective& objective);
	void objectiveComplete(Objective& objective);
	void taskComplete();
	void cannotFulfillObjective(Objective& objective);
	void cannotCompleteTask();
	void cannotFulfillNeed(Objective& objective);
	void detour();
	void restart() { m_currentObjective->reset(); m_currentObjective->execute(); }
	Objective& getCurrent();
	bool hasCurrent() { return m_currentObjective != nullptr; }
	bool hasSupressedNeed(const ObjectiveTypeId objectiveTypeId) const { return m_supressedNeeds.contains(objectiveTypeId); }
	friend class ObjectiveTypePrioritySet;
	friend class SupressedNeed;
};
