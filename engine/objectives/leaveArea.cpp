#include "leaveArea.h"
#include "../area.h"
#include "../simulation.h"
#include "../actors/actors.h"
#include "terrainFacade.h"

LeaveAreaObjective::LeaveAreaObjective(ActorIndex a, uint8_t priority) :
	Objective(a, priority) { }
void LeaveAreaObjective::execute(Area& area)
{
	Actors& actors = area.getActors();
	if(actors.isOnEdge(m_actor))
		// We are at the edge and can leave.
		actors.leaveArea(m_actor);
	else
		actors.move_pathRequestRecord(m_actor, std::make_unique<LeaveAreaPathRequest>(area, *this));
	return;
}
LeaveAreaPathRequest::LeaveAreaPathRequest(Area& area, LeaveAreaObjective& objective) : m_objective(objective)
{
	createGoToEdge(area, objective.m_actor, m_objective.m_detour);
}
void LeaveAreaPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	if(!result.path.empty())
		actors.move_setPath(m_objective.m_actor, result.path);
	else if(m_objective.actorIsAdjacentToEdge())
		actors.leaveArea(m_objective.m_actor);
	else
		actors.objective_canNotCompleteObjective(m_objective.m_actor, m_objective);
}
