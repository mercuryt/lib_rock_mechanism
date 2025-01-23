#include "types.h"
#include "config.h"
#include "cuboid.h"
Volume Volume::operator*(Quantity other) const { return Volume::create(data * other.get()); }
Mass Volume::operator*(Density density) const { return Mass::create(density.get() * data); }
Mass Density::operator*(Volume volume) const { return Mass::create(volume.get() * data); }
Mass Mass::operator*(Quantity quantity) const { return Mass::create(data * quantity.get()); }
Volume Volume::operator*(uint32_t other) const { return Volume::create(other * data); }
Volume Volume::operator*(float other) const { return Volume::create(other * data); }
CollisionVolume Volume::toCollisionVolume() const { return CollisionVolume::create(data / Config::unitsOfVolumePerUnitOfCollisionVolume); }
Volume CollisionVolume::toVolume() const { return Volume::create(data * Config::unitsOfVolumePerUnitOfCollisionVolume); }
Density Density::operator*(float other) const { return Density::create(other * data); }
Mass Mass::operator*(uint32_t other) const { auto output = Mass::create(data * other); assert(output != 0); return output; }
Mass Mass::operator*(float other) const { return Mass::create(data * other); }
DistanceInBlocksFractional DistanceInBlocks::toFloat() const { return DistanceInBlocksFractional::create(data); }
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