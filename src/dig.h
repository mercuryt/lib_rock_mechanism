#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "util.h"
#include "blockFeature.h"

#include <unordered_map>
#include <vector>

// Find a place to dig.
class DigThreadedTask : ThreadedTask
{
	DigObjective& m_digObjective;
	std::vector<Block*> m_result;
	DigThreadedTask(DigObjective& ho) : m_digObjective(ho) {}
	void readStep();
	void writeStep();
};
class DigObjective : Objective
{
	Actor& m_actor;
	HasThreadedTask<DigThreadedTask> m_digThrededTask;
	Project* m_project;
	DigObjective(Actor& a) : m_actor(a), m_project(nullptr) { }
	void execute();
	bool canDigAt(Block& block) const;
};
class DigObjectiveType : ObjectiveType
{
	bool canBeAssigned(Actor& actor);
	std::unique_ptr<Objective> makeFor(Actor& actor);
};
class DigProject : Project
{
	const BlockFeatureType* blockFeatureType;
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed();
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t quantity>> getByproducts() const;
	static uint32_t getWorkerDigScore(Actor& actor) const;
public:
	// BlockFeatureType can be null, meaning the block is to be fully excavated.
	DigProject(Block& b, const BlockFeatureType* bft) : project(b, Config::maxNumberOfWorkersForDigProject), blockFeatureType(bft) { }
	void onComplete();
	// What would the total delay time be if we started from scratch now with current workers?
	uint32_t getDelay() const;
};
// Part of HasDigDesignations.
class HasDigDesignationsForPlayer
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
class HasDigDesignations
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
