#pragma once

#include <string>
#include <cstdint>

class Area;
class ItemIndex;
class Temperature;

namespace UIUtil
{
	std::string describeItem(Area& area, const ItemIndex& item);
	std::string floatToString(const float& input);
	std::string temperatureToString(const Temperature& temperature);
}
