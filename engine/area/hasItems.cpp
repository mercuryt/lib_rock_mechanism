#include "hasItems.h"
#include "../items/items.h"
#include "../area.h"
#include "../blocks/blocks.h"
#include "../itemType.h"
#include "types.h"
void AreaHasItems::setItemIsOnSurface(ItemIndex item)
{
	//assert(!m_onSurface.contains(&item));
	m_onSurface.insert(item);
}
void AreaHasItems::setItemIsNotOnSurface(ItemIndex item)
{
	//assert(m_onSurface.contains(&item));
	m_onSurface.erase(item);
}
void AreaHasItems::onChangeAmbiantSurfaceTemperature()
{
	//TODO: Optimize by not repetedly fetching ambiant.
	Blocks& blocks = m_area.getBlocks();
	Items& items = m_area.getItems();
	for(ItemIndex item : m_onSurface)
	{
		assert(items.getLocation(item) != BLOCK_INDEX_MAX);
		assert(blocks.isOutdoors(items.getLocation(item)));
		items.setTemperature(item, blocks.temperature_get(items.getLocation(item)));
	}
}
void AreaHasItems::add(ItemIndex item)
{
	Items& items = m_area.getItems();
	if(items.getItemType(item).internalVolume)
		m_area.m_hasHaulTools.registerHaulTool(m_area, item);
}
void AreaHasItems::remove(ItemIndex item)
{
	Items& items = m_area.getItems();
	m_onSurface.erase(item);
	m_area.m_hasStockPiles.removeItemFromAllFactions(item);
	if(items.getItemType(item).internalVolume)
		m_area.m_hasHaulTools.unregisterHaulTool(item);
	items.reservable_unreserveAll(item);
}
