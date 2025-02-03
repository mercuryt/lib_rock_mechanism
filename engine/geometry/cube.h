// One BlockIndex has xyz dimensions of 1 meter by 1 meter by 2 meters.
// This File defines physical types.
// idTypes.h defines id types, which don't ever change.
// index.h defines index types, which do change and must be updated.

#pragma once
#include "../json.h"
#include "../idTypes.h"
#include "../types.h"
#include "point3D.h"
#include <compare>
#include <cstdint>
#include <iostream>
#include <string>

class Cuboid;
struct Cube
{
	Point3D center;
	DistanceInBlocks halfWidth;
	[[nodiscard]] uint size() const { return pow(halfWidth.get() * 2, 3); }
	[[nodiscard]] bool intersects(const Cube& other) const
	{
		return
			center.x() + halfWidth >= other.center.x().subtractWithMinimum(other.halfWidth) &&
			center.y() + halfWidth >= other.center.y().subtractWithMinimum(other.halfWidth) &&
			center.z() + halfWidth >= other.center.z().subtractWithMinimum(other.halfWidth) &&
			center.x() - halfWidth <= other.center.x() + other.halfWidth &&
			center.y() - halfWidth <= other.center.y() + other.halfWidth &&
			center.z() - halfWidth <= other.center.z() + other.halfWidth;
	}
	[[nodiscard]] bool intersects(const Cuboid& other) const;
	[[nodiscard]] bool intersects(const Point3D high, const Point3D low) const
	{
		return
			center.x() + halfWidth >= low.x() &&
			center.y() + halfWidth >= low.y() &&
			center.z() + halfWidth >= low.z() &&
			center.x().subtractWithMinimum(halfWidth) <= high.x() &&
			center.y().subtractWithMinimum(halfWidth) <= high.y() &&
			center.z().subtractWithMinimum(halfWidth) <= high.z();
	}
	[[nodiscard]] bool contains(const Cube& other) const
	{
		return
			center.x() + halfWidth >= other.center.x() + other.halfWidth &&
			center.y() + halfWidth >= other.center.y() + other.halfWidth &&
			center.z() + halfWidth >= other.center.z() + other.halfWidth &&
			center.x().subtractWithMinimum(halfWidth) <= other.center.x().subtractWithMinimum(other.halfWidth) &&
			center.y().subtractWithMinimum(halfWidth) <= other.center.y().subtractWithMinimum(other.halfWidth) &&
			center.z().subtractWithMinimum(halfWidth) <= other.center.z().subtractWithMinimum(other.halfWidth);
	}
	[[nodiscard]] bool contains(const Point3D& coordinates) const
	{
		return
			center.x() + halfWidth >= coordinates.x() &&
			center.y() + halfWidth >= coordinates.y() &&
			center.z() + halfWidth >= coordinates.z() &&
			center.x().subtractWithMinimum(halfWidth) <= coordinates.x() &&
			center.y().subtractWithMinimum(halfWidth) <= coordinates.y() &&
			center.z().subtractWithMinimum(halfWidth) <= coordinates.z();
	}
	[[nodiscard]] bool contains(const Cuboid& other) const;
	[[nodiscard]] bool contains(const Point3D high, const Point3D low) const
	{
		return
			center.x() + halfWidth >= high.x() &&
			center.y() + halfWidth >= high.y() &&
			center.z() + halfWidth >= high.z() &&
			center.x().subtractWithMinimum(halfWidth) <= low.x() &&
			center.y().subtractWithMinimum(halfWidth) <= low.y() &&
			center.z().subtractWithMinimum(halfWidth) <= low.z();
	}
	[[nodiscard]] bool isContainedBy(const Cuboid& cuboid) const;
	[[nodiscard]] bool isContainedBy(const Cube& other) const { return other.contains(*this); }
	[[nodiscard]] bool isContainedBy(const Point3D& highest, const Point3D& lowest) const
	{
		return
			highest.x() >= center.x() + halfWidth &&
			highest.y() >= center.y() + halfWidth &&
			highest.z() >= center.z() + halfWidth &&
			lowest.x() <= center.x().subtractWithMinimum(halfWidth) &&
			lowest.y() <= center.y().subtractWithMinimum(halfWidth) &&
			lowest.z() <= center.z().subtractWithMinimum(halfWidth);
	}
	[[nodiscard]] Point3D getHighPoint() const { return {center.x() + halfWidth, center.y() + halfWidth, center.z() + halfWidth}; }
	[[nodiscard]] Point3D getLowPoint() const { return {center.x() - halfWidth, center.y() - halfWidth, center.z() - halfWidth}; }
	[[nodiscard]] bool isSomewhatInFrontOf(const Point3D& position, const Facing4& facing) const { return getHighPoint().isInFrontOf(position, facing) || getLowPoint().isInFrontOf(position, facing); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Cube, center, halfWidth);
};