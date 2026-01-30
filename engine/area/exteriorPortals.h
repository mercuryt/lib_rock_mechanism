#pragma once
#include "../numericTypes/types.h"
#include "../dataStructures/rtreeData.h"
#include "../dataStructures/smallSet.h"
#include "../geometry/point3D.h"
#include "../config/config.h"

class Space;
class AreaHasExteriorPortals
{
	RTreeData<Distance> m_distances;
	std::array<SmallSet<Point3D>, Config::maxDepthExteriorPortalPenetration.get() + 1> m_points;
	std::array<TemperatureDelta, Config::maxDepthExteriorPortalPenetration.get() + 1> m_deltas;
	void setDistance(Space& space, const Point3D& point, const Distance& distance);
	void unsetDistance(Space& space, const Point3D& point);
public:
	void initialize();
	void add(Area& area, const Point3D& point, Distance distance = Distance::create(0));
	void remove(Area& area, const Point3D& point);
	void onChangeAmbiantSurfaceTemperature(Space& space, const Temperature& temperature);
	void onPointCanTransmitTemperature(Area& area, const Point3D& point);
	void onCuboidCanNotTransmitTemperature(Area& area, const Cuboid& cuboid);
	[[nodiscard]] bool isRecordedAsPortal(const Point3D& point) { return getDistanceFor(point) == 0; }
	[[nodiscard]] Distance getDistanceFor(const Point3D& point) { return m_distances.queryGetOne(point); }
	[[nodiscard]] static TemperatureDelta getDeltaForAmbientTemperatureAndDistance(const Temperature& ambientTemperature, const Distance& distance);
	[[nodiscard]] static bool isPortal(const Space& space, const Point3D& point);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasExteriorPortals, m_distances, m_points, m_deltas);
};