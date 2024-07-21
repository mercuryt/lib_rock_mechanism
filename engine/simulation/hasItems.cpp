#include "hasItems.h"
#include "../items/items.h"
void SimulationHasItems::registerItem(ItemId id, Items& store, ItemIndex index)
{
	assert(!m_items.contains(id));
	auto [iter, result] = m_items.try_emplace(id, store, index);
	assert(result);
}
void SimulationHasItems::removeItem(ItemId id)
{
	assert(m_items.contains(id));
	m_items.erase(id);
}
ItemIndex SimulationHasItems::getIndexForId(ItemId id) const
{
	return m_items.at(id).index;
}
Area& SimulationHasItems::getAreaForId(ItemId id) const
{
	return m_items.at(id).store.m_area;
}
