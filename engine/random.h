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
	//TODO: Unify getInRange as a template?
	int getInRange(int lowest, int highest);
	long getInRange(long lowest, long highest);
	unsigned int getInRange(unsigned int lowest, unsigned int highest);
	unsigned long getInRange(unsigned long lowest, unsigned long highest);
	float getInRange(float lowest, float highest);
	double getInRange(double lowest, double highest);
	bool percentChance(const Percent& percent);
	bool chance(double chance);
	bool chance(float chance);
	Point3D getInCuboid(const Cuboid& cuboid);
	template<typename T>
		T* getInVector(const std::vector<T*>& vector)
		{
			return vector.at(getInRange((size_t)0, vector.size() - (size_t)1));
		}
	template<typename T>
		const T& getInVector(const std::vector<T>& vector)
		{
			return vector.at(getInRange((size_t)0, vector.size() - (size_t)1));
		}
	template<typename T>
		T& getInVector(std::vector<T>& vector)
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
				output.push_back(copy[index]);
				copy.erase(copy.begin() + index);
			}
			return output;
		}
	template<typename T>
	T applyRandomFuzzPlusOrMinusPercent(const T& input, const Percent& percent)
	{
		T output = input;
		int maxPlus = percent.ratio() * (float)input.get();
		return input + getInRange(-maxPlus, maxPlus);
	}
	template<typename T>
	T applyRandomFuzzPlusOrMinusRatio(const T& input, const float& ratio)
	{
		T output = input;
		int maxPlus = ratio * (float)input.get();
		return input + getInRange(-maxPlus, maxPlus);
	}
};
