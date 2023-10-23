#pragma once

#include "types.h"

#include <random>
#include <algorithm>
#include <cassert>

namespace randomUtil
{
	constexpr inline uint32_t seed = 23421487;
	inline std::mt19937& getRng()
	{
		static std::mt19937 rng(seed);
		return rng;
	}
	inline int32_t getInRange(int lowest, int highest)
	{
		assert(lowest < highest);
		static std::uniform_int_distribution<uint32_t> dist(lowest, highest);
		return dist(getRng());
	}
	[[maybe_unused]] inline uint32_t getInRange(uint32_t lowest, uint32_t highest)
	{
		return getInRange((int32_t)lowest, (int32_t)highest);
	}
	[[maybe_unused]] inline bool percentChance(Percent percent)
	{
		static std::uniform_int_distribution<Percent> dist(1, 100);
		return dist(getRng()) <= percent;
	}
	[[maybe_unused]] inline bool chance(double chance)
	{
		static std::uniform_real_distribution<double> dist(0.0, 1.0);
		return dist(getRng()) <= chance;
	}
}
