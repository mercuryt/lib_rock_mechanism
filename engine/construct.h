#pragma once

#include "config.h"
#include "cuboid.h"
#include "index.h"
#include "input.h"
#include "reservable.h"
#include "eventSchedule.h"
#include "project.h"

#include <vector>

class ConstructPathRequest;
struct Faction;
struct BlockFeatureType;
class ConstructObjective;
class ConstructProject;
class HasConstructionDesignationsForFaction;
struct DeserializationMemo;
struct MaterialType;
struct FindPathResult;
class DesignateConstructInputAction final : public InputAction
{
	Cuboid m_cuboid;
	const BlockFeatureType* m_blockFeatureType;
	MaterialTypeId m_materialType;
	DesignateConstructInputAction(InputQueue& inputQueue, Cuboid& cuboid, BlockFeatureType* blockFeatureType, MaterialTypeId materialType) : InputAction(inputQueue), m_cuboid(cuboid), m_blockFeatureType(blockFeatureType), m_materialType(materialType) { }
	void execute();
};
class UndesignateConstructInputAction final : public InputAction
{
	Cuboid m_cuboid;
	UndesignateConstructInputAction(InputQueue& inputQueue, Cuboid& cuboid) : InputAction(inputQueue), m_cuboid(cuboid) { }
	void execute();
};
class ConstructProject final : public Project
{
	const BlockFeatureType* m_blockFeatureType;
	MaterialTypeId m_materialType;
	std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const;
	std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const;
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const;
	std::vector<std::pair<ActorQuery, Quantity>> getActors() const;
	uint32_t getWorkerConstructScore(ActorIndex actor) const;
	Step getDuration() const;
	void onComplete();
	void onCancel();
	void onDelay();
	void offDelay();
public:
	// BlockFeatureType can be null, meaning the block is to be filled with a constructed wall.
	ConstructProject(FactionId faction, Area& a, BlockIndex b, const BlockFeatureType* bft, MaterialTypeId mt, std::unique_ptr<DishonorCallback> dishonorCallback) : 
		Project(faction, a, b, Config::maxNumberOfWorkersForConstructionProject, std::move(dishonorCallback)), m_blockFeatureType(bft), m_materialType(mt) { }
	ConstructProject(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	// What would the total delay time be if we started from scratch now with current workers?
	friend class HasConstructionDesignationsForFaction;
	// For testing.
	[[nodiscard, maybe_unused]] MaterialTypeId getMaterialType() const { return m_materialType; }
};
struct ConstructionLocationDishonorCallback final : public DishonorCallback
{
	FactionId m_faction;
	Area& m_area;
	BlockIndex m_location;
	ConstructionLocationDishonorCallback(FactionId f, Area& a, BlockIndex l) : m_faction(f), m_area(a), m_location(l) { }
	ConstructionLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute([[maybe_unused]] Quantity oldCount, [[maybe_unused]] Quantity newCount);
};
class HasConstructionDesignationsForFaction final
{
	FactionId m_faction;
	//TODO: More then one construct project targeting a given block should be able to exist simultaniously.
	SmallMapStable<BlockIndex, ConstructProject> m_data;
public:
	HasConstructionDesignationsForFaction(FactionId p) : m_faction(p) { }
	HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, FactionId faction);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	// If blockFeatureType is null then construct a wall rather then a feature.
	void designate(Area& area, BlockIndex block, const BlockFeatureType* blockFeatureType, MaterialTypeId materialType);
	void undesignate(BlockIndex block);
	void remove(Area& area, BlockIndex block);
	void removeIfExists(Area& area, BlockIndex block);
	bool contains(BlockIndex block) const;
	const BlockFeatureType* getForBlock(BlockIndex block) const;
	bool empty() const;
	friend class AreaHasConstructionDesignations;
};
// To be used by Area.
class AreaHasConstructionDesignations final
{
	Area& m_area;
	//TODO: Why does this have to be stable?
	SmallMapStable<FactionId, HasConstructionDesignationsForFaction> m_data;
public:
	AreaHasConstructionDesignations(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(FactionId faction);
	void removeFaction(FactionId faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(FactionId faction, BlockIndex block, const BlockFeatureType* blockFeatureType, MaterialTypeId materialType);
	void undesignate(FactionId faction, BlockIndex block);
	void remove(FactionId faction, BlockIndex block);
	void clearAll(BlockIndex block);
	void clearReservations();
	bool areThereAnyForFaction(FactionId faction) const;
	bool contains(FactionId faction, BlockIndex block) const;
	ConstructProject& getProject(FactionId faction, BlockIndex block);
	HasConstructionDesignationsForFaction& getForFaction(FactionId faction) { return m_data[faction]; }
};
