#include "hasItems.h"
#include "../items/items.h"
void SimulationHasItems::registerItem(const ItemId& id, Items& store, const ItemIndex& index)
{
	assert(!m_items.contains(id));
	m_items.try_emplace(id, &store, index);
}
void SimulationHasItems::removeItem(const ItemId& id)
{
	assert(m_items.contains(id));
	m_items.erase(id);
}
ItemIndex SimulationHasItems::getIndexForId(const ItemId& id) const
{
	return m_items.at(id).index;
}
Area& SimulationHasItems::getAreaForId(const ItemId& id) const
{
	return m_items.at(id).store->getArea();
}
