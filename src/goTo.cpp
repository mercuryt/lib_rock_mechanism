#include "goTo.h"
#include "block.h"
void GoToObjective::execute()
{
	if(m_actor.m_location != &m_location)
		// Block, detour, adjacent, unreserved, reserve
		m_actor.m_canMove.setDestination(m_location, m_detour, false, false, false);
	else
		m_actor.m_hasObjectives.objectiveComplete(*this);
}
