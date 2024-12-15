#include "types.h"
#include "config.h"
#include "cuboid.h"
#include "area.h"
#include "blocks/blocks.h"
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
bool Cube::intersects(const Area& area, const Cuboid& cuboid) const
{
	const Blocks& blocks = area.getBlocks();
	Point3D highest = blocks.getCoordinates(cuboid.m_highest);
	Point3D lowest = blocks.getCoordinates(cuboid.m_lowest);
	return intersects(highest, lowest);
}
[[nodiscard]] bool Cube::isContainedBy(const Area& area, const Cuboid& cuboid) const
{
	const Blocks& blocks = area.getBlocks();
	Point3D highest = blocks.getCoordinates(cuboid.m_highest);
	Point3D lowest = blocks.getCoordinates(cuboid.m_lowest);
	return isContainedBy(highest, lowest);
}