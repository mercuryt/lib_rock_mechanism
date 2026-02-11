#pragma once

#include "../numericTypes/index.h"
#include "../numericTypes/types.h"
#include "../dataStructures/rtreeBoolean.h"
#include "../dataStructures/rtreeBooleanLowZOnly.h"

class Area;

class OpacityFacade final
{
	RTreeBoolean m_fullOpacity;
	RTreeBooleanLowZOnly m_floorOpacity;
public:
	void update(const Area& area, const Cuboid& cuboid);
	void update(const Area& area, const Point3D& point) { update(area, {point, point}); }
	void maybeInsertFull(const Cuboid& cuboid);
	void maybeRemoveFull(const Cuboid& cuboid);
	void maybeInsertFull(const Point3D& point) { maybeInsertFull({point, point}); }
	void maybeRemoveFull(const Point3D& point) { maybeRemoveFull({point, point}); }
	void removeFromCuboidSet(CuboidSet& set) const { m_fullOpacity.queryRemove(set); }
	void queryForEach(const auto& shape, auto&& action) const { m_fullOpacity.queryForEach(shape, action); }
	[[nodiscard]] bool hasLineOfSight(const Point3D& fromCoords, const Point3D& toCoords) const;
	[[nodiscard]] std::vector<bool> hasLineOfSightBatched(const std::vector<std::pair<Point3D, Point3D>>& coords) const;
	__attribute__((noinline)) void validate(const Area& area) const;
};
