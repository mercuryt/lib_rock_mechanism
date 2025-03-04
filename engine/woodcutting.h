#pragma once

#include "geometry/cuboid.h"
#include "index.h"
#include "input.h"
#include "objective.h"
#include "project.h"
#include "path/pathRequest.h"
#include "path/terrainFacade.h"
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
class WoodCuttingProject final : public Project
{
	void onDelay();
	void offDelay();
	void onComplete();
	void onCancel();
	std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const;
	std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const;
	std::vector<ActorReference> getActors() const;
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const;
	static uint32_t getWorkerWoodCuttingScore(Area& area, ActorIndex actor);
	// What would the total delay time be if we started from scratch now with current workers?
public:
	// BlockFeatureType can be null, meaning the block is to be fully excavated.
	WoodCuttingProject(FactionId faction, Area& area, const BlockIndex& block, std::unique_ptr<DishonorCallback> locationDishonorCallback) :
		Project(faction, area, block, Config::maxNumberOfWorkersForWoodCuttingProject, std::move(locationDishonorCallback)) { }
	WoodCuttingProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	// No toJson is needed here.
	Step getDuration() const;
	friend class HasWoodCuttingDesignationsForFaction;
};
struct WoodCuttingLocationDishonorCallback final : public DishonorCallback
{
	FactionId m_faction;
	Area& m_area;
	BlockIndex m_location;
	WoodCuttingLocationDishonorCallback(FactionId f, Area& a, const BlockIndex& l) : m_faction(f), m_area(a), m_location(l) { }
	WoodCuttingLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(const Quantity& oldCount, const Quantity& newCount);
	[[nodiscard]] Json toJson() const;
};
// Part of HasWoodCuttingDesignations.
class HasWoodCuttingDesignationsForFaction final
{
	Area& m_area;
	FactionId m_faction;
	SmallMapStable<BlockIndex, WoodCuttingProject> m_data;
public:
	HasWoodCuttingDesignationsForFaction(FactionId p, Area& a) : m_area(a), m_faction(p) { }
	HasWoodCuttingDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, FactionId faction);
	Json toJson() const;
	void designate(const BlockIndex& block);
	void undesignate(const BlockIndex& block);
	// To be called by undesignate as well as by WoodCuttingProject::onCancel.
	void remove(const BlockIndex& block);
	void removeIfExists(const BlockIndex& block);
	bool empty() const;
	friend class AreaHasWoodCuttingDesignations;
};
// To be used by Area.
class AreaHasWoodCuttingDesignations final
{
	Area& m_area;
	SmallMapStable<FactionId, HasWoodCuttingDesignationsForFaction> m_data;
public:
	AreaHasWoodCuttingDesignations(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(FactionId faction);
	void removeFaction(FactionId faction);
	// TODO: designate and undesignate should probably take plants rather then blocks as arguments.
	void designate(FactionId faction, const BlockIndex& block);
	void undesignate(FactionId faction, const BlockIndex& block);
	void remove(FactionId faction, const BlockIndex& block);
	void clearAll(const BlockIndex& block);
	void clearReservations();
	bool areThereAnyForFaction(FactionId faction) const;
	bool contains(FactionId faction, const BlockIndex& block) const;
	WoodCuttingProject& getForFactionAndBlock(FactionId faction, const BlockIndex& block);
};