//TODO: Objectives can use reservations from other objectives on the same actor, or make delay call reset.
#pragma once

#include "config.h"
#include "deserializationMemo.h"
#include "eventSchedule.h"
#include "eventSchedule.hpp"
#include "input.h"

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

//Action.

class ObjectiveTypeSetPriorityInputAction final : public InputAction
{
	Actor& m_actor;
	const ObjectiveType& m_objectiveType;
	uint8_t m_priority;
public:
	ObjectiveTypeSetPriorityInputAction(InputQueue& inputQueue, Actor& actor, const ObjectiveType& objectiveType, uint8_t priority) : InputAction(inputQueue), m_actor(actor), m_objectiveType(objectiveType), m_priority(priority) { }
	void execute();
};
class ObjectiveTypeRemoveInputAction final : public InputAction
{
	Actor& m_actor;
	const ObjectiveType& m_objectiveType;
public:
	ObjectiveTypeRemoveInputAction(InputQueue& inputQueue, Actor& actor, const ObjectiveType& objectiveType) : InputAction(inputQueue), m_actor(actor), m_objectiveType(objectiveType) { }
	void execute();
};
enum class ObjectiveTypeId { Construct, Craft, Dig, Drink, Eat, GetToSafeTemperature, GivePlantsFluid, GoTo, Harvest, Haul, Kill, Medical, Rest, Sleep, Station, SowSeeds, StockPile, Wait, Wander };
NLOHMANN_JSON_SERIALIZE_ENUM(ObjectiveTypeId, {
	{ObjectiveTypeId::Construct, "Construct"}, 
	{ObjectiveTypeId::Craft, "Craft"},
	{ObjectiveTypeId::Dig,"Dig"}, 
	{ObjectiveTypeId::Drink, "Drink"}, 
	{ObjectiveTypeId::Eat, "Eat"}, 
	{ObjectiveTypeId::GetToSafeTemperature, "GetToSafeTemperature"}, 
	{ObjectiveTypeId::GivePlantsFluid, "GivePlantsFluid"}, 
	{ObjectiveTypeId::GoTo, "GoTo"}, 
	{ObjectiveTypeId::Harvest, "Harvest"}, 
	{ObjectiveTypeId::Haul, "Haul"}, 
	{ObjectiveTypeId::Kill, "Kill"}, 
	{ObjectiveTypeId::Medical, "Medical"}, 
	{ObjectiveTypeId::Rest, "Rest"}, 
	{ObjectiveTypeId::Sleep, "Sleep"}, 
	{ObjectiveTypeId::Station, "Station"}, 
	{ObjectiveTypeId::SowSeeds, "SowSeeds"}, 
	{ObjectiveTypeId::StockPile, "StockPile"}, 
	{ObjectiveTypeId::Wait, "Wait"}, 
	{ObjectiveTypeId::Wander, "Wander"},
});
struct ObjectiveType
{
	[[nodiscard]] virtual bool canBeAssigned(Actor& actor) const = 0;
	[[nodiscard]] virtual std::unique_ptr<Objective> makeFor(Actor& actor) const = 0;
	[[nodiscard]] virtual ObjectiveTypeId getObjectiveTypeId() const = 0;
	[[nodiscard]] virtual Json& toJson();
	ObjectiveType() = default;
	ObjectiveType(const ObjectiveType&) = delete;
	ObjectiveType(ObjectiveType&&) = delete;
	virtual Json toJson() const;
	// Infastructure
	static std::map<std::string, std::unique_ptr<ObjectiveType>> objectiveTypes;
	static std::map<const ObjectiveType*, std::string> objectiveTypeNames;
	inline static void load();
	[[nodiscard]] bool operator==(const ObjectiveType& other) const { return &other == this; }
};
inline void to_json(Json& data, const ObjectiveType* const& objectiveType){ data = ObjectiveType::objectiveTypeNames[objectiveType]; }
inline void from_json(const Json& data, const ObjectiveType*& objectiveType){ objectiveType = ObjectiveType::objectiveTypes[data.get<std::string>()].get(); }
class Objective
{
public:
	Actor& m_actor;
	uint32_t m_priority;
	bool m_detour = false;
	virtual void execute() = 0;
	virtual void cancel() = 0;
	virtual void delay() = 0;
	virtual void reset() = 0;
	virtual bool onNoPath() { return false; }
	void detour() { m_detour = true; execute(); }
	[[nodiscard]] virtual std::string name() const = 0;
	[[nodiscard]] virtual ObjectiveTypeId getObjectiveTypeId() const = 0;
	[[nodiscard]] virtual bool isNeed() const { return false; }
	Objective(Actor& a, uint32_t p);
	Objective(const Objective&) = delete;
	Objective(Objective&&) = delete;
	Objective(const Json& data, DeserializationMemo& deserializationMemo);
	virtual Json toJson() const;
	bool operator==(const Objective& other) const { return &other == this; }
	virtual ~Objective() = default;
};
struct ObjectivePriority
{
	const ObjectiveType* objectiveType;
	uint8_t priority;
	Step doNotAssignAgainUntil;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ObjectivePriority, objectiveType, priority, doNotAssignAgainUntil);
class ObjectiveTypePrioritySet final
{
	Actor& m_actor;
	std::vector<ObjectivePriority> m_data;
public:
	ObjectiveTypePrioritySet(Actor& a) : m_actor(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
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
	SupressedNeed(const Json& data, DeserializationMemo& deserializationMemo, Actor& a);
	Json toJson() const;
	void callback();
	friend class SupressedNeedEvent;
	bool operator==(const SupressedNeed& supressedNeed){ return &supressedNeed == this; }
};
class SupressedNeedEvent final : public ScheduledEventWithPercent
{
	SupressedNeed& m_supressedNeed;
public:
	SupressedNeedEvent(SupressedNeed& sn, const Step start = 0);
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
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void getNext();
	void addNeed(std::unique_ptr<Objective> objective);
	void addTaskToEnd(std::unique_ptr<Objective> objective);
	void addTaskToStart(std::unique_ptr<Objective> objective);
	void replaceTasks(std::unique_ptr<Objective> objective);
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
