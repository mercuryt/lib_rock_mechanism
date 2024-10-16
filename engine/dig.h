#pragma once

#include "cuboid.h"
//#include "input.h"
#include "reservable.h"
#include "types.h"
#include "eventSchedule.hpp"
#include "project.h"
#include "config.h"

#include <vector>

struct Faction;
class DigPathRequest;
struct BlockFeatureType;
class DigProject;
class HasDigDesignationsForFaction;
struct FindPathResult;
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
class DigProject final : public Project
{
	const BlockFeatureType* m_blockFeatureType;
	void onDelay();
	void offDelay();
	void onComplete();
	void onCancel();
	[[nodiscard]] std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const;
	[[nodiscard]] std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const;
	[[nodiscard]] std::vector<std::pair<ActorQuery, Quantity>> getActors() const;
	[[nodiscard]] std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const;
	[[nodiscard]] static uint32_t getWorkerDigScore(Area& area, ActorIndex actor);
	// What would the total delay time be if we started from scratch now with current workers?
public:
	// BlockFeatureType can be null, meaning the block is to be fully excavated.
	DigProject(FactionId faction, Area& area, BlockIndex block, const BlockFeatureType* bft, std::unique_ptr<DishonorCallback> locationDishonorCallback) : 
		Project(faction, area, block, Config::maxNumberOfWorkersForDigProject, std::move(locationDishonorCallback)), m_blockFeatureType(bft) { }
	DigProject(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Step getDuration() const;
	[[nodiscard]] Json toJson() const;
	friend class HasDigDesignationsForFaction;
};
struct DigLocationDishonorCallback final : public DishonorCallback
{
	FactionId m_faction;
	Area& m_area;
	BlockIndex m_location;
	DigLocationDishonorCallback(FactionId f, Area& a, BlockIndex l) : m_faction(f), m_area(a), m_location(l) { }
	DigLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Quantity oldCount, Quantity newCount);
	[[nodiscard]] Json toJson() const;
};
// Part of HasDigDesignations.
class HasDigDesignationsForFaction final
{
	FactionId m_faction;
	SmallMapStable<BlockIndex, DigProject> m_data;
public:
	HasDigDesignationsForFaction(FactionId p) : m_faction(p) { }
	HasDigDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, FactionId faction);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	void designate(Area& area, BlockIndex block, const BlockFeatureType* blockFeatureType);
	void undesignate(BlockIndex block);
	// To be called by undesignate as well as by DigProject::onCancel.
	void remove(Area& area, BlockIndex block);
	void removeIfExists(Area& area, BlockIndex block);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] const BlockFeatureType* getForBlock(BlockIndex block) const;
	[[nodiscard]] bool empty() const;
	friend class AreaHasDigDesignations;
};
// To be used by Area.
class AreaHasDigDesignations final
{
	Area& m_area;
	//TODO: Why must this be stable?
	SmallMapStable<FactionId, HasDigDesignationsForFaction> m_data;
public:
	AreaHasDigDesignations(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	void addFaction(FactionId faction);
	void removeFaction(FactionId faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(FactionId faction, BlockIndex block, const BlockFeatureType* blockFeatureType);
	void undesignate(FactionId faction, BlockIndex block);
	void remove(FactionId faction, BlockIndex block);
	void clearAll(BlockIndex block);
	void clearReservations();
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool areThereAnyForFaction(FactionId faction) const;
	[[nodiscard]] bool contains(FactionId faction, BlockIndex block) const { return m_data[faction].m_data.contains(block); }
	[[nodiscard]] DigProject& getForFactionAndBlock(FactionId faction, BlockIndex block);
};
