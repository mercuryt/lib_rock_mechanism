#include "actors.h"
#include "area.h"
void Actors::canReserve_clearAll(ActorIndex index)
{
	m_canReserve.at(index).deleteAllWithoutCallback();
}
void Actors::canReserve_setFaction(ActorIndex index, Faction* faction)
{
	m_canReserve.at(index).setFaction(faction);
}
void Actors::canReserve_reserveLocation(ActorIndex index, BlockIndex block)
{
	m_area.getBlocks().reserve(block, m_canReserve.at(index));
}
void Actors::canReserve_reserveItem(ActorIndex index, ItemIndex item)
{
	m_area.getItems().reservable_reserve(item, m_canReserve.at(index));
}
[[nodiscard]] bool Actors::canReserve_tryToReserveLocation(ActorIndex index, BlockIndex block)
{
	if(m_area.getBlocks().isReserved(block, *getFaction(index)))
		return false;
	canReserve_reserveLocation(index, block);
}
[[nodiscard]] bool Actors::canReserve_tryToReserveItem(ActorIndex index, ItemIndex item)
{
	if(m_area.getItems().reservable_isFullyReserved(item, *getFaction(index)))
		return false;
	canReserve_reserveItem(index, item);
}
[[nodiscard]] bool Actors::canReserve_factionExists(ActorIndex index) const
{
	//TODO: This does not check if any faction exists, either change or rename.
	return m_canReserve.at(index).factionExists();
}
[[nodiscard]] bool Actors::canReserve_hasReservationWith(ActorIndex index, Reservable& reservable) const
{
	return m_canReserve.at(index).hasReservationWith(reservable);
}
[[nodiscard]] bool Actors::canReserve_hasReservations(ActorIndex index) const
{
	return m_canReserve.at(index).hasReservations();
}
