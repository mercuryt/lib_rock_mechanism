#include "actors.h"
#include "../area.h"
#include "../items/items.h"
#include "../blocks/blocks.h"
void Actors::canReserve_clearAll(ActorIndex index)
{
	m_canReserve.at(index)->deleteAllWithoutCallback();
}
void Actors::canReserve_setFaction(ActorIndex index, Faction* faction)
{
	reservable_unreserveAll(index);
	if(faction == nullptr)
		m_canReserve.at(index) = nullptr;
	else
		m_canReserve.at(index)->setFaction(*faction);
}
void Actors::canReserve_reserveLocation(ActorIndex index, BlockIndex block)
{
	m_area.getBlocks().reserve(block, *m_canReserve.at(index));
}
void Actors::canReserve_reserveItem(ActorIndex index, ItemIndex item)
{
	m_area.getItems().reservable_reserve(item, *m_canReserve.at(index));
}
bool Actors::canReserve_tryToReserveLocation(ActorIndex index, BlockIndex block)
{
	if(m_area.getBlocks().isReserved(block, *getFaction(index)))
		return false;
	canReserve_reserveLocation(index, block);
	return true;
}
bool Actors::canReserve_tryToReserveItem(ActorIndex index, ItemIndex item)
{
	if(m_area.getItems().reservable_isFullyReserved(item, *getFaction(index)))
		return false;
	canReserve_reserveItem(index, item);
	return true;
}
bool Actors::canReserve_hasReservationWith(ActorIndex index, Reservable& reservable) const
{
	if(m_canReserve.at(index) == nullptr)
		return false;
	return m_canReserve.at(index)->hasReservationWith(reservable);
}
