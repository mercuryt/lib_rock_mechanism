#pragma once

#include "util.h"
#include "config.h"
#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "project.h"

#include <unordered_map>
#include <vector>

class ConstructObjective;
class Block;
class Faction;
class BlockFeatureType;

class ConstructThreadedTask final : public ThreadedTask
{
	ConstructObjective& m_constructObjective;
	std::vector<Block*> m_result;
public:
	ConstructThreadedTask(ConstructObjective& ho) : m_constructObjective(ho) {}
	void readStep();
	void writeStep();
};
class ConstructObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<ConstructThreadedTask> m_constructThreadedTask;
	Project* m_project;
public:
	ConstructObjective(Actor& a) : Objective(Config::constructObjectivePriority), m_actor(a), m_project(nullptr) { }
	void execute();
	bool canConstructAt(Block& block) const;
	Block* selectAdjacentProject(Block& block) const;
	friend class ConstructThreadedTask;
};
class ConstructObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor);
	std::unique_ptr<Objective> makeFor(Actor& actor);
};
class ConstructProject final : public Project
{
	const BlockFeatureType* blockFeatureType;
	const MaterialType& materialType;
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>>& getByproducts() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	uint32_t getWorkerConstructScore(Actor& actor) const;
public:
	// BlockFeatureType can be null, meaning the block is to be filled with a constructed wall.
	ConstructProject(Block& b, const BlockFeatureType* bft, const MaterialType& mt) : Project(b, Config::maxNumberOfWorkersForConstructionProject), blockFeatureType(bft), materialType(mt) { }
	void onComplete();
	// What would the total delay time be if we started from scratch now with current workers?
	uint32_t getDelay() const;
	friend class HasConstructionDesignationsForFaction;
};
class HasConstructionDesignationsForFaction final
{
	Faction& m_faction;
	std::unordered_map<Block*, ConstructProject> m_data;
public:
	HasConstructionDesignationsForFaction(Faction& p) : m_faction(p) { }
	// If blockFeatureType is null then construct a wall rather then a feature.
	void designate(Block& block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType);
	void remove(Block& block);
	void removeIfExists(Block& block);
	bool contains(Block& block) const;
	const BlockFeatureType* at(Block& block) const;
	bool empty() const;
	friend class HasConstructionDesignations;
};
// To be used by Area.
class HasConstructionDesignations final
{
	std::unordered_map<Faction*, HasConstructionDesignationsForFaction> m_data;
public:
	void addFaction(Faction& faction);
	void removeFaction(Faction& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(Faction& faction, Block& block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType);
	void remove(Faction& faction, Block& block);
	void clearAll(Block& block);
	bool areThereAnyForFaction(Faction& faction) const;
	ConstructProject& at(Faction& faction, Block& block) const;
};
