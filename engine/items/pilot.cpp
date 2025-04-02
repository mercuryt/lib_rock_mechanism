#include "items.h"
void Items::pilot_set(const ItemIndex& item, const ActorIndex& pilot)
{
	assert(m_pilot[item].empty());
	assert(m_onDeck[item].contains(ActorOrItemIndex::createForActor(pilot)));
	m_pilot[item] = pilot;
}
void Items::pilot_clear(const ItemIndex& item)
{
	assert(m_pilot[item].exists());
	m_pilot[item].clear();
}
ActorIndex Items::pilot_get(const ItemIndex& item)
{
	return m_pilot[item];
}