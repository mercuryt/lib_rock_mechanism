#pragma once
#include "types.h"
class Blocks;
class BlockIndex;
class Cube;

struct Sphere
{
	Point3D center;
	DistanceInBlocksFractional radius;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] bool contains(const Cube& cube) const;
	[[nodiscard]] bool contains(const Blocks& blocks, const BlockIndex& block) const;
	[[nodiscard]] bool contains(const Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] bool overlapsWith(const Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] bool overlapsWith(const Cube& cube) const;
	[[nodiscard ]] static bool doesCuboidIntersectSphere(const Point3D& C1, const Point3D& C2, const Sphere& sphere);
};