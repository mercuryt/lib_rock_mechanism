#pragma once

#include <random>
#include <algorithm>

namespace randomUtil
{
	static uint32_t seed = 23423487;
	static std::mt19937& getRng()
	{
		static std::mt19937 rng(seed);
		return rng;
	}
	static int32_t getInRange(int32_t lowest, int32_t highest)
	{
		assert(lowest < highest);
		static std::uniform_int_distribution<std::mt19937::result_type> dist(lowest, highest);
		return dist(getRng());
	}
	static uint32_t getInRange(uint32_t lowest, uint32_t highest)
	{
		return getInRange((int32_t)lowest, (int32_t)highest);
	}
	static bool percentChance(uint32_t percent)
	{
		static std::uniform_int_distribution<std::mt19937::result_type> dist(0, 100);
		return dist(getRng()) <= percent;
	}
	static bool chance(double chance)
	{
		static std::uniform_real_distribution<double> dist(0.0, 1.0);
		return dist(getRng()) <= chance;
	}
}
