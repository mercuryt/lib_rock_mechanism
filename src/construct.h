#pragma once

#include "config.h"
#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "project.h"

#include <unordered_map>
#include <vector>

class ConstructObjective;
class Block;
struct Faction;
class BlockFeatureType;

class ConstructThreadedTask final : public ThreadedTask
{
	ConstructObjective& m_constructObjective;
	std::vector<Block*> m_result;
public:
	ConstructThreadedTask(ConstructObjective& co) : m_constructObjective(co) { }
	void readStep();
	void writeStep();
	void clearReferences();
};
class ConstructObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<ConstructThreadedTask> m_constructThreadedTask;
	Project* m_project;
public:
	ConstructObjective(Actor& a) : Objective(Config::constructObjectivePriority), m_actor(a), m_project(nullptr) { }
	void execute();
	void cancel();
	bool canConstructAt(Block& block) const;
	Block* selectAdjacentProject(Block& block) const;
	friend class ConstructThreadedTask;
};
class ConstructObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
};
class ConstructProject final : public Project
{
	const BlockFeatureType* m_blockFeatureType;
	const MaterialType& m_materialType;
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	uint32_t getWorkerConstructScore(Actor& actor) const;
public:
	// BlockFeatureType can be null, meaning the block is to be filled with a constructed wall.
	ConstructProject(const Faction& faction, Block& b, const BlockFeatureType* bft, const MaterialType& mt) : Project(faction, b, Config::maxNumberOfWorkersForConstructionProject), m_blockFeatureType(bft), m_materialType(mt) { }
	void onComplete();
	// What would the total delay time be if we started from scratch now with current workers?
	Step getDelay() const;
	friend class HasConstructionDesignationsForFaction;
};
class HasConstructionDesignationsForFaction final
{
	const Faction& m_faction;
	std::unordered_map<Block*, ConstructProject> m_data;
public:
	HasConstructionDesignationsForFaction(const Faction& p) : m_faction(p) { }
	// If blockFeatureType is null then construct a wall rather then a feature.
	void designate(Block& block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType);
	void remove(Block& block);
	void removeIfExists(Block& block);
	bool contains(const Block& block) const;
	const BlockFeatureType* at(const Block& block) const;
	bool empty() const;
	friend class HasConstructionDesignations;
};
// To be used by Area.
class HasConstructionDesignations final
{
	std::unordered_map<const Faction*, HasConstructionDesignationsForFaction> m_data;
public:
	void addFaction(const Faction& faction);
	void removeFaction(const Faction& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(const Faction& faction, Block& block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType);
	void remove(const Faction& faction, Block& block);
	void clearAll(Block& block);
	bool areThereAnyForFaction(const Faction& faction) const;
	ConstructProject& getProject(const Faction& faction, Block& block);
};
