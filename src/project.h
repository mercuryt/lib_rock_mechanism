#pragma once

#include <vector>
#include <pair>
#include <unordered_set>
#include <unordered_map>

class ProjectFinishEvent : public ScheduledEventWithPercent
{
	Project& m_project;
public:
	ProjectFinishEvent(uint32_t step, Project& p);
	void execute();
	~ProjectFinishEvent();
};
class ProjectGoToAdjacentLocationThreadedTask : ThreadedTask
{
	Project& m_project;
	Actor& m_actor;
	bool m_reserve; // Set reserve to true for going to a position to start making, set to false for going to drop off materials.
	std::vector<Block*> m_route;
public:
	ProjectGoToAdjacentLocationThreadedTask(Project& p, Actor& a, bool r);
	void readStep();
	void writeStep();
};
class ProjectFindItemsThreadedTask : ThreadedTask
{
	Project& m_project;
	Actor& m_actor;
	std::vector<Block*> m_route;
public:
	ProjectFindItemsThreadedTask(Project& p, Actor& a):
	void readStep();
	void writeStep();
};
class ProjectFindActorsThreadedTask : ThreadedTask
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
	HaulSubproject* haulSubproject;
	ProjectWorker() : haulSubproject(nullptr) { }
};
struct ProjectItemCounts
{
	uint8_t required;
	uint8_t reserved;
	uint8_t delivered;
	ProjectItemCounts(uint8_t r) : required(r), reserved(0), delivered(0) { }
};
class Subproject
{
	Project& m_project;
public:
	std::unordered_set<Actor*> m_workers;
	Subproject(Project& p) : m_project(p) { }
	void commandWorker(Actor& actor) = 0;
	void onComplete();
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
	size_t maxWorkers;
	bool reservationsComplete() const;
	bool deliveriesComplete() const;
protected:
	std::unordered_map<Actor*, ProjectWorker> m_workers;
	std::unordered_set<Actor*> m_waiting;
	std::unordered_set<Actor*> m_making;
	std::list<HaulSubproject<Item>> m_haulItemSubprojects;
	std::list<HaulSubproject<Actor>> m_haulActorSubprojects;
	Project(Block& l, size_t mw) : m_gatherComplete(false), m_location(l), maxWorker(mw);
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
	virtual ~Project;
	virtual uint32_t getDelay() const = 0;
	virtual void onComplete() = 0;
	virtual std::vector<std::pair<ItemQuery, uint32_t> getConsumed() const = 0;
	virtual std::vector<std::pair<ItemQuery, uint32_t> getUnconsumed() const = 0;
	virtual std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const = 0;
	virtual std::vector<std::pair<ActorQuery, uint32_t> getActors() const = 0;
};
