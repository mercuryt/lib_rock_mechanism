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
	[[nodiscard]] uint size() const;
	[[nodiscard]] bool intersects(const Cube& other) const;
	[[nodiscard]] bool intersects(const Cuboid& other) const;
	[[nodiscard]] bool intersects(const Point3D high, const Point3D low) const;
	[[nodiscard]] bool contains(const Cube& other) const;
	[[nodiscard]] bool contains(const Point3D& coordinates) const;
	[[nodiscard]] bool contains(const Cuboid& other) const;
	[[nodiscard]] bool contains(const Point3D high, const Point3D low) const;
	[[nodiscard]] bool isContainedBy(const Cuboid& cuboid) const;
	[[nodiscard]] bool isContainedBy(const Cube& other) const;
	[[nodiscard]] bool isContainedBy(const Point3D& highest, const Point3D& lowest) const;
	[[nodiscard]] Point3D getHighPoint() const;
	[[nodiscard]] Point3D getLowPoint() const;
	[[nodiscard]] bool isSomewhatInFrontOf(const Point3D& position, const Facing4& facing) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Cube, center, halfWidth);
};