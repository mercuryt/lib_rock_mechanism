#pragma once

#include <string>
#include <cstdint>

class Area;
class ItemIndex;
class Temperature;

namespace UIUtil
{
	std::wstring describeItem(Area& area, const ItemIndex& item);
	std::wstring floatToString(const float& input);
	std::wstring temperatureToString(const Temperature& temperature);
}
