#include "station.h"
#include "block.h"
void StationObjective::execute()
{
	if(m_actor.m_location != &m_location)
		// Block, detour, adjacent, unreserved, reserve
		m_actor.m_canMove.setDestination(m_location, m_detour, false, false, true);
}
void StationObjective::reset() { m_actor.m_canReserve.clearAll(); }
