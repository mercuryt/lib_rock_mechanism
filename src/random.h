#pragma once

#include "types.h"

#include <random>
#include <algorithm>
#include <cassert>

class Random
{
	std::mt19937 rng;
public:
	inline int getInRange(int lowest, int highest)
	{
		assert(lowest < highest);
		std::uniform_int_distribution<int> dist(lowest, highest);
		return dist(rng);
	}
	inline uint32_t getInRange(uint32_t lowest, uint32_t highest)
	{
		// TODO: optimize by mostly copying code from int get in range?
		return getInRange((int32_t)lowest, (int32_t)highest);
	}
	inline bool percentChance(Percent percent)
	{
		if(percent >= 100)
			return true;
		if(percent <= 0)
			return false;
		std::uniform_int_distribution<Percent> dist(1, 100);
		return dist(rng) <= percent;
	}
	inline bool chance(double chance)
	{
		assert(chance >= 0.0);
		assert(chance <= 1.0);
		std::uniform_real_distribution<double> dist(0.0, 1.0);
		return dist(rng) <= chance;
	}
	template<typename T>
		inline T& getInVector(std::vector<T>& vector)
		{
			return vector[getInRange(0, vector.size() - 1)];
		}
};
