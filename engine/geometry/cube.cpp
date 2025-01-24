#include "cube.h"
#include "../types.h"
#include "cuboid.h"
bool Cube::intersects(const Cuboid& cuboid) const
{
	return intersects(cuboid.m_highest, cuboid.m_lowest);
}
bool Cube::contains(const Cuboid& cuboid) const
{
	return contains(cuboid.m_highest, cuboid.m_lowest);
}
bool Cube::isContainedBy(const Cuboid& cuboid) const
{
	return isContainedBy(cuboid.m_highest, cuboid.m_lowest);
}