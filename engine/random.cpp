#include "random.h"
#include "geometry/cuboid.h"
bool Random::percentChance(const Percent& percent)
{
	if(percent >= 100)
		return true;
	if(percent <= 0)
		return false;
	std::uniform_int_distribution<int> dist(1, 100);
	return dist(rng) <= (int)percent.get();
}
bool Random::chance(double chance)
{
	assert(chance >= 0.0);
	assert(chance <= 1.0);
	std::uniform_real_distribution<double> dist(0.0, 1.0);
	return dist(rng) <= chance;
}
bool Random::chance(float chance)
{
	assert(chance >= 0.0f);
	assert(chance <= 1.0f);
	std::uniform_real_distribution<float> dist(0.0, 1.0);
	return dist(rng) <= chance;
}
Point3D Random::getInCuboid(const Cuboid& cuboid)
{
	std::uniform_int_distribution<DistanceWidth> rangeX(cuboid.m_low.x().get(), cuboid.m_high.x().get());
	std::uniform_int_distribution<DistanceWidth> rangeY(cuboid.m_low.y().get(), cuboid.m_high.y().get());
	std::uniform_int_distribution<DistanceWidth> rangeZ(cuboid.m_low.z().get(), cuboid.m_high.z().get());
	return Point3D::create(
		rangeX(rng),
		rangeY(rng),
		rangeZ(rng)
	);
}
