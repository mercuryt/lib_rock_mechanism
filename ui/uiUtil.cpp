#include "uiUtil.h"
#include "../engine/item.h"
#include "../engine/materialType.h"
#include "../engine/util.h"

std::wstring UIUtil::describeItem(const Item &item)
{
	std::wstring output = util::stringToWideString(item.m_materialType.name + " " + item.m_itemType.name);
	if(item.getQuantity() > 1)
		output += L" ( " + std::to_wstring(item.getQuantity()) + L" ) ";
	else if(!item.m_name.empty())
		output += L" " + item.m_name;
	return output;
}
