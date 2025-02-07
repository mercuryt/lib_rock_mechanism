#include "cube.h"
#include "../types.h"
#include "cuboid.h"
uint Cube::size() const { return pow(halfWidth.get() * 2, 3); }
bool Cube::intersects(const Cube& other) const
{
	Point3D offset = other.center;
	offset.data -= other.halfWidth.get();
	return
		(getHighPoint().data >= other.getLowPoint().data).all() &&
		(getLowPoint().data <= other.getHighPoint().data).all();
}
bool Cube::intersects(const Cuboid& cuboid) const
{
	return intersects(cuboid.m_highest, cuboid.m_lowest);
}
bool Cube::intersects(const Point3D high, const Point3D low) const
{
	Point3D offset = center;
	offset.data -= halfWidth.get();
	return
		(getHighPoint().data >= low.data).all() &&
		(getLowPoint().data <= high.data).all();
}
bool Cube::contains(const Cube& other) const
{
	return
		(center.data + halfWidth.get() >= other.getHighPoint().data).all() &&
		(center.data - halfWidth.get() <= other.getLowPoint().data).all();
}
bool Cube::contains(const Cuboid& cuboid) const
{
	return contains(cuboid.m_highest, cuboid.m_lowest);
}
bool Cube::contains(const Point3D& coordinates) const
{
	return
		(center.data + halfWidth.get() >= coordinates.data).all() &&
		(center.data - halfWidth.get() <= coordinates.data).all();
}
bool Cube::contains(const Point3D high, const Point3D low) const
{
	return
		(center.data + halfWidth.get() >= high.data).all() &&
		(center.data - halfWidth.get() <= low.data).all();
}
bool Cube::isContainedBy(const Cube& other) const { return other.contains(*this); }
bool Cube::isContainedBy(const Cuboid& cuboid) const
{
	return isContainedBy(cuboid.m_highest, cuboid.m_lowest);
}
bool Cube::isContainedBy(const Point3D& highest, const Point3D& lowest) const
{
	return
		(highest.data >= center.data + halfWidth.get()).all() &&
		(lowest.data <= center.data - halfWidth.get()).all();
}
Point3D Cube::getHighPoint() const { return {center + halfWidth}; }
Point3D Cube::getLowPoint() const { return {center - halfWidth}; }
bool Cube::isSomewhatInFrontOf(const Point3D& position, const Facing4& facing) const { return getHighPoint().isInFrontOf(position, facing) || getLowPoint().isInFrontOf(position, facing); }
Cuboid Cube::toCuboid() const
{
	return {getHighPoint(), getLowPoint()};
}