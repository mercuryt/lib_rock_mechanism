#pragma once
#include "../numericTypes/types.h"
#include "../dataStructures/rtreeData.h"
class Area;
struct AreaHasPortalsBetweenOutSideAndInside
{
	RTreeData<Cuboid, RTreeDataConfigs::canOverlapNoMerge> m_data;
	SmallSet<Cuboid> m_toUpdate;
	void add(Area& area, const CuboidSet& cuboids);
	void add(Area& area, const Cuboid cuboid);
	void remove(Area& area, const CuboidSet& cuboids);
	void remove(Area& area, const Cuboid cuboid);
	void onTemperatureCanNoLongerTransmit(Area& area, const CuboidSet& cuboids);
	void onTemperatureCanNowTransmit(Area& area, const CuboidSet& cuboids);
	void doStep(Area& area);
	void maybeAdd(Area& area, const CuboidSet& cuboids);
	void maybeRemove(Area& area, const CuboidSet& cuboids);
	[[nodiscard]] bool isRecordedAsPortal(const Point3D& point) const;
	[[nodiscard]] CuboidSet getAffectedArea(const Area& area, const Cuboid cuboid);
	[[nodiscard]] DistanceFractional queryDistanceToNearest(const Point3D point);
	[[nodiscard]] CuboidSet getPortals(Area& area, const CuboidSet& cuboids);
	[[nodiscard]] bool isPortal(Area& area, const Point3D point);
	[[nodiscard]] TemperatureDelta getDelta(Area& area, const Point3D point);
	[[nodiscard]] GDB_CALLABLE int getToUpdateSize() const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasPortalsBetweenOutSideAndInside, m_data, m_toUpdate);
};