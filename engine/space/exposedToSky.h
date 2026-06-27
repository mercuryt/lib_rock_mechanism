#pragma once
#include "../dataStructures/rtreeBoolean.h"
#include "../numericTypes/index.h"

class PointsExposedToSky
{
	RTreeBoolean m_data;
public:
	void initialize(const Cuboid cuboid);
	void set(Area& area, const Cuboid cuboid);
	void set(Area& area, const Point3D point);
	void maybeSetCuboid(Area& area, const Cuboid cuboid);
	void unset(Area& area, const Point3D point);
	void maybeUnsetBeneathTopLayer(Area& area, const Cuboid cuboid);
	void prepare() { m_data.prepare(); }
	void beforeJsonLoad() { m_data.beforeJsonLoad(); }
	[[nodiscard]] bool check(const CuboidSet& cuboids) const;
	[[nodiscard]] bool check(const Cuboid cuboid) const;
	GDB_CALLABLE bool check(const Point3D point) const;
	[[nodiscard]] bool canPrepare() const { return m_data.canPrepare(); }
	[[nodiscard]] const RTreeBoolean& get() const { return m_data; }
	[[nodiscard]] CuboidSet removeNotExposedFrom(const auto& shape) const
	{
		CuboidSet output = m_data.queryGetLeaves(shape);
		return output.intersection(shape);
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointsExposedToSky, m_data);
};