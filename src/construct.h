#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "util.h"
#include "blockFeature.h"

#include <unordered_map>
#include <vector>

class ConstructThreadedTask : ThreadedTask
{
	ConstructObjective& m_constructObjective;
	std::vector<Block*> m_result;
	ConstructThreadedTask(ConstructObjective& ho) : m_constructObjective(ho) {}
	void readStep();
	void writeStep();
};
class ConstructObjective : Objective
{
	Actor& m_actor;
	HasThreadedTask<ConstructThreadedTask> m_constructThreadedTask;
	Project* m_project;
	Construct(Actor& a) : m_actor(a), m_project(nullptr) { }
	void execute();
	bool canConstructAt(Block& block) const;
};
class ConstructObjectiveType : ObjectiveType
{
	ConstructObjectiveType() : m_name("construct") { }
	bool canBeAssigned(Actor& actor);
	std::unique_ptr<Objective> makeFor(Actor& actor);
};
class ConstructProject : Project
{
	const BlockFeatureType* blockFeatureType;
	const MaterialType& materialType;
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t quantity>> getByproducts() const;
	uint32_t getWorkerConstructScore(Actor& actor) const;
public:
	// BlockFeatureType can be null, meaning the block is to be filled with a constructed wall.
	ConstructDesignation(Block& b, const BlockFeatureType* bft, const MaterialType& mt) : project(b, Config::maxNumberOfWorkersForConstructProject), blockFeatureType(bft), materialType(mt) { }
	void onComplete();
	// What would the total delay time be if we started from scratch now with current workers?
	uint32_t getDelay() const;
};
class HasConstructDesignationsForPlayer
{
	Player& m_player;
	std::unordered_map<Block*, ConstructDesignation> m_data;
public:
	HasConstructDesignationsForPlayer(Player& p) : m_player(p) { }
	// If blockFeatureType is null then construct a wall rather then a feature.
	void designate(Block& block, const BlockFeatureType* blockFeatureType);
	void remove(Block& block);
	void removeIfExists(Block& block);
	bool contains(Block& block) const;
	const BlockFeatureType* at(Block& block) const;
	bool empty() const;
};
// To be used by Area.
class HasConstructDesignations
{
	std::unordered_map<Player*, HasConstructDesignationsForPlayer> m_data;
public:
	void addPlayer(Player& player);
	void removePlayer(Player& player);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(Player& player, Block& block, const BlockFeatureType* blockFeatureType);
	void remove(Player& player, Block& block);
	void clearAll(Block& block);
	void areThereAnyForPlayer(Player& player) const;
};
