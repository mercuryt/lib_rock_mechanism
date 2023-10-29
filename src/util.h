#pragma once

#include "types.h"

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
	inline int scaleByPercent(uint64_t base, Percent percent)
	{
		return (base * (uint64_t)percent) / 100u;
	}
	inline int scaleByPercent(uint32_t base, Percent percent)
	{
		return (base * percent) / 100u;
	}
	inline int scaleByPercent(int base, Percent percent)
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
	//template<typename T>
	//struct AddressEquivalence{ bool operator()(T& other){ return &other == this; } };
};
