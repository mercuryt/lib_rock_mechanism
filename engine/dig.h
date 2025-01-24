#pragma once

#include "geometry/cuboid.h"
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
	DigProject(const FactionId& faction, Area& area, const BlockIndex& block, const BlockFeatureType* bft, std::unique_ptr<DishonorCallback> locationDishonorCallback) :
		Project(faction, area, block, Config::maxNumberOfWorkersForDigProject, std::move(locationDishonorCallback)), m_blockFeatureType(bft) { }
	DigProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	[[nodiscard]] Step getDuration() const;
	[[nodiscard]] Json toJson() const;
	friend class HasDigDesignationsForFaction;
};
struct DigLocationDishonorCallback final : public DishonorCallback
{
	FactionId m_faction;
	Area& m_area;
	BlockIndex m_location;
	DigLocationDishonorCallback(const FactionId& f, Area& a, const BlockIndex& l) : m_faction(f), m_area(a), m_location(l) { }
	DigLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(const Quantity& oldCount, const Quantity& newCount);
	[[nodiscard]] Json toJson() const;
};
// Part of HasDigDesignations.
class HasDigDesignationsForFaction final
{
	FactionId m_faction;
	SmallMapStable<BlockIndex, DigProject> m_data;
public:
	HasDigDesignationsForFaction(const FactionId& p) : m_faction(p) { }
	HasDigDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction, Area& area);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	void designate(Area& area, const BlockIndex& block, const BlockFeatureType* blockFeatureType);
	void undesignate(const BlockIndex& block);
	// To be called by undesignate as well as by DigProject::onCancel.
	void remove(Area& area, const BlockIndex& block);
	void removeIfExists(Area& area, const BlockIndex& block);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] const BlockFeatureType* getForBlock(const BlockIndex& block) const;
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
	void load(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	void addFaction(const FactionId& faction);
	void removeFaction(const FactionId& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(const FactionId& faction, const BlockIndex& block, const BlockFeatureType* blockFeatureType);
	void undesignate(const FactionId& faction, const BlockIndex& block);
	void remove(const FactionId& faction, const BlockIndex& block);
	void clearAll(const BlockIndex& block);
	void clearReservations();
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool areThereAnyForFaction(const FactionId& faction) const;
	[[nodiscard]] bool contains(const FactionId& faction, const BlockIndex& block) const { return m_data[faction].m_data.contains(block); }
	[[nodiscard]] DigProject& getForFactionAndBlock(const FactionId& faction, const BlockIndex& block);
};
