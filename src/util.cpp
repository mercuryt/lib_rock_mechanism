#include "util.h"
int util::scaleByPercent(int base, uint32_t percent)
{
	return (base * percent) / 100u;
}
int util::scaleByInversePercent(int base, uint32_t percent)
{
	return scaleByPercent(base, 100 - percent);
}
int util::scaleByPercentRange(int min, int max, uint32_t percent)
{
	return min + (((max - min) * percent) / 100u);
}
int util::scaleByFraction(int base, uint32_t numerator, uint32_t denominator)
{
	return (base * numerator) / denominator;
}
int util::scaleByInverseFraction(int base, uint32_t numerator, uint32_t denominator)
{
	return scaleByFraction(base, denominator - numerator, denominator);
}

std::string util::wideStringToString(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}
