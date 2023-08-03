#pragma once

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
	inline int scaleByPercent(int base, uint32_t percent)
	{
		return (base * percent) / 100u;
	}
	inline int scaleByInversePercent(int base, uint32_t percent)
	{
		return scaleByPercent(base, 100 - percent);
	}
	inline int scaleByPercentRange(int min, int max, uint32_t percent)
	{
		return min + (((max - min) * percent) / 100u);
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
	//template<typename T>
	//struct AddressEquivalence{ bool operator()(T& other){ return &other == this; } };
};
