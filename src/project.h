#pragma once

#include "reservable.h"
#include "actor.h"
#include "eventSchedule.h"
#include "threadedTask.h"
#include "subproject.h"
#include "haul.h"
#include "item.h"

#include <vector>
#include <utility>
#include <unordered_set>
#include <unordered_map>
class Project;
class Block;

class ProjectFinishEvent : public ScheduledEventWithPercent
{
	Project& m_project;
public:
	ProjectFinishEvent(uint32_t delay, Project& p);
	void execute();
	~ProjectFinishEvent();
};
class ProjectTryToHaulEvent : public ScheduledEventWithPercent
{
	Project& m_project;
public:
	ProjectTryToHaulEvent(uint32_t delay, Project& p) : ScheduledEventWithPercent(delay), m_project(p) { }
	void execute();
	~ProjectTryToHaulEvent();
};
class ProjectTryToMakeHaulSubprojectThreadedTask : public ThreadedTask
{
	Project& m_project;
	HaulSubprojectParamaters m_haulProjectParamaters;
	bool m_result;
public:
	ProjectTryToMakeHaulSubprojectThreadedTask(Project& p) : m_project(p) { }
	void readStep();
	void writeStep();
};
struct ProjectWorker
{
	Subproject* haulSubproject;
	ProjectWorker() : haulSubproject(nullptr) { }
};
struct ProjectItemCounts
{
	uint8_t required;
	uint8_t reserved;
	uint8_t delivered;
	ProjectItemCounts(uint8_t r) : required(r), reserved(0), delivered(0) { }
};
// Derived classes are expected to provide getDelay, getConsumedItems, getUnconsumedItems, getByproducts, and onComplete.
class Project
{
	HasScheduledEventPausable<ProjectFinishEvent> m_finishEvent;
	HasScheduledEvent<ProjectTryToHaulEvent> m_tryToHaulEvent;
	HasThreadedTask<ProjectTryToMakeHaulSubprojectThreadedTask> m_tryToHaulThreadedTask;
	bool m_gatherComplete;
	Block& m_location;
	CanReserve m_canReserve;
	std::vector<std::pair<ItemQuery, ProjectItemCounts>> m_requiredItems;
	std::vector<std::pair<ActorQuery, ProjectItemCounts>> m_requiredActors;
	std::unordered_set<Item*> m_toConsume;
	size_t m_maxWorkers;
	bool reservationsComplete() const;
	bool deliveriesComplete() const;
protected:
	std::unordered_map<Actor*, ProjectWorker> m_workers;
	std::unordered_set<Actor*> m_waiting;
	std::unordered_set<Actor*> m_making;
	std::list<HaulSubproject> m_haulSubprojects;
	Project(Block& l, size_t mw) : m_gatherComplete(false), m_location(l), m_maxWorkers(mw) { }
public:
	void recordRequiredActorsAndItemsAndReserveLocation();
	void addWorker(Actor& actor);
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
	void createHaulSubproject(HaulSubprojectParamaters haulSubprojectParamaters);
	// TODO: minimum speed decreses with failed attempts to generate haul subprojects.
	bool getMinimumHaulSpeed() const { return Config::minimumHaulSpeed; }
	bool canGatherItemAt(Actor& actor, Block& block) const;
	std::pair<Item*, ProjectItemCounts*> gatherableItemAtWithProjectItemCounts(Actor& actor, Block& block) const;
	bool canGatherActorAt(Actor& actor, Block& block) const;
	Actor* gatherableActorAt(Actor& actor, Block& block) const;
	void subprojectComplete(Subproject& subproject);
	bool tryToBeginItemHaulSubproject(Subproject& subproject, Actor& actor);
	virtual uint32_t getDelay() const = 0;
	virtual void onComplete() = 0;
	// Use copies rather then references for return types to allow specalization of Queries as well as byproduct material type.
	virtual std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const = 0;
	virtual std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const = 0;
	virtual std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const = 0;
	virtual std::vector<std::pair<ActorQuery, uint32_t>> getActors() const = 0;
	Block& getLocation() const { return m_location; }
	virtual ~Project();
	friend class ProjectFinishEvent;
	friend class ProjectTryToHaulEvent;
	friend class ProjectGoToAdjacentLocationThreadedTask;
	friend class ProjectFindItemsThreadedTask;
	friend class ProjectFindActorsThreadedTask;
	friend class ProjectTryToMakeHaulSubprojectThreadedTask;
	friend class HaulSubproject;
};
