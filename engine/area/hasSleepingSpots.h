#pragma once
#include "../config/config.h"
#include "../numericTypes/index.h"
#include "../geometry/point3D.h"
struct Faction;
struct DeserializationMemo;
class Area;
// TODO: remove m_area, use default json.
class AreaHasSleepingSpots final
{
	Area& m_area;
	SmallSet<Point3D> m_unassigned;
public:
	AreaHasSleepingSpots(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void designate(const FactionId& faction, const Point3D& point);
	void undesignate(const FactionId& faction, const Point3D& point);
	bool containsUnassigned(const Point3D& point) const { return m_unassigned.contains(point); }
};
