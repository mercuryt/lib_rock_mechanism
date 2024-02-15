#pragma once

#include "cuboid.h"
#include "deserializationMemo.h"
#include "input.h"
#include "reservable.h"
#include "types.h"
#include "objective.h"
#include "threadedTask.hpp"
#include "eventSchedule.hpp"
#include "project.h"
#include "config.h"
#include "findsPath.h"

#include <unordered_map>
#include <vector>

struct Faction;
class DigThreadedTask;
struct BlockFeatureType;
class DigProject;
class HasDigDesignationsForFaction;
class DesignateDigInputAction final : public InputAction
{
	Cuboid m_cuboid;
	BlockFeatureType* m_blockFeatureType;
	DesignateDigInputAction(InputQueue& inputQueue, Cuboid& cuboid, BlockFeatureType* blockFeatureType) : InputAction(inputQueue), m_cuboid(cuboid), m_blockFeatureType(blockFeatureType) { }
	void execute();
};
class UndesignateDigInputAction final : public InputAction
{
	Cuboid m_cuboid;
	UndesignateDigInputAction(InputQueue& inputQueue, Cuboid& cuboid) : InputAction(inputQueue), m_cuboid(cuboid) { }
	void execute();
};
class DigObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Dig; }
	DigObjectiveType() = default;
	DigObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class DigObjective final : public Objective
{
	HasThreadedTask<DigThreadedTask> m_digThreadedTask;
	Project* m_project;
public:
	DigObjective(Actor& a);
	DigObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	void joinProject(DigProject& project);
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Dig; }
	DigProject* getJoinableProjectAt(const Block& block);
	std::string name() const { return "dig"; }
	friend class DigThreadedTask;
	friend class DigProject;
	//For testing.
	Project* getProject() { return m_project; }
};
// Find a place to dig.
class DigThreadedTask final : public ThreadedTask
{
	DigObjective& m_digObjective;
	// Result is the block which will be the actors location while doing the digging.
	FindsPath m_findsPath;
public:
	DigThreadedTask(DigObjective& digObjective);
	void readStep();
	void writeStep();
	void clearReferences();
};
class DigProject final : public Project
{
	const BlockFeatureType* m_blockFeatureType;
	void onDelay();
	void offDelay();
	void onComplete();
	void onCancel();
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	static uint32_t getWorkerDigScore(Actor& actor);
	// What would the total delay time be if we started from scratch now with current workers?
public:
	// BlockFeatureType can be null, meaning the block is to be fully excavated.
	DigProject(const Faction* faction, Block& block, const BlockFeatureType* bft, std::unique_ptr<DishonorCallback> locationDishonorCallback) : 
		Project(faction, block, Config::maxNumberOfWorkersForDigProject, std::move(locationDishonorCallback)), m_blockFeatureType(bft) { }
	DigProject(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	Step getDuration() const;
	friend class HasDigDesignationsForFaction;
};
struct DigLocationDishonorCallback final : public DishonorCallback
{
	const Faction& m_faction;
	Block& m_location;
	DigLocationDishonorCallback(const Faction& f, Block& l) : m_faction(f), m_location(l) { }
	DigLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(uint32_t oldCount, uint32_t newCount);
};
// Part of HasDigDesignations.
class HasDigDesignationsForFaction final
{
	const Faction& m_faction;
	std::unordered_map<Block*, DigProject> m_data;
public:
	HasDigDesignationsForFaction(const Faction& p) : m_faction(p) { }
	HasDigDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const Faction& faction);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void designate(Block& block, const BlockFeatureType* blockFeatureType);
	void undesignate(Block& block);
	// To be called by undesignate as well as by DigProject::onCancel.
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
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(const Faction& faction);
	void removeFaction(const Faction& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(const Faction& faction, Block& block, const BlockFeatureType* blockFeatureType);
	void undesignate(const Faction& faction, Block& block);
	void remove(const Faction& faction, Block& block);
	void clearAll(Block& block);
	bool areThereAnyForFaction(const Faction& faction) const;
	bool contains(const Faction& faction, const Block& block) const { return m_data.at(&faction).m_data.contains(const_cast<Block*>(&block)); }
	DigProject& at(const Faction& faction, const Block& block);
};
