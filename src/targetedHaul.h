// Send specific actors to haul a specific item to a specific place.
#pragma once
#include "project.h"
#include "objective.h"

class Item;
class Actor;
class TargetedHaulProject final : public Project
{
	Item& m_item;
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const { return {}; }
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const { return {{m_item, 1}}; }
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const { return {}; }
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const { return {}; }
	void onComplete();
	void onDelivered(HasShape& delivered) { delivered.setLocation(m_location); }
	// Most projects which are directly created by the user ( dig, construct ) wait a while and then retry if they fail.
	// Despite being directly created it doesn't make sense to retry targeted hauling, so instead we cancel it with a log message.
	// TODO: log message.
	void onDelay() { cancel(); }
	void offDelay() { assert(false); }
	Step getDuration() const { return Config::addToStockPileDelaySteps; }
public:
	TargetedHaulProject(const Faction* f, Block& l, Item& i) : Project(f, l, 4), m_item(i) { }
};
class TargetedHaulObjective final : public Objective
{
	Actor& m_actor;
	TargetedHaulProject& m_project;
public:
	TargetedHaulObjective(Actor& a, TargetedHaulProject& p) : Objective(Config::targetedHaulPriority), m_actor(a), m_project(p)
	{
		m_project.addWorkerCandidate(m_actor, *this);
       	}
	void execute() { m_project.commandWorker(m_actor); }
	void cancel() { m_project.removeWorker(m_actor); }
	void delay() { }
	void reset() { m_actor.m_canReserve.clearAll(); }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Haul; }
	std::string name() const { return "haul"; }
};

class AreaHasTargetedHauling
{
	std::list<TargetedHaulProject> m_projects;
public:
	TargetedHaulProject& begin(std::vector<Actor*> actors, Item& item, Block& destination);
	void cancel(TargetedHaulProject& project);
	void complete(TargetedHaulProject& project);
};
