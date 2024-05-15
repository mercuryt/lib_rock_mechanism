#include "hasItems.h"
#include "../item.h"
#include "../area.h"
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
	for(Item* item : m_onSurface)
	{
		assert(item->m_location != nullptr);
		assert(item->m_location->m_outdoors);
		item->setTemperature(item->m_location->m_blockHasTemperature.get());
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
