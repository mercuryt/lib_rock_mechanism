#pragma once

#include "cuboid.h"
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
#include <unordered_set>
#include <vector>

struct Faction;
class DigThreadedTask;
struct BlockFeatureType;
class DigProject;
class HasDigDesignationsForFaction;
struct DeserializationMemo;
/*
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
*/
class DigObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Actor& actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Actor& actor) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Dig; }
	DigObjectiveType() = default;
	DigObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class DigObjective final : public Objective
{
	HasThreadedTask<DigThreadedTask> m_digThreadedTask;
	Project* m_project = nullptr;
	std::unordered_set<Project*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	DigObjective(Actor& a);
	DigObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute();
	void cancel();
	void delay();
	void reset();
	void onProjectCannotReserve();
	void joinProject(DigProject& project);
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Dig; }
	[[nodiscard]] DigProject* getJoinableProjectAt(BlockIndex block);
	[[nodiscard]] std::string name() const { return "dig"; }
	friend class DigThreadedTask;
	friend class DigProject;
	//For testing.
	[[nodiscard]] Project* getProject() { return m_project; }
	[[nodiscard]] bool hasThreadedTask() const { return m_digThreadedTask.exists(); }
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
	[[nodiscard]] std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	[[nodiscard]] std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	[[nodiscard]] std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	[[nodiscard]] std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	[[nodiscard]] static uint32_t getWorkerDigScore(Actor& actor);
	// What would the total delay time be if we started from scratch now with current workers?
public:
	// BlockFeatureType can be null, meaning the block is to be fully excavated.
	DigProject(Faction* faction, Area& area, BlockIndex block, const BlockFeatureType* bft, std::unique_ptr<DishonorCallback> locationDishonorCallback) : 
		Project(faction, area, block, Config::maxNumberOfWorkersForDigProject, std::move(locationDishonorCallback)), m_blockFeatureType(bft) { }
	DigProject(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Step getDuration() const;
	friend class HasDigDesignationsForFaction;
};
struct DigLocationDishonorCallback final : public DishonorCallback
{
	Faction& m_faction;
	Area& m_area;
	BlockIndex m_location;
	DigLocationDishonorCallback(Faction& f, Area& a, BlockIndex l) : m_faction(f), m_area(a), m_location(l) { }
	DigLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(uint32_t oldCount, uint32_t newCount);
};
// Part of HasDigDesignations.
class HasDigDesignationsForFaction final
{
	Area& m_area;
	Faction& m_faction;
	std::unordered_map<BlockIndex, DigProject> m_data;
public:
	HasDigDesignationsForFaction(Faction& p, Area& a) :m_area(a), m_faction(p) { }
	HasDigDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Faction& faction);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void designate(BlockIndex block, const BlockFeatureType* blockFeatureType);
	void undesignate(BlockIndex block);
	// To be called by undesignate as well as by DigProject::onCancel.
	void remove(BlockIndex block);
	void removeIfExists(BlockIndex block);
	[[nodiscard]] const BlockFeatureType* at(BlockIndex block) const;
	[[nodiscard]] bool empty() const;
	friend class AreaHasDigDesignations;
};
// To be used by Area.
class AreaHasDigDesignations final
{
	Area& m_area;
	std::unordered_map<Faction*, HasDigDesignationsForFaction> m_data;
public:
	AreaHasDigDesignations(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void addFaction(Faction& faction);
	void removeFaction(Faction& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(Faction& faction, BlockIndex block, const BlockFeatureType* blockFeatureType);
	void undesignate(Faction& faction, BlockIndex block);
	void remove(Faction& faction, BlockIndex block);
	void clearAll(BlockIndex block);
	void clearReservations();
	[[nodiscard]] bool areThereAnyForFaction(Faction& faction) const;
	[[nodiscard]] bool contains(Faction& faction, BlockIndex block) const { return m_data.at(&faction).m_data.contains(block); }
	[[nodiscard]] DigProject& at(Faction& faction, BlockIndex block);
};
