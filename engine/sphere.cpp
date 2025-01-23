#include "sphere.h"
#include "blocks/blocks.h"
#include "index.h"
#include <cmath>
bool Sphere::contains(const Point3D& point) const
{
	return point.distanceSquared(center).toFloat() < (radius * radius);
}
bool Sphere::contains(const Cube& cube) const
{
	return contains(cube.getHighPoint()) && contains(cube.getLowPoint());
}
bool Sphere::contains(const Blocks& blocks, const BlockIndex& block) const
{
	return contains(blocks.getCoordinates(block));
}
bool Sphere::contains(const Cuboid& cuboid) const
{
	return contains(cuboid.m_highest) && contains(cuboid.m_lowest);
}
bool Sphere::overlapsWith(const Cuboid& cuboid) const
{
	return doesCuboidIntersectSphere(cuboid.m_highest, cuboid.m_lowest, *this);
}
bool Sphere::overlapsWith(const Cube& cube) const
{
	return doesCuboidIntersectSphere(cube.getHighPoint(), cube.getLowPoint(), *this);
}
bool Sphere::doesCuboidIntersectSphere(const Point3D& highest, const Point3D& lowest, const Sphere& sphere)
{
	auto squared = [](const int32_t& i) -> int32_t { return i * i; };
	float dist_squared = (float)std::pow(sphere.radius.get(), 2);
	Vector3D S = sphere.center.toVector3D();
	Vector3D C1 = highest.toVector3D();
	Vector3D C2 = lowest.toVector3D();
	if (S.x < C1.x) dist_squared -= squared(S.x - C1.x);
	else if (S.x > C2.x) dist_squared -= squared(S.x - C2.x);
	if (S.y < C1.y) dist_squared -= squared(S.y - C1.y);
	else if (S.y > C2.y) dist_squared -= squared(S.y - C2.y);
	if (S.z < C1.z) dist_squared -= squared(S.z - C1.z);
	else if (S.z > C2.z) dist_squared -= squared(S.z - C2.z);
	return dist_squared > 0;
}