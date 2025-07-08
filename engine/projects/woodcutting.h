#pragma once
#include "../project.h"
#include "../items/itemQuery.h"
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include <vector>
class Area;
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
	static uint32_t getWorkerWoodCuttingScore(Area& area, const ActorIndex& actor);
	// What would the total delay time be if we started from scratch now with current workers?
public:
	// PointFeatureType can be null, meaning the point is to be fully excavated.
	WoodCuttingProject(FactionId faction, Area& area, const Point3D& point, std::unique_ptr<DishonorCallback> locationDishonorCallback) :
		Project(faction, area, point, Config::maxNumberOfWorkersForWoodCuttingProject, std::move(locationDishonorCallback)) { }
	WoodCuttingProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	// No toJson is needed here.
	Step getDuration() const;
	friend class HasWoodCuttingDesignationsForFaction;
};