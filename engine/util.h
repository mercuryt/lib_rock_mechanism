#pragma once

#include "types.h"

#include <algorithm>
#include <stack>
#include <vector>
#include <queue>
#include <cstdint>
#include <string>
#include <cassert>
#include <codecvt>
#include <locale>
#include <array>
#include <chrono>

namespace util
{
	inline uint64_t scaleByPercent(uint64_t base, Percent percent)
	{
		return (base * (uint64_t)percent.get()) / 100u;
	}
	inline uint32_t scaleByPercent(uint32_t base, Percent percent)
	{
		return (base * percent.get()) / 100u;
	}
	inline int scaleByPercent(int base, Percent percent)
	{
		return (base * percent.get()) / 100u;
	}
	inline float scaleByPercent(float base, Percent percent)
	{
		return (base * percent.get()) / 100u;
	}
	inline int scaleByInversePercent(int base, Percent percent)
	{
		return scaleByPercent(base, Percent::create(100 - percent.get()));
	}
	inline int scaleByFractionRange(int min, int max, int32_t numerator, int32_t denominator)
	{
		return min + (((max - min) * numerator) / denominator);
	}
	inline int scaleByPercentRange(int min, int max, Percent percent)
	{
		return scaleByFractionRange(min, max, percent.get(), 100u);
	}
	inline int scaleByFraction(int base, int32_t numerator, int32_t denominator)
	{
		return (base * numerator) / denominator;
	}
	inline int scaleByInverseFraction(int base, int32_t numerator, int32_t denominator)
	{
		return scaleByFraction(base, denominator - numerator, denominator);
	}
	inline Percent fractionToPercent(int numerator, int denominator)
	{
		return Percent::create(std::round(((float)numerator / (float)denominator) * 100.f));
	}
	template<typename T>
	inline bool sortedVectorContainsDuplicates(const std::vector<T>& vector)
	{
		auto it = std::adjacent_find(vector.begin(), vector.end());
		return it == vector.end();
	}
	//template<typename T>
	//struct AddressEquivalence{ bool operator()(T& other){ return &other == this; } };
	inline std::array<int32_t, 3> rotateOffsetToFacing(const std::array<int32_t, 3>& position, const Facing4& facing)
	{
		auto [x, y, z] = position;
		switch(facing)
		{
			case Facing4::North:
				return position;
			case Facing4::East:
				return {y*-1, x, z};
			case Facing4::South:
				return {x*-1, y*-1, z};
			case Facing4::West:
				return {y, x*-1, z};
			default:
				std::unreachable();
		}
	}
	inline double radiansToDegrees(double radians)
	{
		return radians * (180.0/3.141592653589793238463);
	}
	template<typename T>
	inline void removeFromVectorByIndexUnordered(std::vector<T>& vector, uint index)
	{
		std::swap(vector[index], vector.back());
		vector.pop_back();
	}
	template<typename T>
	void removeDuplicates(std::vector<T>& input)
	{
		std::sort(input.begin(), input.end());
		input.erase(std::unique(input.begin(), input.end()), input.end());
	}
	template<typename T>
	bool vectorContains(const std::vector<T>& vector, const T& value)
	{
		return std::ranges::find(vector, value) != vector.end();
	}
	template<typename T>
	void removeDuplicatesAndValue(std::vector<T>& vec, const T& valueToRemove)
	{
		std::vector<T> seen = {valueToRemove};
		vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const T& elem)
		{
			if (vectorContains(seen, elem))
				return true;
			seen.push_back(elem);
			return false;
		}), vec.end());
	};
	template<typename T>
	inline void removeFromVectorByValueUnordered(std::vector<T>& vector, const T& value)
	{
		auto iter = std::ranges::find(vector, value);
		(*iter) = std::move(vector.back());
		vector.pop_back();
	}
	template<typename T, int size>
	inline void removeFromArrayByValueUnordered(std::array<T, size>& array, const T& value)
	{
		auto iter = std::ranges::find(array, value);
		assert(iter != array.end());
		auto last = std::ranges::find(array, T::null()) - 1;
		if(last != iter)
			*iter = *last;
		*last = T::null();
	}
	template<typename T, int size>
	inline void removeFromArrayByIndexUnordered(std::array<T, size>& array, const uint& index)
	{
		auto lastIndex = (std::ranges::find(array, T::null()) - 1) - array.begin();
		if(lastIndex != index)
			array[index] = array[lastIndex];
		array[lastIndex] = T::null();
	}
	template<typename T>
	inline void addUniqueToVectorAssert(std::vector<T>& vector, const T& value)
	{
		assert(std::ranges::find(vector, value) == vector.end());
		vector.push_back(value);
	}
	template<typename T>
	inline void addUniqueToVectorIgnore(std::vector<T>& vector, const T& value)
	{
		if(std::ranges::find(vector, value) == vector.end())
			vector.push_back(value);
	}
	inline Facing4 getFacingForAdjacentOffset(uint8_t adjacentOffset)
	{
		switch(adjacentOffset)
		{
			case 6:
			case 7:
			case 8:
			case 12:
			case 14:
			case 15:
			case 23:
			case 24:
			case 25:
				return Facing4::West;
			case 0:
			case 1:
			case 2:
			case 9:
			case 10:
			case 16:
				return Facing4::East;
			case 5:
			case 11:
			case 19:
			case 22:
				return Facing4::South;
			default:
				return Facing4::North;

		}
	}
	inline std::chrono::microseconds getCurrentTimeInMicroSeconds()
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = now.time_since_epoch();
		return std::chrono::duration_cast<std::chrono::microseconds>(duration);
	}
	[[nodiscard]] inline std::string eigenToString(auto& eigenArrayOrMatrix)
	{
		std::stringstream ss;
		ss << eigenArrayOrMatrix;
		return ss.str();
	}
	inline Facing4 rotateFacingByDifference(const Facing4& facing, const Facing4& previousFacing, const Facing4& newFacing)
	{
		int difference = (int)newFacing - (int)previousFacing;
		if(difference < 0)
			difference += 4;
		int output = (int)facing + difference;
		if(output > 3)
			output -= 4;
		assert(output >= 0 && output < 4);
		return (Facing4)output;
	}
}
