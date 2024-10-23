#include "leaveArea.h"
#include "../area.h"
#include "../simulation.h"
#include "../actors/actors.h"

LeaveAreaObjective::LeaveAreaObjective(Priority priority) :
	Objective(priority) { }
void LeaveAreaObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(actors.isOnEdge(actor))
		// We are at the edge and can leave.
		actors.leaveArea(actor);
	else
		actors.move_pathRequestRecord(actor, std::make_unique<LeaveAreaPathRequest>(area, *this));
}
LeaveAreaPathRequest::LeaveAreaPathRequest(Area& area, LeaveAreaObjective& objective) : m_objective(objective)
{
	createGoToEdge(area, getActor(), m_objective.m_detour);
}
void LeaveAreaPathRequest::callback(Area& area, const FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(!result.path.empty())
		actors.move_setPath(actor, result.path);
	else if(actors.isOnEdge(actor))
		actors.leaveArea(actor);
	else
		actors.objective_canNotCompleteObjective(actor, m_objective);
}
LeaveAreaPathRequest::LeaveAreaPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	m_objective(static_cast<LeaveAreaObjective&>(*deserializationMemo.m_objectives[data["objective"]]))
{
	nlohmann::from_json(data, static_cast<PathRequest&>(*this));
}
Json LeaveAreaPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	return output;
}
