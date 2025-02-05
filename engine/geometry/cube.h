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
		Point3D offset = other.center;
		offset.data -= other.halfWidth.get();
		return
			(getHighPoint().data >= other.getLowPoint().data).all() &&
			(getLowPoint().data <= other.getHighPoint().data).all();
	}
	[[nodiscard]] bool intersects(const Cuboid& other) const;
	[[nodiscard]] bool intersects(const Point3D high, const Point3D low) const
	{
		Point3D offset = center;
		offset.data -= halfWidth.get();
		return
			(getHighPoint().data >= low.data).all() &&
			(getLowPoint().data <= high.data).all();
	}
	[[nodiscard]] bool contains(const Cube& other) const
	{
		return
			(center.data + halfWidth.get() >= other.getHighPoint().data).all() &&
			(center.data - halfWidth.get() <= other.getLowPoint().data).all();
	}
	[[nodiscard]] bool contains(const Point3D& coordinates) const
	{
		return
			(center.data + halfWidth.get() >= coordinates.data).all() &&
			(center.data - halfWidth.get() <= coordinates.data).all();
	}
	[[nodiscard]] bool contains(const Cuboid& other) const;
	[[nodiscard]] bool contains(const Point3D high, const Point3D low) const
	{
		return
			(center.data + halfWidth.get() >= high.data).all() &&
			(center.data - halfWidth.get() <= low.data).all();
	}
	[[nodiscard]] bool isContainedBy(const Cuboid& cuboid) const;
	[[nodiscard]] bool isContainedBy(const Cube& other) const { return other.contains(*this); }
	[[nodiscard]] bool isContainedBy(const Point3D& highest, const Point3D& lowest) const
	{
		return
			(highest.data >= center.data + halfWidth.get()).all() &&
			(lowest.data <= center.data - halfWidth.get()).all();
	}
	[[nodiscard]] Point3D getHighPoint() const { return {center + halfWidth}; }
	[[nodiscard]] Point3D getLowPoint() const { return {center - halfWidth}; }
	[[nodiscard]] bool isSomewhatInFrontOf(const Point3D& position, const Facing4& facing) const { return getHighPoint().isInFrontOf(position, facing) || getLowPoint().isInFrontOf(position, facing); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Cube, center, halfWidth);
};