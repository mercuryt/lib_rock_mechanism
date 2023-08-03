#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "project.h"
#include "config.h"

#include <unordered_map>
#include <vector>

struct Faction;
class DigObjective;
class BlockFeatureType;

// Find a place to dig.
class DigThreadedTask final : public ThreadedTask
{
	DigObjective& m_digObjective;
	std::vector<Block*> m_result;
public:
	DigThreadedTask(DigObjective& ho) : m_digObjective(ho) {}
	void readStep();
	void writeStep();
};
class DigObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<DigThreadedTask> m_digThrededTask;
	Project* m_project;
public:
	DigObjective(Actor& a) : Objective(Config::digObjectivePriority), m_actor(a), m_project(nullptr) { }
	void execute();
	void cancel();
	bool canDigAt(Block& block) const;
	friend class DigThreadedTask;
};
class DigObjectiveType : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
};
class DigProject final : public Project
{
	const BlockFeatureType* blockFeatureType;
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	static uint32_t getWorkerDigScore(Actor& actor);
public:
	// BlockFeatureType can be null, meaning the block is to be fully excavated.
	DigProject(Faction& faction, Block& block, const BlockFeatureType* bft) : Project(faction, block, Config::maxNumberOfWorkersForDigProject), blockFeatureType(bft) { }
	void onComplete();
	// What would the total delay time be if we started from scratch now with current workers?
	uint32_t getDelay() const;
	friend class HasDigDesignationsForFaction;
};
// Part of HasDigDesignations.
class HasDigDesignationsForFaction final
{
	Faction& m_faction;
	std::unordered_map<Block*, DigProject> m_data;
public:
	HasDigDesignationsForFaction(Faction& p) : m_faction(p) { }
	void designate(Block& block, const BlockFeatureType* blockFeatureType);
	void remove(Block& block);
	void removeIfExists(Block& block);
	const BlockFeatureType* at(Block& block) const;
	bool empty() const;
	friend class HasDigDesignations;
};
// To be used by Area.
class HasDigDesignations final
{
	std::unordered_map<Faction*, HasDigDesignationsForFaction> m_data;
public:
	void addFaction(Faction& faction);
	void removeFaction(Faction& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(Faction& faction, Block& block, const BlockFeatureType* blockFeatureType);
	void remove(Faction& faction, Block& block);
	void clearAll(Block& block);
	bool areThereAnyForFaction(Faction& faction) const;
	bool contains(Faction& faction, Block& block) const { return m_data.at(&faction).m_data.contains(&block); }
	DigProject& at(Faction& faction, Block& block) { return m_data.at(&faction).m_data.at(&block); }
};
