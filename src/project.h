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
	HasScheduledEvent<ProjectTryToHaulEvent> m_tryToHaulEvent;
	HasScheduledEvent<ProjectTryToReserveEvent> m_tryToReserveEvent;
	HasThreadedTask<ProjectTryToMakeHaulSubprojectThreadedTask> m_tryToHaulThreadedTask;
	HasThreadedTask<ProjectTryToAddWorkersThreadedTask> m_tryToAddWorkersThreadedTask;
	bool m_gatherComplete;
	//TODO: Reservations which cannot be honored cause the project to reset.
	CanReserve m_canReserve;
	std::vector<std::pair<ItemQuery, ProjectItemCounts>> m_requiredItems;
	//TODO: required actors.
	std::vector<std::pair<ActorQuery, ProjectItemCounts>> m_requiredActors;
	std::unordered_set<Item*> m_toConsume;
	size_t m_maxWorkers;
	// In order for the first worker to join a project the worker must be able to path to all the required shapes.
	// If this is not possible delay is set to true, a scheduled event is created to end the delay, and the project's onDelay method is called.
	// The onDelay method is expected to remove the project from listings, remove block designations, etc.
	// The scheduled event sets delay to false and calls the offDelay method.
	bool m_delay;
	// Worker candidates are evaluated in a read step task.
	// For the first we need to reserve all items and actors, as wel as verify access.
	// For subsequent candidates we just need to verify that they can path to the construction site.
	std::vector<std::pair<Actor*, Objective*>> m_workerCandidatesAndTheirObjectives;
	std::unordered_map<HasShape*, std::pair<ProjectItemCounts*, uint32_t>> m_toPickup;
	bool m_requirementsLoaded;
	void addWorker(Actor& actor, Objective& objective);
	void recordRequiredActorsAndItems();
protected:
	Block& m_location;
	const Faction& m_faction;
	std::unordered_map<Actor*, ProjectWorker> m_workers;
	std::unordered_set<Actor*> m_waiting;
	std::unordered_set<Actor*> m_making;
	std::list<HaulSubproject> m_haulSubprojects;
	Project(const Faction* f, Block& l, size_t mw);
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
	// TODO: minimum speed decreses with failed attempts to generate haul subprojects.
	[[nodiscard]] const Faction& getFaction() { return m_faction; }
	[[nodiscard]] uint32_t getMinimumHaulSpeed() const { return Config::minimumHaulSpeed; }
	[[nodiscard]] bool canAddWorker(const Actor& actor) const;
	[[nodiscard]] bool reservationsComplete() const;
	[[nodiscard]] bool deliveriesComplete() const;
	[[nodiscard]] bool isOnDelay() { return m_delay; }
	[[nodiscard]] Block& getLocation() { return m_location; }
	[[nodiscard]] const Block& getLocation() const { return m_location; }
	[[nodiscard]] bool hasCandidate(const Actor& actor) const;
	// What would the total delay time be if we started from scratch now with current workers?
	[[nodiscard]] virtual Step getDuration() const = 0;
	virtual void onComplete() = 0;
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
