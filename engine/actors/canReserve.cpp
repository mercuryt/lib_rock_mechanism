#include "actors.h"
#include "../area.h"
#include "../items/items.h"
#include "../blocks/blocks.h"
void Actors::canReserve_clearAll(ActorIndex index)
{
	if(m_canReserve[index] != nullptr)
		m_canReserve[index]->deleteAllWithoutCallback();
}
void Actors::canReserve_setFaction(ActorIndex index, FactionId faction)
{
	reservable_unreserveAll(index);
	if(faction.empty())
		m_canReserve[index] = nullptr;
	else
		m_canReserve[index]->setFaction(faction);
}
void Actors::canReserve_reserveLocation(ActorIndex index, BlockIndex block)
{
	m_area.getBlocks().reserve(block, *m_canReserve[index]);
}
void Actors::canReserve_reserveItem(ActorIndex index, ItemIndex item)
{
	m_area.getItems().reservable_reserve(item, *m_canReserve[index]);
}
bool Actors::canReserve_tryToReserveLocation(ActorIndex index, BlockIndex block)
{
	if(m_area.getBlocks().isReserved(block, getFactionId(index)))
		return false;
	canReserve_reserveLocation(index, block);
	return true;
}
bool Actors::canReserve_tryToReserveItem(ActorIndex index, ItemIndex item)
{
	if(m_area.getItems().reservable_isFullyReserved(item, m_faction[index]))
		return false;
	canReserve_reserveItem(index, item);
	return true;
}
bool Actors::canReserve_hasReservationWith(ActorIndex index, Reservable& reservable) const
{
	if(m_canReserve[index] == nullptr)
		return false;
	return m_canReserve[index]->hasReservationWith(reservable);
}
