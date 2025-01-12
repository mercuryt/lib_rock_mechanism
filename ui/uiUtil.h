#pragma once

#include <string>

class Area;
class ItemIndex;

namespace UIUtil
{
	std::wstring describeItem(Area& area, const ItemIndex& item);
}
