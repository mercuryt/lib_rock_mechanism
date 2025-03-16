#pragma once

#include "../config.h"
#include "../geometry/cuboid.h"
#include "../index.h"
#include "../input.h"
#include "../reservable.h"
#include "../eventSchedule.h"
#include "../project.h"

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

class ConstructProject final : public Project
{
	const BlockFeatureType* m_blockFeatureType;
	MaterialTypeId m_materialType;
	std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const;
	std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const;
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const;
	std::vector<ActorReference> getActors() const;
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
	ConstructProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
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