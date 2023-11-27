#pragma once

#include "hasShape.h"
#include "reservable.h"
#include "actor.h"
#include "eventSchedule.h"
#include "threadedTask.h"
#include "haul.h"
#include "item.h"
#include "actor.h"
#include "findsPath.h"
#include "eventSchedule.hpp"

#include <vector>
#include <utility>
#include <unordered_set>
#include <unordered_map>
class ProjectFinishEvent;
class ProjectTryToHaulEvent;
class ProjectTryToReserveEvent;
class ProjectTryToMakeHaulSubprojectThreadedTask;
class ProjectTryToAddWorkersThreadedTask;
class Block;

struct ProjectWorker final
{
	HaulSubproject* haulSubproject;
	Objective& objective;
	ProjectWorker(Objective& o) : haulSubproject(nullptr), objective(o) { }
};
struct ProjectItemCounts final
{
	const uint8_t required;
	uint8_t delivered;
	uint8_t reserved;
	bool consumed;
	ProjectItemCounts(const uint8_t r, bool c) : required(r), delivered(0), reserved(0), consumed(c) { }
};
// Derived classes are expected to provide getDelay, getConsumedItems, getUnconsumedItems, getByproducts, and onComplete.
class Project
{
	// Tracks the progress of the 'actual' work of the project, after the hauling is done.
	HasScheduledEventPausable<ProjectFinishEvent> m_finishEvent;
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
	//TODO: Reservations which cannot be honored cause the project to reset.
	CanReserve m_canReserve;
	// Queries for items needed for the project, counts of required, reserved and delivered.
	std::vector<std::pair<ItemQuery, ProjectItemCounts>> m_requiredItems;
	//TODO: required actors are not suported in several places.
	std::vector<std::pair<ActorQuery, ProjectItemCounts>> m_requiredActors;
	// Required items which will be destroyed at the end of the project.
	std::unordered_set<Item*> m_toConsume;
	size_t m_maxWorkers;
	// In order for the first worker to join a project the worker must be able to path to all the required shapes.
	// If this is not possible delay is set to true, a scheduled event is created to end the delay, and the project's onDelay method is called.
	// The onDelay method is expected to remove the project from listings, remove block designations, etc.
	// The scheduled event sets delay to false and calls the offDelay method.
	bool m_delay;
	// Count how many times we have attempted to create a haul subproject.
	// Once we hit the limit, defined as projectTryToMakeSubprojectRetriesBeforeProjectDelay in config.json, the project calls setDelayOn.
	uint8_t m_haulRetries;
	// Targets for haul subprojects awaiting dispatch.
	std::unordered_map<HasShape*, std::pair<ProjectItemCounts*, uint32_t>> m_toPickup;
	bool m_requirementsLoaded;
	// To be called by addWorkerThreadedTask, after validating the worker has access to the project location.
	void addWorker(Actor& actor, Objective& objective);
	// Load requirements from child class.
	void recordRequiredActorsAndItems();
protected:
	// Where the materials are delivered to and where the work gets done.
	Block& m_location;
	const Faction& m_faction;
	// Workers who have passed the candidate screening and ProjectWorker, which holds a reference to the worker's objective and a pointer to it's haul subproject.
	std::unordered_map<Actor*, ProjectWorker> m_workers;
	// Workers present at the job site, waiting for haulers to deliver required materiels.
	std::unordered_set<Actor*> m_waiting;
	// Workers currently doing the 'actual' work.
	std::unordered_set<Actor*> m_making;
	// Worker candidates are evaluated in a read step task.
	// For the first we need to reserve all items and actors, as wel as verify access.
	// For subsequent candidates we just need to verify that they can path to the construction site.
	std::vector<std::pair<Actor*, Objective*>> m_workerCandidatesAndTheirObjectives;
	// Single hauling operations are managed by haul subprojects.
	// They have one or more workers plus optional haul tool and beast of burden.
	std::list<HaulSubproject> m_haulSubprojects;
	Project(const Faction* f, Block& l, size_t mw, DishonorCallback locationDishonorCallback = nullptr);
public:
	void addWorkerCandidate(Actor& actor, Objective& objective);
	void removeWorkerCandidate(Actor& actor);
	// To be called by Objective::execute.
	void commandWorker(Actor& actor);
	// To be called by Objective::interupt.
	void removeWorker(Actor& actor);
	void addToMaking(Actor& actor);
	void removeFromMaking(Actor& actor);
	void complete();
	void cancel();
	void dismissWorkers();
	void scheduleEvent();
	void createHaulSubproject(const HaulSubprojectParamaters haulSubprojectParamaters);
	void haulSubprojectComplete(HaulSubproject& haulSubproject);
	void setDelayOn() { m_delay = true; onDelay(); }
	void setDelayOff() { m_delay = false; offDelay(); }
	// To be called when the last worker is removed, resets to pre-reservations complete status.
	virtual void reset();
	// TODO: minimum speed decreses with failed attempts to generate haul subprojects.
	[[nodiscard]] const Faction& getFaction() { return m_faction; }
	[[nodiscard]] uint32_t getMinimumHaulSpeed() const { return Config::minimumHaulSpeed; }
	[[nodiscard]] bool reservationsComplete() const;
	[[nodiscard]] bool deliveriesComplete() const;
	[[nodiscard]] bool isOnDelay() { return m_delay; }
	[[nodiscard]] Block& getLocation() { return m_location; }
	[[nodiscard]] const Block& getLocation() const { return m_location; }
	[[nodiscard]] bool hasCandidate(const Actor& actor) const;
	[[nodiscard]] virtual bool canAddWorker(const Actor& actor) const;
	// What would the total delay time be if we started from scratch now with current workers?
	[[nodiscard]] virtual Step getDuration() const = 0;
	virtual void onComplete() = 0;
	virtual void onReserve() { }
	virtual void onCancel() { }
	virtual void onDelivered(HasShape& hasShape) { (void)hasShape; }
	virtual void onSubprojectCreated(HaulSubproject& subproject) { (void)subproject; }
	// Projects which are initiated by the users, such as dig or construct, must be delayed when they cannot be completed. Projectes which are initiated automatically, such as Stockpile or Craft, can be canceled.
	virtual void onDelay() = 0;
	virtual void offDelay() = 0;
	// Use copies rather then references for return types to allow specalization of Queries as well as byproduct material type.
	[[nodiscard]] virtual std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const = 0;
	[[nodiscard]] virtual std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const = 0;
	[[nodiscard]] virtual std::vector<std::pair<ActorQuery, uint32_t>> getActors() const = 0;
	[[nodiscard]] virtual std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const = 0;
	virtual ~Project();
	[[nodiscard]] bool operator==(const Project& other) const { return &other == this; }
	// For testing.
	[[nodiscard]] ProjectWorker& getProjectWorkerFor(Actor& actor) { return m_workers.at(&actor); }
	[[nodiscard, maybe_unused]] std::unordered_map<Actor*, ProjectWorker> getWorkers() { return m_workers; }
	friend class ProjectFinishEvent;
	friend class ProjectTryToHaulEvent;
	friend class ProjectTryToReserveEvent;
	friend class ProjectTryToMakeHaulSubprojectThreadedTask;
	friend class ProjectTryToAddWorkersThreadedTask;
	friend class HaulSubproject;
};
class ProjectFinishEvent final : public ScheduledEventWithPercent
{
	Project& m_project;
public:
	ProjectFinishEvent(const Step delay, Project& p);
	void execute();
	void clearReferences();
};
class ProjectTryToHaulEvent final : public ScheduledEventWithPercent
{
	Project& m_project;
public:
	ProjectTryToHaulEvent(const Step delay, Project& p);
	void execute();
	void clearReferences();
};
class ProjectTryToReserveEvent final : public ScheduledEventWithPercent
{
	Project& m_project;
public:
	ProjectTryToReserveEvent(const Step delay, Project& p);
	void execute();
	void clearReferences();
};
class ProjectTryToMakeHaulSubprojectThreadedTask final : public ThreadedTask
{
	Project& m_project;
	HaulSubprojectParamaters m_haulProjectParamaters;
public:
	ProjectTryToMakeHaulSubprojectThreadedTask(Project& p);
	void readStep();
	void writeStep();
	void clearReferences();
	[[nodiscard]] bool blockContainsDesiredItem(const Block& block, Actor& hauler);
};
class ProjectTryToAddWorkersThreadedTask final : public ThreadedTask
{
	Project& m_project;
	std::unordered_set<Actor*> m_cannotPathToJobSite;
	std::unordered_map<HasShape*, uint32_t> m_alreadyAtSite;
	void resetProjectCounts();
public:
	ProjectTryToAddWorkersThreadedTask(Project& p);
	void readStep();
	void writeStep();
	void clearReferences();
	[[nodiscard]] bool validate();
};
