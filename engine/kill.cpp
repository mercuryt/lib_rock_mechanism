#include "kill.h"
#include "block.h"
#include "objective.h"
#include "visionRequest.h"
#include <memory>
void KillInputAction::execute()
{
	std::unique_ptr<Objective> objective = std::make_unique<KillObjective>(m_killer, m_target);
	insertObjective(std::move(objective), m_killer);
}
void KillObjective::execute()
{
	if(!m_target.m_alive)
		//TODO: Do we need to cancel the threaded task here?
		m_killer.m_hasObjectives.objectiveComplete(*this);
	m_killer.m_canFight.setTarget(m_target);
	// If not in range create GetIntoRangeThreadedTask.
	if(!m_getIntoRangeAndLineOfSightThreadedTask.exists() && 
			(m_killer.m_location->taxiDistance(*m_target.m_location) > m_killer.m_canFight.getMaxRange() ||
			 // TODO: hasLineOfSightIncludingActors
			 VisionRequest::hasLineOfSightBasic(*m_killer.m_location, *m_target.m_location))
	)
		m_getIntoRangeAndLineOfSightThreadedTask.create(m_killer, m_target, m_killer.m_canFight.getMaxRange());
	else
		// If in range and has line of sight and attack not on cooldown then attack.
		if(!m_killer.m_canFight.isOnCoolDown())
			m_killer.m_canFight.attackMeleeRange(m_target);
}
void KillObjective::reset() 
{ 
	cancel(); 
	m_killer.m_canReserve.clearAll(); 
}
