#pragma once
#include "../dataStructures/rtreeBoolean.h"
#include "../numericTypes/index.h"

class PointsExposedToSky
{
	RTreeBoolean m_data;
public:
	void initalizeEmpty(Area& area);
	void set(Area& area, const Point3D& point);
	void unset(Area& area, const Point3D& point);
	[[nodiscard]] bool check(const Point3D& point) const { return m_data.query(point); };
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointsExposedToSky, m_data);
};