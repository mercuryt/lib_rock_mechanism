#pragma once

#include "config.h"
#include "cuboid.h"
#include "input.h"
#include "reservable.h"
#include "eventSchedule.h"
#include "project.h"

#include <unordered_map>
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
	const MaterialType& m_materialType;
	DesignateConstructInputAction(InputQueue& inputQueue, Cuboid& cuboid, BlockFeatureType* blockFeatureType, const MaterialType& materialType) : InputAction(inputQueue), m_cuboid(cuboid), m_blockFeatureType(blockFeatureType), m_materialType(materialType) { }
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
	const MaterialType& m_materialType;
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	uint32_t getWorkerConstructScore(ActorIndex actor) const;
	Step getDuration() const;
	void onComplete();
	void onCancel();
	void onDelay();
	void offDelay();
public:
	// BlockFeatureType can be null, meaning the block is to be filled with a constructed wall.
	ConstructProject(Faction& faction, Area& a, BlockIndex b, const BlockFeatureType* bft, const MaterialType& mt, std::unique_ptr<DishonorCallback> dishonorCallback) : 
		Project(faction, a, b, Config::maxNumberOfWorkersForConstructionProject, std::move(dishonorCallback)), m_blockFeatureType(bft), m_materialType(mt) { }
	ConstructProject(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	// What would the total delay time be if we started from scratch now with current workers?
	friend class HasConstructionDesignationsForFaction;
	// For testing.
	[[nodiscard, maybe_unused]] const MaterialType& getMaterialType() const { return m_materialType; }
};
struct ConstructionLocationDishonorCallback final : public DishonorCallback
{
	Faction& m_faction;
	Area& m_area;
	BlockIndex m_location;
	ConstructionLocationDishonorCallback(Faction& f, Area& a, BlockIndex l) : m_faction(f), m_area(a), m_location(l) { }
	ConstructionLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute([[maybe_unused]] uint32_t oldCount, [[maybe_unused]] uint32_t newCount);
};
class HasConstructionDesignationsForFaction final
{
	Area& m_area;
	Faction& m_faction;
	//TODO: More then one construct project targeting a given block should be able to exist simultaniously.
	std::unordered_map<BlockIndex, ConstructProject> m_data;
public:
	HasConstructionDesignationsForFaction(Faction& p, Area& a) : m_area(a), m_faction(p) { }
	HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Faction& faction);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	// If blockFeatureType is null then construct a wall rather then a feature.
	void designate(BlockIndex block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType);
	void undesignate(BlockIndex block);
	void remove(BlockIndex block);
	void removeIfExists(BlockIndex block);
	bool contains(BlockIndex block) const;
	const BlockFeatureType* at(BlockIndex block) const;
	bool empty() const;
	friend class AreaHasConstructionDesignations;
};
// To be used by Area.
class AreaHasConstructionDesignations final
{
	Area& m_area;
	std::unordered_map<Faction*, HasConstructionDesignationsForFaction> m_data;
public:
	AreaHasConstructionDesignations(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(Faction& faction);
	void removeFaction(Faction& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(Faction& faction, BlockIndex block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType);
	void undesignate(Faction& faction, BlockIndex block);
	void remove(Faction& faction, BlockIndex block);
	void clearAll(BlockIndex block);
	void clearReservations();
	bool areThereAnyForFaction(Faction& faction) const;
	bool contains(Faction& faction, BlockIndex block) const;
	ConstructProject& getProject(Faction& faction, BlockIndex block);
	HasConstructionDesignationsForFaction& at(Faction& faction) { return m_data.at(&faction); }
};
