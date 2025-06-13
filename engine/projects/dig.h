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
struct BlockFeatureType;
class HasDigDesignationsForFaction;
struct DeserializationMemo;

class DigProject final : public Project
{
	BlockFeatureTypeId m_blockFeatureType;
	void onDelay();
	void offDelay();
	void onComplete();
	void onCancel();
	[[nodiscard]] std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const;
	[[nodiscard]] std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const;
	[[nodiscard]] std::vector<ActorReference> getActors() const;
	[[nodiscard]] std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const;
	[[nodiscard]] static uint32_t getWorkerDigScore(Area& area, ActorIndex actor);
	// What would the total delay time be if we started from scratch now with current workers?
public:
	// BlockFeatureType can be null, meaning the block is to be fully excavated.
	DigProject(const FactionId& faction, Area& area, const BlockIndex& block, const BlockFeatureTypeId bft, std::unique_ptr<DishonorCallback> locationDishonorCallback) :
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