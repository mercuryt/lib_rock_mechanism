#include "../config.h"
#include "cuboid.h"
Vector3D Point3D::toVector3D() const { return {(int32_t)x.get(), (int32_t)y.get(), (int32_t)z.get()}; }
Facing4 Point3D::getFacingTwords(const Point3D& other) const
{
	if(other.y > y)
		return Facing4::South;
	if(other.x < x)
		return Facing4::West;
	if(other.x > x)
		return Facing4::East;
	if(other.y == y)
		assert(other.z != z);
	return Facing4::North;
}
bool Point3D::isInFrontOf(const Point3D& coordinates, const Facing4& facing) const
{
		switch(facing)
		{
			case Facing4::North:
				return y <= coordinates.y;
				break;
			case Facing4::East:
				return x >= coordinates.x;
				break;
			case Facing4::South:
				return y >= coordinates.y;
				break;
			case Facing4::West:
				return x <= coordinates.x;
				break;
			default:
				assert(false);
		}
}
void Point3D::operator+=(const Vector3D& other)
{
	x += other.x;
	y += other.y;
	z += other.z;
}