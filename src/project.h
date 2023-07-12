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
class ProjectGoToAdjacentLocationThreadedTask : public ThreadedTask
{
	Project& m_project;
	Actor& m_actor;
	bool m_reserve; // Set reserve to true for going to a position to start making, set to false for going to drop off materials.
	std::vector<Block*> m_route;
public:
	ProjectGoToAdjacentLocationThreadedTask(Project& p, Actor& a, bool r) : m_project(p), m_actor(a), m_reserve(r) { }
	void readStep();
	void writeStep();
};
class ProjectFindItemsThreadedTask : public ThreadedTask
{
	Project& m_project;
	Actor& m_actor;
	std::vector<Block*> m_route;
public:
	ProjectFindItemsThreadedTask(Project& p, Actor& a): m_project(p), m_actor(a) { }
	void readStep();
	void writeStep();
};
class ProjectFindActorsThreadedTask : public ThreadedTask
{
	Project& m_project;
	Actor& m_actor;
	std::vector<Block*> m_route;
public:
	ProjectFindActorsThreadedTask(Project& p, Actor& a);
	void readStep();
	void writeStep();
};
struct ProjectWorker
{
	HasThreadedTask<ProjectGoToAdjacentLocationThreadedTask> goToTask;
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
	std::list<HaulSubproject<Item>> m_haulItemSubprojects;
	std::list<HaulSubproject<Actor>> m_haulActorSubprojects;
	Project(Block& l, size_t mw) : m_gatherComplete(false), m_location(l), m_maxWorkers(mw) { }
public:
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
	bool canGatherItemAt(Actor& actor, Block& block) const;
	Item* gatherableItemAt(Actor& actor, Block& block) const;
	bool canGatherActorAt(Actor& actor, Block& block) const;
	Actor* gatherableActorAt(Actor& actor, Block& block) const;
	void subprojectComplete(Subproject& subproject);
	bool tryToBeginItemHaulSubproject(Subproject& subproject, Actor& actor);
	virtual uint32_t getDelay() const = 0;
	virtual void onComplete() = 0;
	virtual std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const = 0;
	virtual std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const = 0;
	virtual std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>>& getByproducts() const = 0;
	virtual std::vector<std::pair<ActorQuery, uint32_t>> getActors() const = 0;
	virtual ~Project();
};
