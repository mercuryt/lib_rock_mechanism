#include "types.h"
#include "config.h"
#include "geometry/cuboid.h"
FullDisplacement FullDisplacement::operator*(Quantity other) const { return FullDisplacement::create(data * other.get()); }
Mass FullDisplacement::operator*(Density density) const { return Mass::create(density.get() * data); }
Mass Density::operator*(FullDisplacement volume) const { return Mass::create(volume.get() * data); }
Mass Mass::operator*(Quantity quantity) const { return Mass::create(data * quantity.get()); }
FullDisplacement FullDisplacement::operator*(uint32_t other) const { return FullDisplacement::create(other * data); }
FullDisplacement FullDisplacement::operator*(float other) const { return FullDisplacement::create(other * data); }
CollisionVolume FullDisplacement::toCollisionVolume() const { return CollisionVolume::create(data / Config::unitsOfVolumePerUnitOfCollisionVolume); }
FullDisplacement CollisionVolume::toVolume() const { return FullDisplacement::create(data * Config::unitsOfVolumePerUnitOfCollisionVolume); }
Density Density::operator*(float other) const { return Density::create(other * data); }
Mass Mass::operator*(uint32_t other) const { auto output = Mass::create(data * other); assert(output != 0); return output; }
Mass Mass::operator*(float other) const { return Mass::create(data * other); }
Density Mass::operator/(const FullDisplacement& displacement) const { return Density::create((float)data / (float)displacement.get()); }