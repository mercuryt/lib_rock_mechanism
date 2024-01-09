#include "stocks.h"
#include "item.h"
void AreaHasStocksForFaction::record(Item& item)
{
	m_data[&item.m_itemType][&item.m_materialType].insert(&item);
}
void AreaHasStocksForFaction::unrecord(Item& item)
{
	if(m_data.at(&item.m_itemType).at(&item.m_materialType).size() > 1)
		// There are more then one item for this combination of item type and material type.
		m_data[&item.m_itemType][&item.m_materialType].erase(&item);
	else
	{
		if(m_data.at(&item.m_itemType).size() > 1)
			// There is more then one materialType ( and thus more then one item ) for this item type.
			m_data[&item.m_itemType].erase(&item.m_materialType);
		else
			// This is the only item for this item type
			m_data.erase(&item.m_itemType);
	}
}
