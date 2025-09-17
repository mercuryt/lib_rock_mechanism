#include "leaveArea.h"
#include "../area/area.h"
#include "../simulation/simulation.h"
#include "../actors/actors.h"
#include "../path/terrainFacade.hpp"

LeaveAreaObjective::LeaveAreaObjective(Priority priority) :
	Objective(priority) { }
void LeaveAreaObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(actors.isOnEdge(actor))
		// We are at the edge and can leave.
		actors.leaveArea(actor);
	else
		actors.move_pathRequestRecord(actor, std::make_unique<LeaveAreaPathRequest>(area, *this, actor));
}
LeaveAreaPathRequest::LeaveAreaPathRequest(Area& area, LeaveAreaObjective& objective, const ActorIndex& actorIndex) :
	m_objective(objective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForDigDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_objective.m_detour;
	adjacent = true;
}
FindPathResult LeaveAreaPathRequest::readStep(Area&, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	return terrainFacade.findPathToEdge(memo, start, facing, shape, m_objective.m_detour);
}
void LeaveAreaPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(!result.path.empty())
		actors.move_setPath(actorIndex, result.path);
	else if(actors.isOnEdge(actorIndex))
		actors.leaveArea(actorIndex);
	else
		actors.objective_canNotCompleteObjective(actorIndex, m_objective);
}
LeaveAreaPathRequest::LeaveAreaPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<LeaveAreaObjective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
Json LeaveAreaPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = "leave area";
	return output;
}
