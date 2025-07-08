#include "hasDigDesignations.h"
#include "area.h"
#include "../space/space.h"
HasDigDesignationsForFaction::HasDigDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction, Area& area) :
	m_faction(faction)
{
	for(const Json& pair : data["projects"])
	{
		Point3D point = pair[0].get<Point3D>();
		m_data.emplace(point, pair[1], deserializationMemo, area);
	}
}
void HasDigDesignationsForFaction::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data["projects"])
	{
		Point3D point = pair[0].get<Point3D>();
		m_data[point].loadWorkers(pair[1], deserializationMemo);
	}
}
Json HasDigDesignationsForFaction::toJson() const
{
	Json projects;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second->toJson()};
		projects.push_back(jsonPair);
	}
	return {{"projects", projects}};
}
void HasDigDesignationsForFaction::designate(Area& area, const Point3D& point, [[maybe_unused]] const PointFeatureTypeId pointFeatureType)
{
	assert(!m_data.contains(point));
	Space& space =  area.getSpace();
	space.designation_set(point, m_faction, SpaceDesignation::Dig);
	// To be called when point is no longer a suitable location, for example if it got dug out already.
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<DigLocationDishonorCallback>(m_faction, area, point);
	[[maybe_unused]] DigProject& project = m_data.emplace(point, m_faction, area, point, pointFeatureType, std::move(locationDishonorCallback));
	assert(static_cast<DigProject*>(space.project_get(point, m_faction)) == &project);
}
void HasDigDesignationsForFaction::undesignate(const Point3D& point)
{
	assert(m_data.contains(point));
	DigProject& project = m_data[point];
	project.cancel();
}
void HasDigDesignationsForFaction::remove(Area& area, const Point3D& point)
{
	assert(m_data.contains(point));
	 area.getSpace().designation_unset(point, m_faction, SpaceDesignation::Dig);
	m_data.erase(point);
}
void HasDigDesignationsForFaction::removeIfExists(Area& area, const Point3D& point)
{
	if(m_data.contains(point))
		remove(area, point);
}
PointFeatureTypeId HasDigDesignationsForFaction::getForPoint(const Point3D& point) const { return m_data[point].m_pointFeatureType; }
bool HasDigDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void AreaHasDigDesignations::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.emplace(faction, pair[1], deserializationMemo, faction, area);
	}
}
void AreaHasDigDesignations::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data[faction].loadWorkers(pair[1], deserializationMemo);
	}

}
Json AreaHasDigDesignations::toJson() const
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
void AreaHasDigDesignations::addFaction(const FactionId& faction)
{
	assert(!m_data.contains(faction));
	m_data.emplace(faction, faction);
}
void AreaHasDigDesignations::removeFaction(const FactionId& faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
// If pointFeatureType is null then dig out fully rather then digging out a feature.
void AreaHasDigDesignations::designate(const FactionId& faction, const Point3D& point, const PointFeatureTypeId pointFeatureType)
{
	if(!m_data.contains(faction))
		addFaction(faction);
	m_data[faction].designate(m_area, point, pointFeatureType);
}
void AreaHasDigDesignations::undesignate(const FactionId& faction, const Point3D& point)
{
	assert(m_data.contains(faction));
	assert(m_data[faction].m_data.contains(point));
	m_data[faction].undesignate(point);
}
void AreaHasDigDesignations::remove(const FactionId& faction, const Point3D& point)
{
	assert(m_data.contains(faction));
	m_data[faction].remove(m_area, point);
}
void AreaHasDigDesignations::clearAll(const Point3D& point)
{
	for(auto& pair : m_data)
		pair.second->removeIfExists(m_area, point);
}
void AreaHasDigDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair : pair.second->m_data)
			pair.second->clearReservations();
}
bool AreaHasDigDesignations::areThereAnyForFaction(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return !m_data[faction].empty();
}
DigProject& AreaHasDigDesignations::getForFactionAndPoint(const FactionId& faction, const Point3D& point)
{
	assert(m_data.contains(faction));
	assert(m_data[faction].m_data.contains(point));
	return m_data[faction].m_data[point];
}