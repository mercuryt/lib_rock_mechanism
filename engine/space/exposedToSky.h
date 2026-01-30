#pragma once
#include "../dataStructures/rtreeBoolean.h"
#include "../numericTypes/index.h"

class PointsExposedToSky
{
	RTreeBoolean m_data;
public:
	void initialize(const Cuboid& cuboid);
	void set(Area& area, const Point3D& point);
	void unset(Area& area, const Point3D& point);
	void maybeUnset(Area& area, const Cuboid& cuboid);
	void prepare() { m_data.prepare(); }
	void beforeJsonLoad() { m_data.beforeJsonLoad(); }
	[[nodiscard]] bool check(const auto& shape) const { return m_data.query(shape); };
	[[nodiscard]] bool canPrepare() const { return m_data.canPrepare(); };
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointsExposedToSky, m_data);
};