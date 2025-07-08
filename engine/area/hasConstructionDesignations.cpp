#include "hasConstructionDesignations.h"
#include "hasSpaceDesignations.h"
#include "area.h"
#include "../space/space.h"

HasConstructionDesignationsForFaction::HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction, Area& area) :
	m_faction(faction)
{
	for(const Json& pair : data["projects"])
	{
		Point3D point = pair[0].get<Point3D>();
		m_data.emplace(point, pair[1], deserializationMemo, area);
	}
}
void HasConstructionDesignationsForFaction::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data["projects"])
	{
		Point3D point = pair[0].get<Point3D>();
		m_data[point].loadWorkers(pair[1], deserializationMemo);
	}
}
Json HasConstructionDesignationsForFaction::toJson() const
{
	Json projects;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second->toJson()};
		projects.push_back(jsonPair);
	}
	return {{"projects", projects}};
}
// If pointFeatureType is null then construct a wall rather then a feature.
void HasConstructionDesignationsForFaction::designate(Area& area, const Point3D& point, const PointFeatureTypeId pointFeatureType, const MaterialTypeId& materialType)
{
	assert(!contains(point));
	Space& space =  area.getSpace();
	space.designation_set(point, m_faction, SpaceDesignation::Construct);
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<ConstructionLocationDishonorCallback>(m_faction, area, point);
	m_data.emplace(point, m_faction, area, point, pointFeatureType, materialType, std::move(locationDishonorCallback));
}
void HasConstructionDesignationsForFaction::undesignate(const Point3D& point)
{
	assert(m_data.contains(point));
	ConstructProject& project = m_data[point];
	project.cancel();
}
void HasConstructionDesignationsForFaction::remove(Area& area, const Point3D& point)
{
	assert(contains(point));
	 area.getSpace().designation_unset(point, m_faction, SpaceDesignation::Construct);
	m_data.erase(point);
}
void AreaHasConstructionDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second->m_data)
			pair2.second->clearReservations();
}
void HasConstructionDesignationsForFaction::removeIfExists(Area& area, const Point3D& point)
{
	if(m_data.contains(point))
		remove(area, point);
}
bool HasConstructionDesignationsForFaction::contains(const Point3D& point) const { return m_data.contains(point); }
PointFeatureTypeId HasConstructionDesignationsForFaction::getForPoint(const Point3D& point) const { return m_data[point].m_pointFeatureType; }
bool HasConstructionDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void AreaHasConstructionDesignations::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.emplace(faction, pair[1], deserializationMemo, faction, area);
	}
}
void AreaHasConstructionDesignations::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data[faction].loadWorkers(pair[1], deserializationMemo);
	}

}
Json AreaHasConstructionDesignations::toJson() const
{
	Json data = Json::array();
	for(auto& pair : m_data)
	{
		Json jsonPair;
		jsonPair[0] = pair.first;
		jsonPair[1] = pair.second->toJson();
		data.push_back(jsonPair);
	}
	return data;
}
void AreaHasConstructionDesignations::addFaction(const FactionId& faction)
{
	assert(!m_data.contains(faction));
	m_data.emplace(faction, faction);
}
void AreaHasConstructionDesignations::removeFaction(const FactionId& faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
void AreaHasConstructionDesignations::designate(const FactionId& faction, const Point3D& point, const PointFeatureTypeId pointFeatureType, const MaterialTypeId& materialType)
{
	if(!m_data.contains(faction))
		addFaction(faction);
	m_data[faction].designate(m_area, point, pointFeatureType, materialType);
}
void AreaHasConstructionDesignations::undesignate(const FactionId& faction, const Point3D& point)
{
	m_data[faction].undesignate(point);
}
void AreaHasConstructionDesignations::remove(const FactionId& faction, const Point3D& point)
{
	assert(m_data.contains(faction));
	m_data[faction].remove(m_area, point);
}
void AreaHasConstructionDesignations::clearAll(const Point3D& point)
{
	for(auto& pair : m_data)
		pair.second->removeIfExists(m_area, point);
}
bool AreaHasConstructionDesignations::areThereAnyForFaction(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return !m_data[faction].empty();
}
bool AreaHasConstructionDesignations::contains(const FactionId& faction, const Point3D& point) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].contains(point);
}
ConstructProject& AreaHasConstructionDesignations::getProject(const FactionId& faction, const Point3D& point) { return m_data[faction].m_data[point]; }