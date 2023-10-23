#include "station.h"
#include "block.h"
void StationObjective::execute()
{
	if(m_actor.m_location != &m_location)
		m_actor.m_canMove.setDestination(m_location, m_detour);
}
void StationObjective::reset() { m_actor.m_canReserve.clearAll(); }
