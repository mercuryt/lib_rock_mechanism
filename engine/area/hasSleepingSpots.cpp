#include "hasSleepingSpots.h"
#include "../deserializationMemo.h"
#include "../designations.h"
#include "../simulation/simulation.h"
#include "../space/space.h"
#include "../area/area.h"
void AreaHasSleepingSpots::load(const Json& data, DeserializationMemo&)
{
	for(const Json& point : data["unassigned"])
		m_unassigned.insert(point.get<Point3D>());
}
Json AreaHasSleepingSpots::toJson() const
{
	Json data{{"unassigned", Json::array()}};
	for(const Point3D& point : m_unassigned)
		data["unassigned"].push_back(point);
	return data;
}
void AreaHasSleepingSpots::designate(const FactionId& faction, const Point3D& point)
{
	m_unassigned.insert(point);
	m_area.getSpace().designation_set(point, faction, SpaceDesignation::Sleep);
}
void AreaHasSleepingSpots::undesignate(const FactionId& faction, const Point3D& point)
{
	m_unassigned.erase(point);
	m_area.getSpace().designation_unset(point, faction, SpaceDesignation::Sleep);
}
