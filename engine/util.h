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
	inline int scaleByFractionRange(int min, int max, uint32_t numerator, uint32_t denominator)
	{
		return min + (((max - min) * numerator) / denominator);
	}
	inline int scaleByPercentRange(int min, int max, Percent percent)
	{
		return scaleByFractionRange(min, max, percent.get(), 100u);
	}
	inline int scaleByFraction(int base, uint32_t numerator, uint32_t denominator)
	{
		return (base * numerator) / denominator;
	}
	inline int scaleByInverseFraction(int base, uint32_t numerator, uint32_t denominator)
	{
		return scaleByFraction(base, denominator - numerator, denominator);
	}
	inline std::string wideStringToString(const std::wstring& wstr)
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;
		return converterX.to_bytes(wstr);
	}
	inline std::wstring stringToWideString(const std::string& str)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(str);
	}
	[[maybe_unused]]inline Percent fractionToPercent(int numerator, int denominator)
	{
		return Percent::create(((float)numerator / (float)denominator) * 100);
	}
	template<typename T>
	inline bool sortedVectorContainsDuplicates(std::vector<T> vector)
	{
		auto it = std::adjacent_find(vector.begin(), vector.end());
		return it == vector.end();
	}
	//template<typename T>
	//struct AddressEquivalence{ bool operator()(T& other){ return &other == this; } };
	inline std::array<int32_t, 3> rotateOffsetToFacing(const std::array<int32_t, 3>& position, Facing facing)
	{
		auto [x, y, z] = position;
		if(facing == 0)
			return position;
		if(facing == 1)
			return {y, x, z};
		if(facing == 2)
			return {x, y*-1, z};
		assert(facing ==  3);
		return {y * -1, x, z};
	}
	inline double radiansToDegrees(double radians)
	{
		return radians * (180.0/3.141592653589793238463);
	}
	template<typename T>
	inline void removeFromVectorByIndexUnordered(std::vector<T>& vector, size_t index)
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
	bool vectorContains(const std::vector<T>& vector,const  T& value) 
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
	inline Facing getFacingForAdjacentOffset(uint8_t adjacentOffset)
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
				return Facing::create(3);
			case 0:
			case 1:
			case 2:
			case 9:
			case 10:
			case 16:
				return Facing::create(1);
			case 5:
			case 11:
			case 19:
			case 22:
				return Facing::create(2);
			default:
				return Facing::create(0);

		}
	}
}
