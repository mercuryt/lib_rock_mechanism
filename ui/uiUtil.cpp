#include "uiUtil.h"
#include "../engine/items/items.h"
#include "../engine/area/area.h"
#include "../engine/definitions/materialType.h"
#include "../engine/util.h"

#include <format>

std::string UIUtil::describeItem(Area& area, const ItemIndex &item)
{
	Items& items = area.getItems();
	std::string output = MaterialType::getName(items.getMaterialType(item)) + " " + ItemType::getName(items.getItemType(item));
	if(items.getQuantity(item) > 1)
		output += " ( " + std::to_string(items.getQuantity(item).get()) + " ) ";
	else if(!items.getName(item).empty())
		output += " " + items.getName(item);
	return output;
}
std::string UIUtil::floatToString(const float& input)
{
	return std::format("{:.2f}", input);
}
std::string UIUtil::temperatureToString(const Temperature& temperature)
{
	return std::to_string(temperature.get()) + "°";
}