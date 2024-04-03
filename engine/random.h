#pragma once

#include "types.h"

#include <random>
#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <unordered_map>

class Random
{
	std::mt19937 rng;
public:
	//TODO: Unify getInRange as a template?
	int getInRange(int lowest, int highest);
	long getInRange(long lowest, long highest);
	unsigned int getInRange(unsigned int lowest, unsigned int highest);
	unsigned long getInRange(unsigned long lowest, unsigned long highest);
	float getInRange(float lowest, float highest);
	double getInRange(double lowest, double highest);
	bool percentChance(Percent percent);
	bool chance(double chance);
	template<typename T>
		T* getInVector(const std::vector<T*>& vector)
		{
			return vector.at(getInRange((size_t)0, vector.size() - (size_t)1));
		}
	template<typename T>
		const T* getInVector(const std::vector<const T*>& vector)
		{
			return vector.at(getInRange((size_t)0, vector.size() - (size_t)1));
		}
	template<typename T>
		std::vector<T*> getMultipleInVector(const std::vector<T*>& vector, uint32_t count)
		{
			assert(count <= vector.size());
			auto copy = vector;
			std::vector<T*> output;
			for(size_t i = 0; i < count; ++i)
			{
				size_t index = getInRange((size_t)0, copy.size() - (size_t)1);
				output.push_back(copy.at(index));
				copy.erase(copy.begin() + index);
			}
			return output;
		}
	//TODO: duplication.
	template<typename T>
		T& getInMap(const std::unordered_map<T, float>&& map)
		{
			float previous = 0;
			// Make probabilities cumulative.
			for(auto& pair : map)
				previous = (pair.second += previous);
			float roll = getInRange(0.f, previous);
			for(auto& pair : map)
				if(roll <= pair.second)
					return const_cast<T&>(pair.first);
			assert(false);
		}
	uint32_t deterministicScramble(uint32_t seed);
};
