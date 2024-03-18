#include "random.h"
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
	std::uniform_int_distribution<Percent> dist(1, 100);
	return dist(rng) <= percent;
}
bool Random::chance(double chance)
{
	assert(chance >= 0.0);
	assert(chance <= 1.0);
	std::uniform_real_distribution<double> dist(0.0, 1.0);
	return dist(rng) <= chance;
}
uint32_t Random::deterministicScramble(uint32_t seed)
{
	std::mt19937 rng(seed);
	std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
	return dist(rng);
}
