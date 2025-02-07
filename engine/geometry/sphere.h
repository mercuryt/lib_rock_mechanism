#pragma once
#include "../json.h"
#include "point3D.h"
#include "../types.h"
class Blocks;
class BlockIndex;
class Cuboid;

struct Sphere
{
	Point3D center;
	DistanceInBlocksFractional radius;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] bool contains(const Blocks& blocks, const BlockIndex& block) const;
	[[nodiscard]] bool contains(const Cuboid& cuboid) const;
	[[nodiscard]] bool intersects(const Cuboid& cuboid) const;
	[[nodiscard ]] static bool doesCuboidIntersectSphere(const Point3D& C1, const Point3D& C2, const Sphere& sphere);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Sphere, center, radius)
};