#pragma once

#include "actorOrItemIndex.h"
#include "types.h"
#include "reservable.h"
#include "items/itemQuery.h"
#include "eventSchedule.hpp"
#include "threadedTask.hpp"
#include "onDestroy.h"
#include "haul.h"
#include "reference.h"
#include "actors/actorQuery.h"
#include "vectorContainers.h"

#include <vector>
#include <utility>

class ProjectFinishEvent;
class ProjectTryToHaulEvent;
class ProjectTryToReserveEvent;
class ProjectTryToMakeHaulSubprojectThreadedTask;
class ProjectTryToAddWorkersThreadedTask;
class Objective;
class Area;
class ActorOrItemIndex;
struct DeserializationMemo;

struct ProjectWorker final
{
	HaulSubproject* haulSubproject;
	Objective* objective;
	ProjectWorker(Objective& o) : haulSubproject(nullptr), objective(&o) { }
	ProjectWorker(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
};
struct ProjectRequirementCounts final
{
	Quantity required;
	Quantity delivered = Quantity::create(0);
	Quantity reserved = Quantity::create(0);
	bool consumed;
	ProjectRequirementCounts(const Quantity& r, bool c) : required(r), consumed(c) { }
	//TODO: Why isn't intrusive deserializer working here?
	ProjectRequirementCounts(const Json& data) :
		required(data["required"].get<Quantity>()), delivered(data["delivered"].get<Quantity>()), 
		reserved(data["reserved"].get<Quantity>()), consumed(data["consumed"].get<bool>()) { }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(ProjectRequirementCounts, required, delivered, reserved, consumed);
};
struct ProjectRequiredShapeDishonoredCallback final : public DishonorCallback
{
	Project& m_project;
	ProjectRequiredShapeDishonoredCallback(Project& p) : m_project(p) { }
	ProjectRequiredShapeDishonoredCallback(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(const Quantity&, const Quantity&);
	[[nodiscard]] Json toJson() const;
};
// Derived classes are expected to provide getDelay, getConsumedItems, getUnconsumedItems, getByproducts, and onComplete.
class Project
{
	// Tracks the progress of the 'actual' work of the project, after the hauling is done.
	HasScheduledEvent<ProjectFinishEvent> m_finishEvent;
	// Schedule a delay before trying again to generate a haul subproject.
	// Increments m_haulRetries.
	HasScheduledEvent<ProjectTryToHaulEvent> m_tryToHaulEvent;
	// Schedule a delay before calling setDelayOff.
	HasScheduledEvent<ProjectTryToReserveEvent> m_tryToReserveEvent;
	// Tries to generate a haul subproject.
	HasThreadedTask<ProjectTryToMakeHaulSubprojectThreadedTask> m_tryToHaulThreadedTask;
	// Tries to reserve all requirements if not reserved yet.
	// If able to reserve requirements then workers from m_workerCandidatesAndTheirObjectives are added if they can path to the project location.
	HasThreadedTask<ProjectTryToAddWorkersThreadedTask> m_tryToAddWorkersThreadedTask;
	CanReserve m_canReserve;
	// Queries for items needed for the project, counts of required, reserved and delivered.
	std::vector<std::pair<ItemQuery, ProjectRequirementCounts>> m_requiredItems;
	//TODO: required actors are not suported in several places.
	std::vector<std::pair<ActorQuery, ProjectRequirementCounts>> m_requiredActors;
	// Required items which will be destroyed at the end of the project.
	SmallMap<ItemReference, Quantity> m_toConsume;
	// Required items which are equiped by workers (tools).
	SmallMap<ActorReference, SmallMap<ProjectRequirementCounts*, ItemReference>> m_reservedEquipment;
	// Targets for haul subprojects awaiting dispatch.
	SmallMap<ActorOrItemReference, std::pair<ProjectRequirementCounts*, Quantity>> m_toPickup;
	// To be called by addWorkerThreadedTask, after validating the worker has access to the project location.
	void addWorker(const ActorIndex& actor, Objective& objective);
	// Load requirements from child class.
	void recordRequiredActorsAndItems();
	// After create.
	void setup();
protected:
	// Workers who have passed the candidate screening and ProjectWorker, which holds a reference to the worker's objective and a pointer to it's haul subproject.
	SmallMap<ActorReference, ProjectWorker> m_workers;
	// Required shapes which don't need to be hauled because they are already at or adjacent to m_location.
	// To be used by StockpileProject::onComplete.
	SmallMap<ActorOrItemReference, Quantity> m_alreadyAtSite;
	// Workers present at the job site, waiting for haulers to deliver required materiels.
	SmallSet<ActorReference> m_waiting;
	// Workers currently doing the 'actual' work.
	SmallSet<ActorReference> m_making;
	// Worker candidates are evaluated in a read step task.
	// For the first we need to reserve all items and actors, as wel as verify access.
	// For subsequent candidates we just need to verify that they can path to the construction site.
	std::vector<std::pair<ActorReference, Objective*>> m_workerCandidatesAndTheirObjectives;
	// Single hauling operations are managed by haul subprojects.
	// They have one or more workers plus optional haul tool and beast of burden.
	std::list<HaulSubproject> m_haulSubprojects;
	// Delivered items.
	SmallSet<ItemReference> m_deliveredItems;
	// Where the materials are delivered to and where the work gets done.
	Area& m_area;
	FactionId m_faction;
	BlockIndex m_location;
	Project(const FactionId& f, Area& a, const BlockIndex& l, const Quantity& mw, std::unique_ptr<DishonorCallback> locationDishonorCallback = nullptr);
	Project(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
private:
	// Count how many times we have attempted to create a haul subproject.
	// Once we hit the limit, defined as projectTryToMakeSubprojectRetriesBeforeProjectDelay in config.json, the project calls setDelayOn.
	Quantity m_haulRetries = Quantity::create(0);
	//TODO: Decrement by a config value instead of 1.
	Speed m_minimumMoveSpeed = Speed::create(0);
	Quantity m_maxWorkers = Quantity::create(0);
	bool m_requirementsLoaded = false;
	// If a project fails multiple times to create a haul subproject it resets and calls onDelay
	// The onDelay method is expected to remove the project from listings, remove block designations, etc.
	// The scheduled event sets delay to false and calls the offDelay method.
	bool m_delay = false;
public:
	// Seperated from primary Json constructor because must be run after objectives are created.
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	void addWorkerCandidate(const ActorIndex& actor, Objective& objective);
	void removeWorkerCandidate(const ActorIndex& actor);
	// To be called by Objective::execute.
	void commandWorker(const ActorIndex& actor);
	// To be called by Objective::interupt.
	void removeWorker(const ActorIndex& actor);
	void addToMaking(const ActorIndex& actor);
	void removeFromMaking(const ActorIndex& actor);
	void complete();
	// To be called by the player for manually created project types or in place of reset otherwise.
	void cancel();
	//TODO: impliment this.
	void dismissWorkers();
	void scheduleFinishEvent(Step start = Step::null());
	void haulSubprojectComplete(HaulSubproject& haulSubproject);
	void haulSubprojectCancel(HaulSubproject& haulSubproject);
	void setLocationDishonorCallback(std::unique_ptr<DishonorCallback> dishonorCallback);
	// Calls onDelay and schedules delay end.
	void setDelayOn();
	// Calls offDelay.
	void setDelayOff();
	// Record reserved shapes which need haul subprojects dispatched for them.
	void addToPickup(const ActorOrItemIndex& actor, ProjectRequirementCounts& counts, const Quantity& quantity);
	void removeToPickup(const ActorOrItemIndex& actor, const Quantity& quantity);
	// To be called when the last worker is removed, when a haul subproject repetidly fails, or when a required reservation is dishonored, resets to pre-reservations status.
	void reset();
	// TODO: Implimentation of this.
	void resetOrCancel();
	// Before unload when shutting down or hibernating.
	void clearReservations();
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] FactionId getFaction() { return m_faction; }
	[[nodiscard]] CanReserve& getCanReserve() { return m_canReserve; }
	[[nodiscard]] Speed getMinimumHaulSpeed() const { return m_minimumMoveSpeed; }
	[[nodiscard]] bool reservationsComplete() const;
	[[nodiscard]] bool deliveriesComplete() const;
	[[nodiscard]] bool isOnDelay() { return m_delay; }
	// Block where the work will be done.
	[[nodiscard]] BlockIndex getLocation() const { return m_location; }
	[[nodiscard]] bool hasCandidate(const ActorIndex& actor) const;
	[[nodiscard]] bool hasWorker(const ActorIndex& actor) const;
	// When cannotCompleteSubobjective is called do we reset and try again or do we call cannotCompleteObjective?
	// Should be false for objectives like targeted hauling, where if the specific target is inaccessable there is no fallback possible.
	[[nodiscard]] virtual bool canReset() const { return true; }
	[[nodiscard]] ActorIndices getWorkersAndCandidates();
	[[nodiscard]] std::vector<std::pair<ActorIndex, Objective*>> getWorkersAndCandidatesWithObjectives();
	[[nodiscard]] Percent getPercentComplete() const { return m_finishEvent.exists() ? m_finishEvent.percentComplete() : Percent::create(0); }
	[[nodiscard]] virtual bool canAddWorker(const ActorIndex& actor) const;
	// What would the total delay time be if we started from scratch now with current workers?
	[[nodiscard]] virtual Step getDuration() const = 0;
	// True for stockpile because there is no 'work' to do after the hauling is done.
	[[nodiscard]] virtual bool canRecruitHaulingWorkersOnly() const { return false; }
	[[nodiscard]] Area& getArea() { return m_area; }
	virtual void onComplete() = 0;
	virtual void onReserve() { }
	virtual void onCancel() { }
	virtual void onAddToMaking(const ActorIndex&) { }
	virtual void onDelivered(const ActorOrItemIndex&) { }
	virtual void onSubprojectCreated(HaulSubproject& subproject) { (void)subproject; }
	virtual void onActorOrItemReservationDishonored() { if(canReset()) reset(); else cancel(); }
	// Projects which are initiated by the users, such as dig or construct, must be delayed when they cannot be completed. Projectes which are initiated automatically, such as Stockpile or Craft, can be canceled.
	virtual void onDelay() = 0;
	virtual void offDelay() = 0;
	// Use copies rather then references for return types to allow specalization of Queries as well as byproduct material type.
	[[nodiscard]] virtual std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const = 0;
	[[nodiscard]] virtual std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const = 0;
	[[nodiscard]] virtual std::vector<std::pair<ActorQuery, Quantity>> getActors() const = 0;
	[[nodiscard]] virtual std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const = 0;
	virtual ~Project() = default;
	[[nodiscard]] bool operator==(const Project& other) const { return &other == this; }
	// For testing.
	[[nodiscard]] ProjectWorker& getProjectWorkerFor(ActorReference actor) { return m_workers[actor]; }
	[[nodiscard, maybe_unused]] auto& getWorkers() { return m_workers; }
	[[nodiscard, maybe_unused]] Quantity getHaulRetries() const { return m_haulRetries; }
	[[nodiscard, maybe_unused]] bool hasTryToHaulThreadedTask() const { return m_tryToHaulThreadedTask.exists(); }
	[[nodiscard, maybe_unused]] bool hasTryToHaulEvent() const { return m_tryToHaulEvent.exists(); }
	[[nodiscard, maybe_unused]] bool hasTryToAddWorkersThreadedTask() const { return m_tryToAddWorkersThreadedTask.exists(); }
	[[nodiscard, maybe_unused]] bool hasTryToReserveEvent() const { return m_tryToReserveEvent.exists(); }
	[[nodiscard, maybe_unused]] bool finishEventExists() const { return m_finishEvent.exists(); }
	[[nodiscard, maybe_unused]] Step getFinishStep() const { return m_finishEvent.getStep(); }
	[[nodiscard, maybe_unused]] auto getToPickup() const { return m_toPickup; }
	[[nodiscard]] Quantity getMaxWorkers() const { return m_maxWorkers; }
	friend class ProjectFinishEvent;
	friend class ProjectTryToHaulEvent;
	friend class ProjectTryToReserveEvent;
	friend class ProjectTryToMakeHaulSubprojectThreadedTask;
	friend class ProjectTryToAddWorkersThreadedTask;
	friend class HaulSubproject;
	Project(const Project&) = delete;
	Project(const Project&&) = delete;
};
inline void to_json(Json& data, const Project& project){ data = reinterpret_cast<uintptr_t>(&project); }
inline void to_json(Json& data, const Project* const& project){ data = reinterpret_cast<uintptr_t>(project); }
class ProjectFinishEvent final : public ScheduledEvent
{
	Project& m_project;
public:
	ProjectFinishEvent(const Step& delay, Project& p, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class ProjectTryToHaulEvent final : public ScheduledEvent
{
	Project& m_project;
public:
	ProjectTryToHaulEvent(const Step& delay, Project& p, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class ProjectTryToReserveEvent final : public ScheduledEvent
{
	Project& m_project;
public:
	ProjectTryToReserveEvent(const Step& delay, Project& p, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class ProjectTryToMakeHaulSubprojectThreadedTask final : public ThreadedTask
{
	Project& m_project;
	HaulSubprojectParamaters m_haulProjectParamaters;
public:
	ProjectTryToMakeHaulSubprojectThreadedTask(Project& p);
	void readStep(Simulation& simulation, Area* area);
	void writeStep(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	[[nodiscard]] bool blockContainsDesiredItem(const BlockIndex& block, const ActorIndex& hauler);
};
class ProjectTryToAddWorkersThreadedTask final : public ThreadedTask
{
	Project& m_project;
	SmallSet<ActorReference> m_cannotPathToJobSite;
	SmallMap<ActorOrItemReference, Quantity> m_alreadyAtSite;
	SmallMap<ActorReference, SmallMap<ProjectRequirementCounts*, ItemReference>> m_reservedEquipment;
	HasOnDestroySubscriptions m_hasOnDestroy;
	void resetProjectCounts();
public:
	ProjectTryToAddWorkersThreadedTask(Project& p);
	void readStep(Simulation& simulation, Area* area);
	void writeStep(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	[[nodiscard]] bool validate();
};
class BlockHasProjects
{
	FactionIdMap<SmallSet<Project*>> m_data;
public:
	void add(Project& project);
	void remove(Project& project);
	Percent getProjectPercentComplete(const FactionId& faction) const;
	[[nodiscard]] Project* get(const FactionId& faction) const;
};
