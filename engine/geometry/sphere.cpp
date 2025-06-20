#include "sphere.h"
#include "cuboid.h"
#include "../blocks/blocks.h"
#include "../numericTypes/index.h"
#include <cmath>
bool Sphere::contains(const Point3D& point) const
{
	return point.distanceSquared(center).toFloat() < (radius * radius);
}
bool Sphere::contains(const Blocks& blocks, const BlockIndex& block) const
{
	return contains(blocks.getCoordinates(block));
}
bool Sphere::contains(const Cuboid& cuboid) const
{
	return contains(cuboid.m_highest) && contains(cuboid.m_lowest);
}
bool Sphere::intersects(const Cuboid& cuboid) const
{
	return doesCuboidIntersectSphere(cuboid.m_highest, cuboid.m_lowest, *this);
}
bool Sphere::doesCuboidIntersectSphere(const Point3D& highest, const Point3D& lowest, const Sphere& sphere)
{
	int radius = sphere.radius.get();
	const Eigen::Array<int, 1, 3>& high = highest.toOffset().data;
	const Eigen::Array<int, 1, 3>& low = lowest.toOffset().data;
	const Eigen::Array<int, 1, 3>& center = sphere.center.toOffset().data;
	return !(
		((high + radius) < center).any() ||
		((low - radius) > center).any()
	);
}