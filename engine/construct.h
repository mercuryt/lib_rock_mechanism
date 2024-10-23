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
	DesignateConstructInputAction(InputQueue& inputQueue, Cuboid& cuboid, const BlockFeatureType* blockFeatureType, const MaterialTypeId& materialType) : InputAction(inputQueue), m_cuboid(cuboid), m_blockFeatureType(blockFeatureType), m_materialType(materialType) { }
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
	uint32_t getWorkerConstructScore(const ActorIndex& actor) const;
	Step getDuration() const;
	void onComplete();
	void onCancel();
	void onDelay();
	void offDelay();
public:
	// BlockFeatureType can be null, meaning the block is to be filled with a constructed wall.
	ConstructProject(const FactionId& faction, Area& a, const BlockIndex& b, const BlockFeatureType* bft, const MaterialTypeId& mt, std::unique_ptr<DishonorCallback> dishonorCallback) : 
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
	ConstructionLocationDishonorCallback(const FactionId& f, Area& a, const BlockIndex& l) : m_faction(f), m_area(a), m_location(l) { }
	ConstructionLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(const Quantity& oldCount, const Quantity& newCount);
};
class HasConstructionDesignationsForFaction final
{
	FactionId m_faction;
	//TODO: More then one construct project targeting a given block should be able to exist simultaniously.
	SmallMapStable<BlockIndex, ConstructProject> m_data;
public:
	HasConstructionDesignationsForFaction(const FactionId& p) : m_faction(p) { }
	HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	// If blockFeatureType is null then construct a wall rather then a feature.
	void designate(Area& area, const BlockIndex& block, const BlockFeatureType* blockFeatureType, const MaterialTypeId& materialType);
	void undesignate(const BlockIndex& block);
	void remove(Area& area, const BlockIndex& block);
	void removeIfExists(Area& area, const BlockIndex& block);
	bool contains(const BlockIndex& block) const;
	const BlockFeatureType* getForBlock(const BlockIndex& block) const;
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
	void addFaction(const FactionId& faction);
	void removeFaction(const FactionId& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(const FactionId& faction, const BlockIndex& block, const BlockFeatureType* blockFeatureType, const MaterialTypeId& materialType);
	void undesignate(const FactionId& faction, const BlockIndex& block);
	void remove(const FactionId& faction, const BlockIndex& block);
	void clearAll(const BlockIndex& block);
	void clearReservations();
	bool areThereAnyForFaction(const FactionId& faction) const;
	bool contains(const FactionId& faction, const BlockIndex& block) const;
	ConstructProject& getProject(const FactionId& faction, const BlockIndex& block);
	HasConstructionDesignationsForFaction& getForFaction(const FactionId& faction) { return m_data[faction]; }
};
