//TODO: Objectives can use reservations from other objectives on the same actor, or make delay call reset.
#pragma once

#include "reference.h"
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
	const ObjectiveTypeId& m_objectiveType;
	Priority m_priority;
public:
	ObjectiveTypeSetPriorityInputAction(InputQueue& inputQueue, const ActorIndex& actor, const ObjectiveTypeId& objectiveType, const Priority& priority) : InputAction(inputQueue), m_actor(actor), m_objectiveType(objectiveType), m_priority(priority) { }
	void execute();
};
class ObjectiveTypeRemoveInputAction final : public InputAction
{
	ActorIndex m_actor;
	const ObjectiveTypeId& m_objectiveType;
public:
	ObjectiveTypeRemoveInputAction(InputQueue& inputQueue, const ActorIndex& actor, const ObjectiveTypeId& objectiveType) : InputAction(inputQueue), m_actor(actor), m_objectiveType(objectiveType) { }
	void execute();
};
*/
// Used by needs subsystem within hasObjectives.
enum class NeedType
{
	none,
	eat,
	drink,
	sleep,
	temperature,
	test,
};
class ObjectiveType
{
public:
	ObjectiveType() = default;
	static void load();
	static ObjectiveTypeId getIdByName(std::wstring name);
	static const ObjectiveType& getById(const ObjectiveTypeId& id);
	static const ObjectiveType& getByName(std::wstring name);
	ObjectiveTypeId getId() const;
	[[nodiscard]] virtual bool canBeAssigned(Area& area, const ActorIndex& actor) const = 0;
	[[nodiscard]] virtual std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const = 0;
	[[nodiscard]] virtual std::wstring name() const = 0;
	ObjectiveType(const ObjectiveTypeId&) = delete;
	ObjectiveType(ObjectiveType&&) = delete;
	virtual ~ObjectiveType() = default;
};
inline StrongVector<std::unique_ptr<ObjectiveType>, const ObjectiveTypeId&> objectiveTypeData;
class Objective
{
public:
	// Controlls usurping of current objective between the tasks currently at the top of the queue vs highest priority need.
	Priority m_priority;
	// If detour is true then pathing will account for the positions of other actors.
	bool m_detour = false;
	// Reentrant state machine method.
	virtual void execute(Area& area, const ActorIndex& actor) = 0;
	// Clean up references.
	// Called when the player replaces the current objective queue with a new task.
	// Objective will be destroyed after this call.
	virtual void cancel(Area& area, const ActorIndex& actor) = 0;
	// Clean up threaded tasks and events.
	// Called when an objective usurps the current objective.
	virtual void delay(Area& area, const ActorIndex& actor) = 0;
	// Return to inital state and try again.
	// Called on cannotCompleteSubobjective
	virtual void reset(Area& area, const ActorIndex& actor) = 0;
	// Returns true if the condition is resolved or false otherwise.
	// If false is returned canNotCompleteObjective is called.
	virtual bool onCanNotPath(Area&, const ActorIndex&) { return false; }
	// To be used by objectives with projects which cannot be auto destroyed.
	// Records projects which have failed to reserve requirements so as not to retry them with this objective instance.
	// Will be 'flushed' when this instance is destroyed and then another objective of this type is created later, after objectivePrioritySet delay ends.
	virtual void onProjectCannotReserve(Area&, const ActorIndex&) { }
	void detour(Area& area, const ActorIndex& actor) { m_detour = true; execute(area, actor); }
	[[nodiscard]] virtual std::wstring name() const = 0;
	// TODO: This is silly.
	[[nodiscard]] ObjectiveTypeId getTypeId() const { return ObjectiveType::getIdByName(name());}
	// Needs are biological imperatives which overide tasks. Eat, sleep, etc.
	[[nodiscard]] virtual bool isNeed() const { return false; }
	// When an objective is interrputed by a higher priority objective should it be kept in the task queue for later or discarded?
	// Should be true only for objectives like Wander or Wait which are not meant to resume after interrupt because they are idle tasks.
	[[nodiscard]] virtual bool canResume() const { return true; }
	[[nodiscard]] virtual NeedType getNeedType() const { std::unreachable(); return NeedType::none; }
	Objective(const Priority& priority);
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
	ObjectiveTypeId objectiveType;
	Priority priority;
	Step doNotAssignAgainUntil;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ObjectivePriority, objectiveType, priority, doNotAssignAgainUntil);
class ObjectiveTypePrioritySet final
{
	std::vector<ObjectivePriority> m_data;
	ObjectivePriority& getById(const ObjectiveTypeId& objectiveTypeId);
	const ObjectivePriority& getById(const ObjectiveTypeId& objectiveTypeId) const;
public:
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void setPriority(Area& area, const ActorIndex& actor, const ObjectiveTypeId& objectiveType, const Priority& priority);
	void remove(const ObjectiveTypeId& objectiveType);
	void setObjectiveFor(Area& area, const ActorIndex& actor);
	void setDelay(Area& area, const ObjectiveTypeId& objectiveTypeId);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Priority getPriorityFor(const ObjectiveTypeId& objectiveTypeId) const;
	// For testing.
	[[nodiscard]] bool isOnDelay(Area& area, const ObjectiveTypeId& objectiveTypeId) const;
	[[nodiscard]] Step getDelayEndFor(const ObjectiveTypeId& objectiveTypeId) const;
};
class SupressedNeed final
{
	std::unique_ptr<Objective> m_objective;
	HasScheduledEvent<SupressedNeedEvent> m_event;
	ActorReference m_actor;
public:
	SupressedNeed(Area& area, std::unique_ptr<Objective> o, const ActorReference& ref);
	SupressedNeed(Area& area, const Json& data, DeserializationMemo& deserializationMemo, const ActorReference& ref);
	void callback(Area& area);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Step getDelayRemaining() const { return m_event.remainingSteps(); }
	friend class SupressedNeedEvent;
};
class SupressedNeedEvent final : public ScheduledEvent
{
	SupressedNeed& m_supressedNeed;
public:
	SupressedNeedEvent(Area& area, SupressedNeed& sn, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
// Some objectives without projects may need to reserve items, they can use this callback to call cannot complete.
class CannotCompleteObjectiveDishonorCallback final : public DishonorCallback
{
	Area& m_area;
	ActorReference m_actor;
public:
	CannotCompleteObjectiveDishonorCallback(Area& area, const ActorReference& a) : m_area(area), m_actor(a) { }
	CannotCompleteObjectiveDishonorCallback(Area& area, const Json& data);
	void execute(const Quantity& oldCount, const Quantity& newCount);
	[[nodiscard]] Json toJson() const;
};
class HasObjectives final
{
	// Two objective queues, objectives are choosen from which ever has the higher priority.
	// Biological needs like eat, drink, go to safe temperature, and sleep go into needs queue, possibly overiding the current objective in either queue.
	std::list<std::unique_ptr<Objective>> m_needsQueue;
	// Prevent duplicate Objectives in needs queue.
	SmallSet<NeedType> m_typesOfNeedsInQueue;
	// Voluntary tasks like harvest, dig, build, craft, guard, station, and kill go into task queue. Station and kill both have higher priority then baseline needs like eat but lower then needs like flee.
	// findNewTask only adds one task at a time so there usually is only once objective in the queue. More then one task objective can be added by the user manually.
	std::list<std::unique_ptr<Objective>> m_tasksQueue;
	SmallMapStable<NeedType, SupressedNeed> m_supressedNeeds;
	Objective* m_currentObjective = nullptr;
	ActorIndex m_actor;

	void maybeUsurpsPriority(Area& area, Objective& objective);
	void setCurrentObjective(Area& area, Objective& objective);
	// Erase objective.
	void destroy(Area& area, Objective& objective);
public:
	ObjectiveTypePrioritySet m_prioritySet;

	HasObjectives(const ActorIndex& a) : m_actor(a) { }
	void updateActorIndex(const ActorIndex& actor) { m_actor = actor; }
	void load(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const ActorIndex& actor);
	// Assign next objective from either task queue or needs, depending on priority.
	void getNext(Area& area);
	//TODO: should the unique ptrs be passed by reference here?
	// Add to need set, to be sorted into position depending on priority. May usurp.
	void addNeed(Area& area, std::unique_ptr<Objective> objective);
	// Add task to end of queue.
	void addTaskToEnd(Area& area, std::unique_ptr<Objective> objective);
	// Add task to start of queue. May usurp current objective.
	void addTaskToStart(Area& area, std::unique_ptr<Objective> objective);
	// Clear task queue and then add single task.
	void replaceTasks(Area& area, std::unique_ptr<Objective> objective);
	void cancel(Area& area, Objective& objective);
	void objectiveComplete(Area& area, Objective& objective);
	void cannotFulfillObjective(Area& area, Objective& objective);
	void cannotFulfillNeed(Area& area, Objective& objective);
	// Sub unit of objective is complete, get the next sub unit if there is one or call objectiveComplete.
	void subobjectiveComplete(Area& area);
	// Sub unit of objective cannot be completed, get an alternative or call cannotFulfill.
	void cannotCompleteSubobjective(Area& area);
	// Repath taking into account the locations of other actors.
	// To be used when the path is temporarily blocked.
	void detour(Area& area);
	void restart(Area& area, const ActorIndex& actor) { m_currentObjective->reset(area, actor); m_currentObjective->execute(area, actor); }
	[[nodiscard]] Objective& getCurrent();
	[[nodiscard]] bool hasCurrent() const { return m_currentObjective != nullptr; }
	[[nodiscard]] bool hasSupressedNeed(const NeedType& needType) const { return m_supressedNeeds.contains(needType); }
	[[nodiscard]] bool queuesAreEmpty() const;
	[[nodiscard]] bool hasTask(const ObjectiveTypeId& objectiveTypeId) const;
	[[nodiscard]] bool hasNeed(const NeedType& objectiveTypeId) const;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Step getNeedDelayRemaining(NeedType needType) const;
	friend class ObjectiveTypePrioritySet;
	friend class SupressedNeed;
};
