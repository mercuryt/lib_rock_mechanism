#include "leaveArea.h"
#include "../actor.h"
#include "../area.h"
#include "../simulation.h"
#include "../threadedTask.h"

LeaveAreaObjective::LeaveAreaObjective(Actor& a, uint8_t priority) : Objective(a, priority), m_task(a.getSimulation().m_threadedTaskEngine) { }
void LeaveAreaObjective::execute()
{
	if(actorIsAdjacentToEdge())
		// We are at the edge and can leave.
		m_actor.leaveArea();
	else
		m_task.create(*this);
	return;
}
bool LeaveAreaObjective::actorIsAdjacentToEdge() const
{
	Blocks& blocks = m_actor.m_area->getBlocks();
	return m_actor.predicateForAnyOccupiedBlock([&blocks](BlockIndex block){ return blocks.isEdge(block); });
}
LeaveAreaThreadedTask::LeaveAreaThreadedTask(LeaveAreaObjective& objective) : 
	ThreadedTask(objective.m_actor.getSimulation().m_threadedTaskEngine),
	m_objective(objective), m_findsPath(objective.m_actor, false) { }
void LeaveAreaThreadedTask::readStep()
{
	if(!m_objective.actorIsAdjacentToEdge())
		m_findsPath.pathToAreaEdge();
}
void LeaveAreaThreadedTask::writeStep()
{
	if(m_findsPath.found())
		m_objective.m_actor.m_canMove.setPath(m_findsPath.getPath());
	else if(m_objective.actorIsAdjacentToEdge())
		m_objective.m_actor.leaveArea();
	else
		m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
}
