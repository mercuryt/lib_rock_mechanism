#include "random.h"
#include "geometry/cuboid.h"
int Random::getInRange(int lowest, int highest)
{
	assert(lowest <= highest);
	//TODO: should uniform distribution be static?
	std::uniform_int_distribution<int> dist(lowest, highest);
	return dist(rng);
}
long Random::getInRange(long lowest, long highest)
{
	assert(lowest <= highest);
	//TODO: should uniform distribution be static?
	std::uniform_int_distribution<long> dist(lowest, highest);
	return dist(rng);
}
unsigned int Random::getInRange(unsigned int lowest, unsigned int highest)
{
	assert(lowest <= highest);
	//TODO: should uniform distribution be static?
	std::uniform_int_distribution<unsigned int> dist(lowest, highest);
	return dist(rng);
}
unsigned long Random::getInRange(unsigned long lowest, unsigned long highest)
{
	assert(lowest <= highest);
	//TODO: should uniform distribution be static?
	std::uniform_int_distribution<unsigned long> dist(lowest, highest);
	return dist(rng);
}
float Random::getInRange(float lowest, float highest)
{
	assert(lowest <= highest);
	//TODO: should uniform distribution be static?
	std::uniform_real_distribution<float> dist(lowest, highest);
	return dist(rng);
}
double Random::getInRange(double lowest, double highest)
{
	assert(lowest <= highest);
	//TODO: should uniform distribution be static?
	std::uniform_real_distribution<double> dist(lowest, highest);
	return dist(rng);
}
bool Random::percentChance(Percent percent)
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
