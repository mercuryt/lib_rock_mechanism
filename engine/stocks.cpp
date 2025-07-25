#include "stocks.h"
#include "definitions/itemType.h"
#include "definitions/materialType.h"
#include "area/area.h"
#include "items/items.h"
void AreaHasStocksForFaction::record(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	const ItemTypeId& itemType = items.getItemType(item);
	const MaterialTypeId& materialType = items.getMaterialType(item);
	assert(!m_data.contains(itemType) || !m_data[itemType].contains(materialType) || !m_data[itemType][materialType].contains(item));
	m_data.getOrCreate(itemType).getOrCreate(materialType).insert(item);
}
void AreaHasStocksForFaction::maybeRecord(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	const ItemTypeId& itemType = items.getItemType(item);
	const MaterialTypeId& materialType = items.getMaterialType(item);
	if(!m_data.contains(itemType) || !m_data[itemType].contains(materialType) || !m_data[itemType][materialType].contains(item))
		m_data.getOrCreate(itemType).getOrCreate(materialType).insert(item);

}
void AreaHasStocksForFaction::unrecord(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	if(m_data[items.getItemType(item)][items.getMaterialType(item)].size() > 1)
		// There are more then one item for this combination of item type and material type.
		m_data[items.getItemType(item)][items.getMaterialType(item)].erase(item);
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
