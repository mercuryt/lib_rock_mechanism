#pragma once

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
class ProjectTryToMakeHaulSubprojectThreadedTask;
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
	uint8_t reserved;
	uint8_t delivered;
	ProjectItemCounts(const uint8_t r) : required(r), reserved(0), delivered(0) { }
};
// Derived classes are expected to provide getDelay, getConsumedItems, getUnconsumedItems, getByproducts, and onComplete.
class Project
{
	HasScheduledEventPausable<ProjectFinishEvent> m_finishEvent;
	HasScheduledEvent<ProjectTryToHaulEvent> m_tryToHaulEvent;
	HasThreadedTask<ProjectTryToMakeHaulSubprojectThreadedTask> m_tryToHaulThreadedTask;
	bool m_gatherComplete;
	CanReserve m_canReserve;
	std::vector<std::pair<ItemQuery, ProjectItemCounts>> m_requiredItems;
	std::vector<std::pair<ActorQuery, ProjectItemCounts>> m_requiredActors;
	std::unordered_set<Item*> m_toConsume;
	size_t m_maxWorkers;
	bool m_loadedRequirements;
	bool reservationsComplete() const;
	bool deliveriesComplete() const;
protected:
	Block& m_location;
	std::unordered_map<Actor*, ProjectWorker> m_workers;
	std::unordered_set<Actor*> m_waiting;
	std::unordered_set<Actor*> m_making;
	std::list<HaulSubproject> m_haulSubprojects;
	Project(const Faction* f, Block& l, size_t mw);
public:
	void recordRequiredActorsAndItemsAndReserveLocation();
	void addWorker(Actor& actor, Objective& objective);
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
	// TODO: minimum speed decreses with failed attempts to generate haul subprojects.
	[[nodiscard]] uint32_t getMinimumHaulSpeed() const { return Config::minimumHaulSpeed; }
	[[nodiscard]] bool canGatherItemAt(const Actor& actor, const Block& block) const;
	[[nodiscard]] std::pair<Item*, ProjectItemCounts*> gatherableItemAtWithProjectItemCounts(const Actor& actor, const Block& block) const;
	[[nodiscard]] bool canGatherActorAt(const Actor& actor, const Block& block) const;
	[[nodiscard]] Actor* gatherableActorAt(const Actor& actor, const Block& block) const;
	void haulSubprojectComplete(HaulSubproject& haulSubproject);
	[[nodiscard]] Block& getLocation() const { return m_location; }
	[[nodiscard]] virtual Step getDelay() const = 0;
	virtual void onComplete() = 0;
	// Use copies rather then references for return types to allow specalization of Queries as well as byproduct material type.
	[[nodiscard]] virtual std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const = 0;
	[[nodiscard]] virtual std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const = 0;
	[[nodiscard]] virtual std::vector<std::pair<ActorQuery, uint32_t>> getActors() const = 0;
	[[nodiscard]] virtual std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const = 0;
	virtual ~Project();
	[[nodiscard]] bool operator==(const Project& other) const { return &other == this; }
	// For testing.
	[[nodiscard]] ProjectWorker& getProjectWorkerFor(Actor& actor) { return m_workers.at(&actor); }
	friend class ProjectFinishEvent;
	friend class ProjectTryToHaulEvent;
	friend class ProjectGoToAdjacentLocationThreadedTask;
	friend class ProjectFindItemsThreadedTask;
	friend class ProjectFindActorsThreadedTask;
	friend class ProjectTryToMakeHaulSubprojectThreadedTask;
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
class ProjectTryToMakeHaulSubprojectThreadedTask final : public ThreadedTask
{
	Project& m_project;
	HaulSubprojectParamaters m_haulProjectParamaters;
	FindsPath m_findsPath;
public:
	ProjectTryToMakeHaulSubprojectThreadedTask(Project& p);
	void readStep();
	void writeStep();
	void clearReferences();
};
