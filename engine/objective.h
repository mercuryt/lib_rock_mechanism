//TODO: Objectives can use reservations from other objectives on the same actor, or make delay call reset.
#pragma once

#include "config.h"
#include "eventSchedule.hpp"
#include "reservable.h"
#include "types.h"
//#include "input.h"

#include <map>
#include <memory>
#include <queue>
#include <utility>
#include <vector>
#include <unordered_map>

class WaitEvent;
class Objective;
class SupressedNeedEvent;
class Area;
struct DeserializationMemo;

//Action.
/*
class ObjectiveTypeSetPriorityInputAction final : public InputAction
{
	ActorIndex m_actor;
	const ObjectiveType& m_objectiveType;
	uint8_t m_priority;
public:
	ObjectiveTypeSetPriorityInputAction(InputQueue& inputQueue, ActorIndex actor, const ObjectiveType& objectiveType, uint8_t priority) : InputAction(inputQueue), m_actor(actor), m_objectiveType(objectiveType), m_priority(priority) { }
	void execute();
};
class ObjectiveTypeRemoveInputAction final : public InputAction
{
	ActorIndex m_actor;
	const ObjectiveType& m_objectiveType;
public:
	ObjectiveTypeRemoveInputAction(InputQueue& inputQueue, ActorIndex actor, const ObjectiveType& objectiveType) : InputAction(inputQueue), m_actor(actor), m_objectiveType(objectiveType) { }
	void execute();
};
*/
enum class ObjectiveTypeId { Construct, Craft, Dig, Drink, Eat, Equip, Exterminate, GetToSafeTemperature, GiveItem, GivePlantsFluid, GoTo, Harvest, Haul, InstallItem, Kill, LeaveArea, Medical, Rest, Sleep, Station, SowSeeds, StockPile, Unequip, Uniform, Wait, Wander, WoodCutting };
NLOHMANN_JSON_SERIALIZE_ENUM(ObjectiveTypeId, {
	{ObjectiveTypeId::Construct, "Construct"}, 
	{ObjectiveTypeId::Craft, "Craft"},
	{ObjectiveTypeId::Dig,"Dig"}, 
	{ObjectiveTypeId::Drink, "Drink"}, 
	{ObjectiveTypeId::Eat, "Eat"}, 
	{ObjectiveTypeId::GetToSafeTemperature, "GetToSafeTemperature"}, 
	{ObjectiveTypeId::GiveItem, "GiveItem"}, 
	{ObjectiveTypeId::GivePlantsFluid, "GivePlantsFluid"}, 
	{ObjectiveTypeId::GoTo, "GoTo"}, 
	{ObjectiveTypeId::Harvest, "Harvest"}, 
	{ObjectiveTypeId::Haul, "Haul"}, 
	{ObjectiveTypeId::Kill, "Kill"}, 
	{ObjectiveTypeId::LeaveArea, "LeaveArea"}, 
	{ObjectiveTypeId::Medical, "Medical"}, 
	{ObjectiveTypeId::Rest, "Rest"}, 
	{ObjectiveTypeId::Sleep, "Sleep"}, 
	{ObjectiveTypeId::Station, "Station"}, 
	{ObjectiveTypeId::SowSeeds, "SowSeeds"}, 
	{ObjectiveTypeId::StockPile, "StockPile"}, 
	{ObjectiveTypeId::Wait, "Wait"}, 
	{ObjectiveTypeId::Uniform, "Uniform"}, 
	{ObjectiveTypeId::Unequip, "Unequip"}, 
	{ObjectiveTypeId::Wander, "Wander"},
	{ObjectiveTypeId::Wander, "WoodCutting"},
});
struct ObjectiveType
{
	[[nodiscard]] virtual bool canBeAssigned(Area& area, ActorIndex actor) const = 0;
	[[nodiscard]] virtual std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const = 0;
	[[nodiscard]] virtual ObjectiveTypeId getObjectiveTypeId() const = 0;
	[[nodiscard]] virtual Json toJson() const;
	ObjectiveType() = default;
	ObjectiveType(const ObjectiveType&) = delete;
	ObjectiveType(ObjectiveType&&) = delete;
	virtual ~ObjectiveType() = default;
	// Infastructure
	inline static std::map<std::string, std::unique_ptr<ObjectiveType>> objectiveTypes;
	inline static std::map<const ObjectiveType*, std::string> objectiveTypeNames;
	static void load();
	[[nodiscard]] bool operator==(const ObjectiveType& other) const { return &other == this; }
};
void to_json(Json& data, const ObjectiveType* const& objectiveType);
void from_json(const Json& data, const ObjectiveType*& objectiveType);
class Objective
{
public:
	ActorIndex m_actor;
	// Controlls usurping of current objective between the tasks currently at the top of the queue vs highest priority need.
	uint32_t m_priority;
	// If detour is true then pathing will account for the positions of other actors.
	bool m_detour = false;
	// Reentrant state machine method.
	virtual void execute(Area& area) = 0;
	// Clean up references.
	// Called when the player replaces the current objective queue with a new task.
	// Objective will be destroyed after this call.
	virtual void cancel(Area& area) = 0;
	// Clean up threaded tasks and events.
	// Called when an objective usurps the current objective.
	virtual void delay(Area& area) = 0;
	// Return to inital state and try again.
	// Called on cannotCompleteSubobjective
	virtual void reset(Area& area) = 0;
	// Returns true if the condition is resolved or false otherwise.
	// If false is returned canNotCompleteObjective is called.
	virtual bool onCanNotRepath(Area&) { return false; }
	// To be used by objectives with projects which cannot be auto destroyed.
	// Records projects which have failed to reserve requirements so as not to retry them with this objective instance.
	// Will be 'flushed' when this instance is destroyed and then another objective of this type is created later, after objectivePrioritySet delay ends.
	virtual void onProjectCannotReserve(Area&) { }
	void detour(Area& area) { m_detour = true; execute(area); }
	[[nodiscard]] virtual std::string name() const = 0;
	[[nodiscard]] virtual ObjectiveTypeId getObjectiveTypeId() const = 0;
	// Needs are biological imperatives which overide tasks. Eat, sleep, etc.
	[[nodiscard]] virtual bool isNeed() const { return false; }
	// When an objective is interrputed by a higher priority objective should it be kept in the task queue for later or discarded?
	// Should be true only for objectives like Wander or Wait which are not meant to resume after interrupt because they are idle tasks.
	[[nodiscard]] virtual bool canResume() const { return true; }
	Objective(ActorIndex a, uint32_t p);
	// Explicit delete of copy and move constructors to ensure pointer stability.
	Objective(const Objective&) = delete;
	Objective(Objective&&) = delete;
	Objective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] virtual Json toJson() const;
	bool operator==(const Objective& other) const { return &other == this; }
	virtual ~Objective() = default;
};
inline void to_json(Json& data, const Objective* const& objective){ data = reinterpret_cast<uintptr_t>(objective); }
struct ObjectivePriority
{
	const ObjectiveType* objectiveType;
	uint8_t priority;
	Step doNotAssignAgainUntil;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ObjectivePriority, objectiveType, priority, doNotAssignAgainUntil);
class ObjectiveTypePrioritySet final
{
	ActorIndex m_actor;
	std::vector<ObjectivePriority> m_data;
	ObjectivePriority& getById(ObjectiveTypeId objectiveTypeId);
	const ObjectivePriority& getById(ObjectiveTypeId objectiveTypeId) const;
public:
	ObjectiveTypePrioritySet() = default; // default constructor needed to allow vector resize.
	ObjectiveTypePrioritySet(ActorIndex a) : m_actor(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void setPriority(Area& area, const ObjectiveType& objectiveType, uint8_t priority);
	void remove(const ObjectiveType& objectiveType);
	void setObjectiveFor(Area& area, ActorIndex actor);
	void setDelay(Area& area, ObjectiveTypeId objectiveTypeId);
	[[nodiscard]] uint8_t getPriorityFor(ObjectiveTypeId objectiveTypeId) const;
	// For testing.
	[[nodiscard, maybe_unused]] bool isOnDelay(Area& area, ObjectiveTypeId objectiveTypeId) const;
	[[nodiscard, maybe_unused]] Step getDelayEndFor(ObjectiveTypeId objectiveTypeId) const;
};
class SupressedNeed final
{
	Area& m_area;
	ActorIndex m_actor;
	std::unique_ptr<Objective> m_objective;
	HasScheduledEvent<SupressedNeedEvent> m_event;
public:
	SupressedNeed(Area& area, std::unique_ptr<Objective> o, ActorIndex a);
	SupressedNeed(const Json& data, DeserializationMemo& deserializationMemo, ActorIndex a);
	Json toJson() const;
	void callback();
	friend class SupressedNeedEvent;
	bool operator==(const SupressedNeed& supressedNeed){ return &supressedNeed == this; }
};
class SupressedNeedEvent final : public ScheduledEvent
{
	SupressedNeed& m_supressedNeed;
public:
	SupressedNeedEvent(Area& area, SupressedNeed& sn, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
// Some objectives without projects may need to reserve items, they can use this callback to call cannot complete.
class CannotCompleteObjectiveDishonorCallback final : public DishonorCallback
{
	Area& m_area;
	ActorIndex m_actor;
public:
	CannotCompleteObjectiveDishonorCallback(Area& area, ActorIndex a) : m_area(area), m_actor(a) { }
	CannotCompleteObjectiveDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(uint32_t oldCount, uint32_t newCount);
};
class HasObjectives final
{
	// Two objective queues, objectives are choosen from which ever has the higher priority.
	// Biological needs like eat, drink, go to safe temperature, and sleep go into needs queue, possibly overiding the current objective in either queue.
	std::list<std::unique_ptr<Objective>> m_needsQueue;
	// Prevent duplicate Objectives in needs queue.
	std::unordered_set<ObjectiveTypeId> m_idsOfObjectivesInNeedsQueue;
	// Voluntary tasks like harvest, dig, build, craft, guard, station, and kill go into task queue. Station and kill both have higher priority then baseline needs like eat but lower then needs like flee.
	// findNewTask only adds one task at a time so there usually is only once objective in the queue. More then one task objective can be added by the user manually.
	std::list<std::unique_ptr<Objective>> m_tasksQueue;
	std::unordered_map<ObjectiveTypeId, SupressedNeed> m_supressedNeeds;
	Objective* m_currentObjective = nullptr;
	ActorIndex m_actor = ACTOR_INDEX_MAX;

	void maybeUsurpsPriority(Area& area, Objective& objective);
	void setCurrentObjective(Area& area, Objective& objective);
	// Erase objective.
	void destroy(Area& area, Objective& objective);
public:
	ObjectiveTypePrioritySet m_prioritySet;

	HasObjectives() = default; // default constructor needed to allow vector resize.
	HasObjectives(ActorIndex a);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	// Assign next objective from either task queue or needs, depending on priority.
	void getNext(Area& area);
	// Add to need set, to be sorted into position depending on priority. May usurp.
	void addNeed(Area& area, std::unique_ptr<Objective> objective);
	// Add task to end of queue.
	void addTaskToEnd(Area& area, std::unique_ptr<Objective> objective);
	// Add task to start of queue. May usurp current objective.
	void addTaskToStart(Area& area, std::unique_ptr<Objective> objective);
	// Clear task queue and then add single task.
	void replaceTasks(Area& area, std::unique_ptr<Objective> objective);
	void cancel(Objective& objective);
	void objectiveComplete(Objective& objective);
	void cannotFulfillObjective(Objective& objective);
	void cannotFulfillNeed(Objective& objective);
	// Sub unit of objective is complete, get the next sub unit if there is one or call objectiveComplete.
	void subobjectiveComplete();
	// Sub unit of objective cannot be completed, get an alternative or call cannotFulfill.
	void cannotCompleteSubobjective();
	// Repath taking into account the locations of other actors.
	// To be used when the path is temporarily blocked.
	void detour();
	void restart(Area& area) { m_currentObjective->reset(area); m_currentObjective->execute(area); }
	[[nodiscard]] Objective& getCurrent();
	[[nodiscard]] bool hasCurrent() const { return m_currentObjective != nullptr; }
	[[nodiscard]] bool hasSupressedNeed(const ObjectiveTypeId objectiveTypeId) const { return m_supressedNeeds.contains(objectiveTypeId); }
	[[nodiscard]] bool queuesAreEmpty() const;
	friend class ObjectiveTypePrioritySet;
	friend class SupressedNeed;
};
