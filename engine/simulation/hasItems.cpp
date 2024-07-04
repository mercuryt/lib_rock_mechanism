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
