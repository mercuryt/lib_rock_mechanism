#include "stocks.h"
#include "itemType.h"
#include "materialType.h"
#include "area.h"
#include "items/items.h"
void AreaHasStocksForFaction::record(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	m_data.getOrCreate(items.getItemType(item)).getOrCreate(items.getMaterialType(item)).add(item);
}
void AreaHasStocksForFaction::unrecord(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	if(m_data[items.getItemType(item)][items.getMaterialType(item)].size() > 1)
		// There are more then one item for this combination of item type and material type.
		m_data[items.getItemType(item)][items.getMaterialType(item)].remove(item);
	else
	{
		if(m_data[items.getItemType(item)].size() > 1)
			// There is more then one materialType ( and thus more then one item ) for this item type.
			m_data[items.getItemType(item)].erase(items.getMaterialType(item));
		else
			// This is the only item for this item type
			m_data.erase(items.getItemType(item));
	}
}
