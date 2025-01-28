#include "uiUtil.h"
#include "../engine/items/items.h"
#include "../engine/area.h"
#include "../engine/materialType.h"
#include "../engine/util.h"

#include <format>

std::wstring UIUtil::describeItem(Area& area, const ItemIndex &item)
{
	Items& items = area.getItems();
	std::wstring output = MaterialType::getName(items.getMaterialType(item)) + L" " + ItemType::getName(items.getItemType(item));
	if(items.getQuantity(item) > 1)
		output += L" ( " + std::to_wstring(items.getQuantity(item).get()) + L" ) ";
	else if(!items.getName(item).empty())
		output += L" " + items.getName(item);
	return output;
}
std::wstring UIUtil::floatToString(const float& input)
{
	return std::format(L"{:.2f}", input);
}