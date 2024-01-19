#pragma once

#include "types.h"

#include <random>
#include <algorithm>
#include <cassert>
#include <unordered_set>

class Random
{
	std::mt19937 rng;
public:
	inline int getInRange(int lowest, int highest)
	{
		assert(lowest < highest);
		//TODO: should uniform distribution be static?
		std::uniform_int_distribution<int> dist(lowest, highest);
		return dist(rng);
	}
	inline long getInRange(long lowest, long highest)
	{
		assert(lowest < highest);
		//TODO: should uniform distribution be static?
		std::uniform_int_distribution<long> dist(lowest, highest);
		return dist(rng);
	}
	inline unsigned int getInRange(unsigned int lowest, unsigned int highest)
	{
		assert(lowest < highest);
		//TODO: should uniform distribution be static?
		std::uniform_int_distribution<unsigned int> dist(lowest, highest);
		return dist(rng);
	}
	inline float getInRange(float lowest, float highest)
	{
		assert(lowest < highest);
		//TODO: should uniform distribution be static?
		std::uniform_real_distribution<float> dist(lowest, highest);
		return dist(rng);
	}
	inline double getInRange(double lowest, double highest)
	{
		assert(lowest < highest);
		//TODO: should uniform distribution be static?
		std::uniform_real_distribution<double> dist(lowest, highest);
		return dist(rng);
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
	template<typename T>
		inline std::vector<T*> getMultipleInVector(std::vector<T>& vector, uint32_t count)
		{
			assert(count <= vector.size());
			auto copy = vector;
			std::vector<T*> output;
			for(uint32_t i = 0; i < count; ++i)
			{
				size_t index = getInRange(0, copy.size() - 1);
				output.push_back(copy[index]);
				copy.erase(copy.begin() + index);
			}
			return output;
		}
	template<typename T, typename Probability>
		inline T& getInMap(std::unordered_map<T, Probability>&& map)
		{
			Probability previous = 0;
			// Make probabilities cumulative.
			for(auto& pair : map)
				previous = (pair.second += previous);
			Probability roll = getInRange(0, previous);
			for(auto& pair : map)
				if(roll <= pair.second)
					return pair.first;
			assert(false);
		}
	inline uint32_t deterministicScramble(uint32_t seed)
	{
		std::mt19937 rng(seed);
		std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
		return dist(rng);
	}
};
