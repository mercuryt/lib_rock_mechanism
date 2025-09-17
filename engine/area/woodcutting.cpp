#include "woodcutting.h"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../plants.h"
#include "../area/area.h"
#include "../pointFeature.h"
#include "../deserializationMemo.h"
#include "../random.h"
#include "../reservable.h"
#include "../path/terrainFacade.h"
#include "../numericTypes/types.h"
#include "../util.h"
#include "../simulation/simulation.h"
#include "../definitions/itemType.h"
#include "../objectives/woodcutting.h"
#include "../projects/woodcutting.h"
#include "../definitions/plantSpecies.h"
#include <memory>
#include <sys/types.h>
WoodCuttingLocationDishonorCallback::WoodCuttingLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) :
	m_faction(data["faction"].get<FactionId>()),
	m_area(deserializationMemo.area(data["area"])),
	m_location(data["location"].get<Point3D>()) { }
Json WoodCuttingLocationDishonorCallback::toJson() const { return Json({{"type", "WoodCuttingLocationDishonorCallback"}, {"faction", m_faction}, {"location", m_location}}); }
void WoodCuttingLocationDishonorCallback::execute([[maybe_unused]] const Quantity& oldCount, [[maybe_unused]] const Quantity& newCount)
{
	m_area.m_hasWoodCuttingDesignations.undesignate(m_faction, m_location);
}
HasWoodCuttingDesignationsForFaction::HasWoodCuttingDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, FactionId faction) :
	m_area(deserializationMemo.area(data["area"])),
	m_faction(faction)
{
	for(const Json& pair : data)
	{
		Point3D point = pair[0].get<Point3D>();
		m_data.emplace(point, pair[1], deserializationMemo, m_area);
	}
}
Json HasWoodCuttingDesignationsForFaction::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second->toJson()};
		data.push_back(jsonPair);
	}
	return data;
}
void HasWoodCuttingDesignationsForFaction::designate(const Point3D& point)
{
	Space& space = m_area.getSpace();
	[[maybe_unused]] Plants& plants = m_area.getPlants();
	assert(!m_data.contains(point));
	assert(space.plant_exists(point));
	assert(PlantSpecies::getIsTree(plants.getSpecies(space.plant_get(point))));
	assert(plants.getPercentGrown(space.plant_get(point)) >= Config::minimumPercentGrowthForWoodCutting);
	space.designation_set(point, m_faction, SpaceDesignation::WoodCutting);
	// To be called when point is no longer a suitable location, for example if it got crushed by a collapse.
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<WoodCuttingLocationDishonorCallback>(m_faction, m_area, point);
	m_data.emplace(point, m_faction, m_area, point, std::move(locationDishonorCallback));
}
void HasWoodCuttingDesignationsForFaction::undesignate(const Point3D& point)
{
	assert(m_data.contains(point));
	WoodCuttingProject& project = m_data[point];
	project.cancel();
}
void HasWoodCuttingDesignationsForFaction::remove(const Point3D& point)
{
	assert(m_data.contains(point));
	m_area.getSpace().designation_unset(point, m_faction, SpaceDesignation::WoodCutting);
	m_data.erase(point);
}
void HasWoodCuttingDesignationsForFaction::removeIfExists(const Point3D& point)
{
	if(m_data.contains(point))
		remove(point);
}
bool HasWoodCuttingDesignationsForFaction::empty() const { return m_data.empty(); }
WoodCuttingProject& HasWoodCuttingDesignationsForFaction::getForPoint(const Point3D& point) { return m_data[point]; }
// To be used by Area.
void AreaHasWoodCuttingDesignations::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.emplace(faction, pair[1], deserializationMemo, faction);
	}
}
Json AreaHasWoodCuttingDesignations::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair;
		jsonPair[0] = pair.first;
		jsonPair[1] = pair.second->toJson();
		data.push_back(jsonPair);
	}
	return data;
}
void AreaHasWoodCuttingDesignations::addFaction(FactionId faction)
{
	assert(!m_data.contains(faction));
	m_data.emplace(faction, faction, m_area);
}
void AreaHasWoodCuttingDesignations::removeFaction(FactionId faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
// If pointFeatureType is null then woodCutting out fully rather then woodCuttingging out a feature.
void AreaHasWoodCuttingDesignations::designate(FactionId faction, const Point3D& point)
{
	if(!m_data.contains(faction))
		addFaction(faction);
	m_data[faction].designate(point);
}
void AreaHasWoodCuttingDesignations::undesignate(FactionId faction, const Point3D& point)
{
	assert(m_data.contains(faction));
	m_data[faction].undesignate(point);
}
void AreaHasWoodCuttingDesignations::remove(FactionId faction, const Point3D& point)
{
	assert(m_data.contains(faction));
	m_data[faction].remove(point);
}
void AreaHasWoodCuttingDesignations::clearAll(const Point3D& point)
{
	for(auto& pair : m_data)
		pair.second->removeIfExists(point);
}
void AreaHasWoodCuttingDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second->m_data)
			pair2.second->clearReservations();
}
bool AreaHasWoodCuttingDesignations::areThereAnyForFaction(FactionId faction) const
{
	if(!m_data.contains(faction))
		return false;
	return !m_data[faction].empty();
}
bool AreaHasWoodCuttingDesignations::contains(FactionId faction, const Point3D& point) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].m_data.contains(point);
}
HasWoodCuttingDesignationsForFaction& AreaHasWoodCuttingDesignations::getForFaction(FactionId faction) { return m_data[faction]; }
const HasWoodCuttingDesignationsForFaction& AreaHasWoodCuttingDesignations::getForFaction(FactionId faction) const { return m_data[faction]; }