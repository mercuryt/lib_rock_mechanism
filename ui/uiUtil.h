#pragma once

#include <string>
#include <cstdint>

class Area;
class ItemIndex;
class Temperature;

namespace UIUtil
{
	std::string describeItem(Area& area, const ItemIndex& item);
	std::string floattoS(const float& input);
	std::string temperaturetoS(const Temperature& temperature);
}
