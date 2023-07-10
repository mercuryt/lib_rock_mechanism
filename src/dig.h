#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "util.h"
#include "blockFeature.h"

#include <unordered_map>
#include <vector>

class DigObjective;

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
	bool canDigAt(Block& block) const;
};
class DigObjectiveType : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor);
	std::unique_ptr<Objective> makeFor(Actor& actor);
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
	DigProject(Block& b, const BlockFeatureType* bft) : Project(b, Config::maxNumberOfWorkersForDigProject), blockFeatureType(bft) { }
	void onComplete();
	// What would the total delay time be if we started from scratch now with current workers?
	uint32_t getDelay() const;
};
// Part of HasDigDesignations.
class HasDigDesignationsForPlayer final
{
	Player& m_player;
	std::unordered_map<Block*, DigProject> m_data;
public:
	HasDigDesignationsForPlayer(Player& p) : m_player(p) { }
	void designate(Block& block, const BlockFeatureType* blockFeatureType);
	void remove(Block& block);
	void removeIfExists(Block& block);
	const BlockFeatureType* at(Block& block) const;
	bool empty() const;
};
// To be used by Area.
class HasDigDesignations final
{
	std::unordered_map<Player*, HasDigDesignationsForPlayer> m_data;
public:
	void addPlayer(Player& player);
	void removePlayer(Player& player);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(Player& player, Block& block, const BlockFeatureType* blockFeatureType);
	void remove(Player& player, Block& block);
	void clearAll(Block& block);
	void areThereAnyForPlayer(Player& player) const;
};
