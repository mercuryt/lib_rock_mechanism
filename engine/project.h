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
	ProjectRequirementCounts() = default;
	ProjectRequirementCounts(const ProjectRequirementCounts& other) = default;
	ProjectRequirementCounts& operator=(const ProjectRequirementCounts& other) = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProjectRequirementCounts, required, delivered, reserved, consumed);
};
struct FluidRequirementData final
{
	ProjectRequirementCounts counts;
	SmallMap<ItemReference, CollisionVolume> containersAndVolumes;
	SmallSet<BlockIndex> foundBlocks;
	CollisionVolume volumeRequired;
	CollisionVolume volumeFound = CollisionVolume::create(0);
	FluidRequirementData() = default;
	FluidRequirementData(const CollisionVolume& v) :
		counts(Quantity::create(v.get()), true),
		volumeRequired(v)
	{ }
	FluidRequirementData(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	FluidRequirementData& operator=(const FluidRequirementData& other) = default;
	[[nodiscard]] Json toJson() const
	{
		Json output
		{
			{"counts", counts},
			{"containersAndVolumes", Json::array()},
			{"volumeRequired", volumeRequired},
			{"volumeFound", volumeFound}
		};
		for(const auto& [container, volume] : containersAndVolumes)
			output["containersAndVolumes"].push_back({container, volume});
		output["counts"]["address"] = reinterpret_cast<uintptr_t>(&counts);
		return output;
	}
};
struct ProjectRequiredShapeDishonoredCallback final : public DishonorCallback
{
	Project& m_project;
	ProjectRequiredShapeDishonoredCallback(Project& p) : m_project(p) { }
	ProjectRequiredShapeDishonoredCallback(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(const Quantity&, const Quantity&);
	[[nodiscard]] Json toJson() const;
};
// Derived classes are expected to provide getDelay, getConsumedItems, getUnconsumedItems, getByproducts, and onComplete. onDelivered is optional.
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
	std::vector<ActorReference> m_requiredActors;
	SmallMap<FluidTypeId, FluidRequirementData> m_requiredFluids;
	// Required items which will be destroyed at the end of the project.
	SmallMap<ItemReference, Quantity> m_toConsume;
	// Required items which are equiped by workers (tools).
	SmallMap<ActorReference, SmallMap<ProjectRequirementCounts*, ItemReference>> m_reservedEquipment;
	// Containers with fluid to consume at end of project.
	SmallSet<ItemReference> m_containersWithFluidToConsume;
	// Targets for haul subprojects awaiting dispatch.
	SmallMap<ItemReference, std::pair<ProjectRequirementCounts*, Quantity>> m_itemsToPickup;
	SmallSet<ActorReference> m_actorsToPickup;
	SmallSet<ItemReference> m_fluidContainersToPickup;
	// To be called by addWorkerThreadedTask, after validating the worker has access to the project location.
	void addWorker(const ActorIndex& actor, Objective& objective);
	// Load requirements from child class.
	void recordRequiredActorsAndItemsAndFluids();
protected:
	// Workers who have passed the candidate screening and ProjectWorker, which holds a reference to the worker's objective and a pointer to it's haul subproject.
	SmallMap<ActorReference, ProjectWorker> m_workers;
	// Required shapes which don't need to be hauled because they are already at or adjacent to m_location.
	// To be used by StockpileProject::onComplete.
	SmallSet<ActorReference> m_actorAlreadyAtSite;
	SmallMap<ItemReference, Quantity> m_itemAlreadyAtSite;
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
	SmallMap<ItemReference, Quantity> m_deliveredItems;
	// Delivered Actors.
	SmallSet<ActorReference> m_deliveredActors;
	Area& m_area;
	FactionId m_faction;
	// Where the materials are delivered to and where the work gets done.
	BlockIndex m_location;
	Project(const FactionId& faction, Area& area, const BlockIndex& location, const Quantity& maxWorkers, std::unique_ptr<DishonorCallback> locationDishonorCallback = nullptr, const BlockIndices& additionalBlocksToReserve = {});
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
	void addActorToPickup(const ActorIndex& actor);
	void addItemToPickup(const ItemIndex& item, ProjectRequirementCounts& counts, const Quantity& quantity);
	void removeActorToPickup(const ActorReference& actor);
	void removeFluidContainerToPickup(const ItemReference& item);
	void removeItemToPickup(const ItemReference& item, const Quantity& quantity);
	// To be called when the last worker is removed, when a haul subproject repetidly fails, or when a required reservation is dishonored, resets to pre-reservations status.
	void reset();
	void resetOrCancel() { if(canReset()) reset(); else cancel(); }
	// Before unload when shutting down or hibernating.
	void clearReservations();
	// In HaulSubproject::complete, before merging generics.
	void clearReferenceFromRequiredItems(const ItemReference& ref);
	// These two are to be used when a project is on a moving vehicle.
	void clearLocation();
	void setLocation(const BlockIndex& block);
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
	virtual void onPickUpRequired(const ActorOrItemIndex&) { }
	// Projects which are initiated by the users, such as dig or construct, must be delayed when they cannot be completed. Projectes which are initiated automatically, such as Stockpile or Craft, can be canceled.
	virtual void onDelay() = 0;
	virtual void offDelay() = 0;
	virtual void updateRequiredGenericReference(const ItemReference&) { }
	// Use copies rather then references for return types to allow specalization of Queries as well as byproduct material type.
	[[nodiscard]] virtual std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const = 0;
	[[nodiscard]] virtual std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const = 0;
	[[nodiscard]] virtual std::vector<ActorReference> getActors() const = 0;
	[[nodiscard]] virtual SmallMap<FluidTypeId, CollisionVolume> getFluids() const { return { }; };
	[[nodiscard]] virtual std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const = 0;
	virtual ~Project() = default;
	[[nodiscard]] bool operator==(const Project& other) const { return &other == this; }
	[[nodiscard]] bool itemIsFluidContainer(const ItemReference& item) const;
	[[nodiscard]] FluidTypeId getFluidTypeForContainer(const ItemReference& item) const;
	[[nodiscard]] CollisionVolume getFluidVolumeForContainer(const ItemReference& item) const;
	// For testing.
	[[nodiscard]] const ProjectWorker& getProjectWorkerFor(ActorReference actor) const { return m_workers[actor]; }
	[[nodiscard]] auto& getWorkers() { return m_workers; }
	[[nodiscard]] Quantity getHaulRetries() const { return m_haulRetries; }
	[[nodiscard]] bool hasTryToHaulThreadedTask() const { return m_tryToHaulThreadedTask.exists(); }
	[[nodiscard]] bool hasTryToHaulEvent() const { return m_tryToHaulEvent.exists(); }
	[[nodiscard]] bool hasTryToAddWorkersThreadedTask() const { return m_tryToAddWorkersThreadedTask.exists(); }
	[[nodiscard]] bool hasTryToReserveEvent() const { return m_tryToReserveEvent.exists(); }
	[[nodiscard]] bool finishEventExists() const { return m_finishEvent.exists(); }
	[[nodiscard]] Step getFinishStep() const { return m_finishEvent.getStep(); }
	[[nodiscard]] auto getToPickup() const { return m_itemsToPickup; }
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
	HaulSubprojectParamaters m_haulProjectParamaters;
	Project& m_project;
public:
	ProjectTryToMakeHaulSubprojectThreadedTask(Project& p);
	void readStep(Simulation& simulation, Area* area);
	void writeStep(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	[[nodiscard]] bool blockContainsDesiredItemOrActor(const BlockIndex& block, const ActorIndex& hauler);
};
class ProjectTryToAddWorkersThreadedTask final : public ThreadedTask
{
	SmallSet<ActorReference> m_cannotPathToJobSite;
	SmallMap<ItemReference, Quantity> m_itemAlreadyAtSite;
	SmallSet<ActorReference> m_actorAlreadyAtSite;
	SmallMap<ActorReference, SmallMap<ProjectRequirementCounts*, ItemReference>> m_reservedEquipment;
	Project& m_project;
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
