#pragma once

#include "cuboid.h"
#include "input.h"
#include "objective.h"
#include "project.h"
#include "pathRequest.h"
#include "terrainFacade.h"
#include "types.h"
#include <memory>

class WoodCuttingProject;
class WoodCuttingPathRequest;
/*
// InputAction
class DesignateWoodCuttingInputAction final : public InputAction
{
	Cuboid m_blocks;
public:
	DesignateWoodCuttingInputAction(InputQueue& inputQueue, Cuboid& blocks) : InputAction(inputQueue), m_blocks(blocks) { }
	void execute();
};
class UndesignateWoodCuttingInputAction final : public InputAction
{
	Cuboid m_blocks;
public:
	UndesignateWoodCuttingInputAction(InputQueue& inputQueue, Cuboid& blocks) : InputAction(inputQueue), m_blocks(blocks) { }
	void execute();
};
*/
// ObjectiveType
class WoodCuttingObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area& area, ActorIndex actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::WoodCutting; }
};
// Objective
class WoodCuttingObjective final : public Objective
{
	WoodCuttingProject* m_project;
	std::unordered_set<Project*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	WoodCuttingObjective(ActorIndex a);
	WoodCuttingObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(Area& area);
	void cancel(Area& area);
	void reset(Area& area);
	void delay(Area& area);
	void onProjectCannotReserve(Area& area);
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::WoodCutting; }
	[[nodiscard]] std::string name() const { return "woodcutting"; }
	void joinProject(WoodCuttingProject& project);
	WoodCuttingProject* getJoinableProjectAt(Area& area, BlockIndex block);
	friend class WoodCuttingPathRequest;
	friend class WoodCuttingProject;
};
// Find a place to woodCutting.
class WoodCuttingPathRequest final : public PathRequest
{
	WoodCuttingObjective& m_woodCuttingObjective;
	// Result is the block which will be the actors location while doing the woodCuttingging.
public:
	WoodCuttingPathRequest(Area& area, WoodCuttingObjective& woodCuttingObjective);
	void callback(Area& area, FindPathResult& result);
};
class WoodCuttingProject final : public Project
{
	void onDelay();
	void offDelay();
	void onComplete();
	void onCancel();
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	static uint32_t getWorkerWoodCuttingScore(Area& area, ActorIndex actor);
	// What would the total delay time be if we started from scratch now with current workers?
public:
	// BlockFeatureType can be null, meaning the block is to be fully excavated.
	WoodCuttingProject(Faction* faction, Area& area, BlockIndex block, std::unique_ptr<DishonorCallback> locationDishonorCallback) : 
		Project(faction, area, block, Config::maxNumberOfWorkersForWoodCuttingProject, std::move(locationDishonorCallback)) { }
	WoodCuttingProject(const Json& data, DeserializationMemo& deserializationMemo);
	// No toJson is needed here.
	Step getDuration() const;
	friend class HasWoodCuttingDesignationsForFaction;
};
struct WoodCuttingLocationDishonorCallback final : public DishonorCallback
{
	Faction& m_faction;
	Area& m_area;
	BlockIndex m_location;
	WoodCuttingLocationDishonorCallback(Faction& f, Area& a, BlockIndex l) : m_faction(f), m_area(a), m_location(l) { }
	WoodCuttingLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(uint32_t oldCount, uint32_t newCount);
};
// Part of HasWoodCuttingDesignations.
class HasWoodCuttingDesignationsForFaction final
{
	Area& m_area;
	Faction& m_faction;
	std::unordered_map<BlockIndex, WoodCuttingProject> m_data;
public:
	HasWoodCuttingDesignationsForFaction(Faction& p, Area& a) : m_area(a), m_faction(p) { }
	HasWoodCuttingDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Faction& faction);
	Json toJson() const;
	void designate(BlockIndex block);
	void undesignate(BlockIndex block);
	// To be called by undesignate as well as by WoodCuttingProject::onCancel.
	void remove(BlockIndex block);
	void removeIfExists(BlockIndex block);
	bool empty() const;
	friend class AreaHasWoodCuttingDesignations;
};
// To be used by Area.
class AreaHasWoodCuttingDesignations final
{
	Area& m_area;
	std::unordered_map<Faction*, HasWoodCuttingDesignationsForFaction> m_data;
public:
	AreaHasWoodCuttingDesignations(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(Faction& faction);
	void removeFaction(Faction& faction);
	// TODO: designate and undesignate should probably take plants rather then blocks as arguments.
	void designate(Faction& faction, BlockIndex block);
	void undesignate(Faction& faction, BlockIndex block);
	void remove(Faction& faction, BlockIndex block);
	void clearAll(BlockIndex block);
	void clearReservations();
	bool areThereAnyForFaction(Faction& faction) const;
	bool contains(Faction& faction, BlockIndex block) const;
	WoodCuttingProject& at(Faction& faction, BlockIndex block);
};
