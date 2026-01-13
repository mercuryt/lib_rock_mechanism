#pragma once

#include "numericTypes/types.h"

#include <random>
#include <algorithm>
#include <cassert>

struct Cuboid;
struct Point3D;

class Random
{
	std::mt19937 rng;
public:
	template<typename T>
	T getInRange(T lowest, T highest)
	{
		assert(lowest <= highest);
		//TODO: should uniform distribution be static?
		if constexpr(std::is_integral_v<T>)
		{
			std::uniform_int_distribution<T> dist(lowest, highest);
			return dist(rng);
		}
		else
		{
			std::uniform_real_distribution<T> dist(lowest, highest);
			return dist(rng);
		}
	}
	// Selects a random integer in range. If the condition accepts the result return it, otherwise iterate forward untill we find one that does or we have exhausted the range.
	int getInRangeWithCondition(int min, int max, auto&& condition)
	{
		int output = getInRange(min, max);
		int start = output;
		while(!condition(output))
		{
			if(output == max)
				output = 0;
			else if(output + 1 == start)
				return max + 1;
			else
				++output;
		}
		return output;
	}
	bool percentChance(const Percent& percent);
	bool chance(double chance);
	bool chance(float chance);
	Point3D getInCuboid(const Cuboid& cuboid);
	template<typename T>
	T getInEnum()
	{
		return (T)getInRange(0, (int32_t)T::Null - 1);
	}
	template<typename T>
	T* getInVector(const std::vector<T*>& vector)
	{
		return vector[getInRange((size_t)0, vector.size() - (size_t)1)];
	}
	template<typename T>
	const T& getInVector(const std::vector<T>& vector)
	{
		return vector[getInRange((size_t)0, vector.size() - (size_t)1)];
	}
	template<typename T>
	T& getInVector(std::vector<T>& vector)
	{
		return vector[getInRange((size_t)0, vector.size() - (size_t)1)];
	}
	template<typename T>
	const T* getInVector(const std::vector<const T*>& vector)
	{
		return vector[getInRange((size_t)0, vector.size() - (size_t)1)];
	}
	template<typename T>
	std::vector<T*> getMultipleInVector(const std::vector<T*>& vector, int32_t count)
	{
		assert(count <= vector.size());
		auto copy = vector;
		std::vector<T*> output;
		for(size_t i = 0; i < count; ++i)
		{
			size_t index = getInRange((size_t)0, copy.size() - (size_t)1);
			output.push_back(copy[index]);
			copy.erase(copy.begin() + index);
		}
		return output;
	}
	template<typename T>
	T applyRandomFuzzPlusOrMinusPercent(const T& input, const Percent& percent)
	{
		T output = input;
		int32_t maxPlus = percent.ratio() * (float)input.get();
		return input + getInRange(-maxPlus, maxPlus);
	}
	template<typename T>
	T applyRandomFuzzPlusOrMinusRatio(const T& input, const float& ratio)
	{
		int32_t maxPlus = ratio * (float)input.get();
		return input + getInRange(-maxPlus, maxPlus);
	}
};
