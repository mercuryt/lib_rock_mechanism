#pragma once

#include "types.h"

#include <algorithm>
#include <unordered_set>
#include <stack>
#include <vector>
#include <queue>
#include <cstdint>
#include <string>
#include <cassert>
#include <codecvt>
#include <locale>

namespace util
{
	inline uint64_t scaleByPercent(uint64_t base, Percent percent)
	{
		return (base * (uint64_t)percent) / 100u;
	}
	inline uint32_t scaleByPercent(uint32_t base, Percent percent)
	{
		return (base * percent) / 100u;
	}
	inline int scaleByPercent(int base, Percent percent)
	{
		return (base * percent) / 100u;
	}
	inline float scaleByPercent(float base, Percent percent)
	{
		return (base * percent) / 100u;
	}
	inline int scaleByInversePercent(int base, Percent percent)
	{
		return scaleByPercent(base, 100 - percent);
	}
	inline int scaleByFractionRange(int min, int max, uint32_t numerator, uint32_t denominator)
	{
		return min + (((max - min) * numerator) / denominator);
	}
	inline int scaleByPercentRange(int min, int max, Percent percent)
	{
		return scaleByFractionRange(min, max, percent, 100u);
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
	[[maybe_unused]]inline int fractionToPercent(int numerator, int denominator)
	{
		return ((float)numerator / (float)denominator) * 100;
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
};
