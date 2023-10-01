// Send specific actors to haul a specific item to a specific place.
#pragma once
#include "project.h"
#include "objective.h"

class Item;
class Actor;
class TargetedHaulProject final : public Project
{
	Item& m_item;
public:
	TargetedHaulProject(const Faction* f, Block& l, Item& i) : Project(f, l, 4), m_item(i) { }
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const { return {}; }
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const { return {{m_item, 1}}; }
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const { return {}; }
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const { return {}; }
	void onComplete();
	Step getDelay() const { return Config::addToStockPileDelaySteps; }
};
class TargetedHaulObjective final : public Objective
{
	Item& m_item;
	Actor& m_actor;
	TargetedHaulProject& m_project;
public:
	TargetedHaulObjective(Item& i, Actor& a, TargetedHaulProject& p) : Objective(Config::targetedHaulPriority), m_item(i), m_actor(a), m_project(p)
	{
		m_project.addWorker(m_actor, *this);
       	}
	void execute() { m_project.commandWorker(m_actor); }
	void cancel() { m_project.removeWorker(m_actor); }
	void delay() { m_project.removeWorker(m_actor); }
	ObjectiveId getObjectiveId() const { return ObjectiveId::Haul; }
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
