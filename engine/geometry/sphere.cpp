#include "sphere.h"
#include "cuboid.h"
#include "../space/space.h"
#include "../numericTypes/index.h"
#include <cmath>
bool Sphere::contains(const Point3D& point) const
{
	return point.distanceToSquared(center).toFloat() < (radius * radius);
}
bool Sphere::contains(const Cuboid& cuboid) const
{
	return contains(cuboid.m_high) && contains(cuboid.m_low);
}
bool Sphere::intersects(const Cuboid& cuboid) const
{
	return doesCuboidIntersectSphere(cuboid.m_high, cuboid.m_low, *this);
}
bool Sphere::doesCuboidIntersectSphere(const Point3D& highest, const Point3D& lowest, const Sphere& sphere)
{
	int32_t radius = sphere.radius.get();
	const Eigen::Array<int32_t, 1, 3>& high = highest.toOffset().data;
	const Eigen::Array<int32_t, 1, 3>& low = lowest.toOffset().data;
	const Eigen::Array<int32_t, 1, 3>& center = sphere.center.toOffset().data;
	return !(
		((high + radius) < center).any() ||
		((low - radius) > center).any()
	);
}