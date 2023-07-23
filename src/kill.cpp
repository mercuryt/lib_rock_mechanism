#include "kill.h"
#include "block.h"
void KillObjective::execute()
{
	if(!m_target.m_alive)
		//TODO: Do we need to cancel the threaded task here?
		m_killer.m_hasObjectives.objectiveComplete(*this);
	m_killer.m_canFight.setTarget(m_target);
	// If not in range create GetIntoRangeThreadedTask.
	if(!m_getIntoRangeAndLineOfSightThreadedTask.exists() && m_killer.m_location->taxiDistance(*m_target.m_location) > m_killer.m_canFight.getMaxRange())
		m_getIntoRangeAndLineOfSightThreadedTask.create(m_killer, m_target.m_location, m_killer.m_canFight.getMaxRange());
	else
		// If in range and attack not on cooldown then attack.
		if(!m_killer.m_canFight.isOnCoolDown())
			m_killer.m_canFight.attack(m_target);
}
