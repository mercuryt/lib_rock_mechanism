#include "hasItems.h"
#include "../item.h"
#include "../area.h"
#include "types.h"
void AreaHasItems::setItemIsOnSurface(Item& item)
{
	//assert(!m_onSurface.contains(&item));
	m_onSurface.insert(&item);
}
void AreaHasItems::setItemIsNotOnSurface(Item& item)
{
	//assert(m_onSurface.contains(&item));
	m_onSurface.erase(&item);
}
void AreaHasItems::onChangeAmbiantSurfaceTemperature()
{
	//TODO: Optimize by not repetedly fetching ambiant.
	Blocks& blocks = m_area.getBlocks();
	for(Item* item : m_onSurface)
	{
		assert(item->m_location != BLOCK_INDEX_MAX);
		assert(blocks.isOutdoors(item->m_location));
		item->setTemperature(blocks.temperature_get(item->m_location));
	}
}
void AreaHasItems::add(Item& item)
{
	if(item.m_itemType.internalVolume)
		m_area.m_hasHaulTools.registerHaulTool(item);
}
void AreaHasItems::remove(Item& item)
{
	m_onSurface.erase(&item);
	m_area.m_hasStockPiles.removeItemFromAllFactions(item);
	if(item.m_itemType.internalVolume)
		m_area.m_hasHaulTools.unregisterHaulTool(item);
	item.m_reservable.clearAll();
}
