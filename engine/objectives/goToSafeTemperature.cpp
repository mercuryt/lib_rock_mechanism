#include "goToSafeTemperature.h"
#include "area.h"
#include "pathRequest.h"
//TODO: Detour locked to true for emergency moves.
GetToSafeTemperaturePathRequest::GetToSafeTemperaturePathRequest(Area& area, GetToSafeTemperatureObjective& o) : m_objective(o)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	std::function<bool(BlockIndex, Facing facing)> condition = [this, &actors, &blocks](BlockIndex location, Facing facing)
	{
		for(BlockIndex adjacent : actors.getBlocksWhichWouldBeOccupiedAtLocationAndFacing(m_objective.m_actor, location, facing))
			if(actors.temperature_isSafe(m_objective.m_actor, blocks.temperature_get(adjacent)))
				return true;
		return false;
	};
	bool unreserved = false;
	createGoToCondition(area, m_objective.m_actor, condition, m_objective.m_detour, unreserved, BLOCK_DISTANCE_MAX);
}
void GetToSafeTemperaturePathRequest::callback(Area& area, FindPathResult result)
{
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
	{
		if(m_objective.m_noWhereWithSafeTemperatureFound)
			// Cannot leave area.
			actors.objective_canNotFulfillNeed(m_objective.m_actor, m_objective);
		else
		{
			// Try to leave area.
			m_objective.m_noWhereWithSafeTemperatureFound = true;
			m_objective.execute(area);
		}
	}
	if(result.useCurrentPosition)
		// Current position is now safe.
		actors.objective_complete(m_objective.m_actor, m_objective);
	else
		// Safe position found, go there.
		actors.move_setPath(m_objective.m_actor, result.path);
}
GetToSafeTemperatureObjective::GetToSafeTemperatureObjective(ActorIndex a) :
	Objective(a, Config::getToSafeTemperaturePriority) { }
/*
GetToSafeTemperatureObjective::GetToSafeTemperatureObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_getToSafeTemperaturePathRequest(deserializationMemo.m_simulation.m_threadedTaskEngine), m_noWhereWithSafeTemperatureFound(data["noWhereSafeFound"].get<bool>())
{
	if(data.contains("threadedTask"))
		m_getToSafeTemperaturePathRequest.create(*this);
}
Json GetToSafeTemperatureObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noWhereSafeFound"] = m_noWhereWithSafeTemperatureFound;
	if(m_getToSafeTemperaturePathRequest.exists())
		data["threadedTask"] = true;
	return data;
}
*/
void GetToSafeTemperatureObjective::execute(Area& area)
{
	Actors& actors = area.getActors();
	if(m_noWhereWithSafeTemperatureFound)
	{
		Blocks& blocks = area.getBlocks();
		if(actors.predicateForAnyOccupiedBlock(m_actor, [blocks](BlockIndex block){ return blocks.isEdge(block); }))
			// We are at the edge and can leave.
			actors.leaveArea(m_actor);
		else
			// No safe temperature and no escape.
			actors.objective_canNotFulfillNeed(m_actor, *this);
		return;
	}
	if(actors.temperature_isSafeAtCurrentLocation(m_actor))
		actors.objective_complete(m_actor, *this);
	else
		actors.move_pathRequestRecord(m_actor, std::make_unique<GetToSafeTemperaturePathRequest>(area, *this));
}
void GetToSafeTemperatureObjective::cancel(Area& area) { area.getActors().move_pathRequestMaybeCancel(m_actor); }
void GetToSafeTemperatureObjective::reset(Area& area) 
{ 
	cancel(area); 
	m_noWhereWithSafeTemperatureFound = false; 
	area.getActors().canReserve_clearAll(m_actor);
}
//GetToSafeTemperatureObjective::~GetToSafeTemperatureObjective() { m_actor.m_needsSafeTemperature.m_objectiveExists = false; }
