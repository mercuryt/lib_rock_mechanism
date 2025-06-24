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
	std::uniform_int_distribution<DistanceInBlocksWidth> rangeX(cuboid.m_lowest.x().get(), cuboid.m_highest.x().get());
	std::uniform_int_distribution<DistanceInBlocksWidth> rangeY(cuboid.m_lowest.y().get(), cuboid.m_highest.y().get());
	std::uniform_int_distribution<DistanceInBlocksWidth> rangeZ(cuboid.m_lowest.z().get(), cuboid.m_highest.z().get());
	return Point3D::create(
		rangeX(rng),
		rangeY(rng),
		rangeZ(rng)
	);
}
