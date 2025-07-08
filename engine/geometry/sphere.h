#pragma once
#include "../json.h"
#include "point3D.h"
#include "../numericTypes/types.h"
class Space;
class Point3D;
class Cuboid;

struct Sphere
{
	Point3D center;
	DistanceFractional radius;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] bool contains(const Space& space, const Point3D& point) const;
	[[nodiscard]] bool contains(const Cuboid& cuboid) const;
	[[nodiscard]] bool intersects(const Cuboid& cuboid) const;
	[[nodiscard]] DistanceFractional radiusSquared() const { return radius * radius; }
	// Provided for conformity of interface with cuboid.
	[[nodiscard]] const Point3D& getCenter() const { return center; }
	[[nodiscard ]] static bool doesCuboidIntersectSphere(const Point3D& C1, const Point3D& C2, const Sphere& sphere);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Sphere, center, radius)
};