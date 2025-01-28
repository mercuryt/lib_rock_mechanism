#pragma once

#include <string>
#include <cstdint>

class Area;
class ItemIndex;

namespace UIUtil
{
	std::wstring describeItem(Area& area, const ItemIndex& item);
	std::wstring floatToString(const float& input);
}
