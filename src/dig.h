#pragma once

#include "objective.h"
#include "threadedTask.hpp"
#include "eventSchedule.hpp"
#include "project.h"
#include "config.h"

#include <unordered_map>
#include <vector>

struct Faction;
class DigThreadedTask;
struct BlockFeatureType;

class DigObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<DigThreadedTask> m_digThrededTask;
	Project* m_project;
public:
	DigObjective(Actor& a);
	void execute();
	void cancel();
	std::string name() { return "dig"; }
	bool canDigAt(Block& block) const;
	friend class DigThreadedTask;
};
// Find a place to dig.
class DigThreadedTask final : public ThreadedTask
{
	DigObjective& m_digObjective;
	std::vector<Block*> m_result;
public:
	DigThreadedTask(DigObjective& digObjective);
	void readStep();
	void writeStep();
	void clearReferences();
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
	DigProject(const Faction& faction, Block& block, const BlockFeatureType* bft) : Project(faction, block, Config::maxNumberOfWorkersForDigProject), blockFeatureType(bft) { }
	void onComplete();
	// What would the total delay time be if we started from scratch now with current workers?
	Step getDelay() const;
	friend class HasDigDesignationsForFaction;
};
// Part of HasDigDesignations.
class HasDigDesignationsForFaction final
{
	const Faction& m_faction;
	std::unordered_map<Block*, DigProject> m_data;
public:
	HasDigDesignationsForFaction(const Faction& p) : m_faction(p) { }
	void designate(Block& block, const BlockFeatureType* blockFeatureType);
	void remove(Block& block);
	void removeIfExists(Block& block);
	const BlockFeatureType* at(const Block& block) const;
	bool empty() const;
	friend class HasDigDesignations;
};
// To be used by Area.
class HasDigDesignations final
{
	std::unordered_map<const Faction*, HasDigDesignationsForFaction> m_data;
public:
	void addFaction(const Faction& faction);
	void removeFaction(const Faction& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(const Faction& faction, Block& block, const BlockFeatureType* blockFeatureType);
	void remove(const Faction& faction, Block& block);
	void clearAll(Block& block);
	bool areThereAnyForFaction(const Faction& faction) const;
	bool contains(const Faction& faction, const Block& block) const { return m_data.at(&faction).m_data.contains(const_cast<Block*>(&block)); }
	DigProject& at(const Faction& faction, const Block& block) { return m_data.at(&faction).m_data.at(const_cast<Block*>(&block)); }
};
