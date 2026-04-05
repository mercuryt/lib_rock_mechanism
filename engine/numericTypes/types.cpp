#include "numericTypes/types.h"
#include "config/config.h"
#include "config/physics.h"
#include "geometry/cuboid.h"
Step Step::createDbg(const StepWidth& value) { return Step::create(value); }
Speed Force::operator/(const Mass mass) const { return Speed::create((float)data / (float)mass.get()); }
FullDisplacement FullDisplacement::operator*(Quantity other) const { return FullDisplacement::create(data * other.get()); }
Mass FullDisplacement::operator*(Density density) const { return Mass::create(density.get() * data); }
Mass Density::operator*(FullDisplacement volume) const { return Mass::create(volume.get() * data); }
Mass Mass::operator*(Quantity quantity) const { return Mass::create(data * quantity.get()); }
FullDisplacement FullDisplacement::operator*(FullDisplacementWidth other) const { return FullDisplacement::create(other * data); }
FullDisplacement FullDisplacement::operator*(float other) const { return FullDisplacement::create(other * data); }
CollisionVolume FullDisplacement::toCollisionVolume() const { return CollisionVolume::create(data / Config::unitsOfVolumePerUnitOfCollisionVolume); }
FullDisplacement FullDisplacement::createFromCollisionVolume(const CollisionVolume collisionVolume)
{
	return FullDisplacement::create(collisionVolume.get() * Config::unitsOfVolumePerUnitOfCollisionVolume);
}
FullDisplacement CollisionVolume::toVolume() const { return FullDisplacement::create(data * Config::unitsOfVolumePerUnitOfCollisionVolume); }
Density Density::operator*(float other) const { return Density::create(other * data); }
Mass Mass::operator*(int other) const { auto output = Mass::create(data * other); return output; }
Mass Mass::operator*(float other) const { return Mass::create(data * other); }
Density Mass::operator/(const FullDisplacement displacement) const { return Density::create((float)data / (float)displacement.get()); }
Temperature& Temperature::operator+=(const TemperatureDelta delta)
{
	data = addWithMaximum(delta.get()).get();
	return *this;
}
Temperature& Temperature::operator-=(const TemperatureDelta delta)
{
	data = subtractWithMinimum(delta.get()).get();
	return *this;
}

Temperature Temperature::operator+(const TemperatureDelta delta) const
{
	auto copy = *this;
	copy += delta;
	return copy;
}
Temperature Temperature::operator-(const TemperatureDelta delta) const
{
	auto copy = *this;
	copy += delta;
	return copy;
}
TemperatureDelta TemperatureDelta::reduceForDistanceAmbiant(const DistanceFractional distance) const
{
	if(distance == 0)
		return *this;
	return TemperatureDelta::create((float)data / (distance.get() * Config::Physics::ambiantTemperatureDeltaDecay));
}
Distance TemperatureDelta::effectDistanceAmbiant() const
{
	// td1 = td0 / (d * R)
	// td1 * ( d * R) = td0
	// d * R = td0 / td1
	// d = (td0/td1)/R
	return Distance::create(std::round(((float)data / (float)Config::Physics::minimumHeatDeltaToTrackAffectedArea.get()) / Config::Physics::ambiantTemperatureDeltaDecay));
}
TemperatureDelta TemperatureDelta::reduceForDistanceRadiant(const DistanceFractional distance) const
{
	if(distance == 0)
		return *this;
	return TemperatureDelta::create((float)data / std::pow(distance.get(), Config::Physics::radiantHeatDisipatesAtDistanceExponent));
}
Distance TemperatureDelta::effectDistanceRadiant() const
{
	// td1 = td0/(d^E)
	// td1 * (d ^ E) = td0
	// (d^E)=td0/td1
	// d = root(td0 / td1, E)
	return Distance::create(std::round(std::pow((float)data / (float)Config::Physics::minimumHeatDeltaToTrackAffectedArea.get(), 1.f / Config::Physics::radiantHeatDisipatesAtDistanceExponent)));
}