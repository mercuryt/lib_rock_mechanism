#include "getToSafeTemperature.h"
#include "../area.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../pathRequest.h"
//TODO: Detour locked to true for emergency moves.
GetToSafeTemperaturePathRequest::GetToSafeTemperaturePathRequest(Area& area, GetToSafeTemperatureObjective& o, const ActorIndex& actor) : m_objective(o)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	DestinationCondition condition = [&actors, &blocks, actor](const BlockIndex& location, const Facing& facing)
	{
		for(BlockIndex adjacent : actors.getBlocksWhichWouldBeOccupiedAtLocationAndFacing(actor, location, facing))
			if(actors.temperature_isSafe(actor, blocks.temperature_get(adjacent)))
			return std::make_pair(true, location);
		return std::make_pair(false, BlockIndex::null());
	};
	bool unreserved = false;
	createGoToCondition(area, actor, condition, m_objective.m_detour, unreserved, DistanceInBlocks::max());
}
GetToSafeTemperaturePathRequest::GetToSafeTemperaturePathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	PathRequest(data),
	m_objective(static_cast<GetToSafeTemperatureObjective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
void GetToSafeTemperaturePathRequest::callback(Area& area, const FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(result.path.empty() && !result.useCurrentPosition)
	{
		if(m_objective.m_noWhereWithSafeTemperatureFound)
			// Cannot leave area.
			actors.objective_canNotFulfillNeed(actor, m_objective);
		else
		{
			// Try to leave area.
			m_objective.m_noWhereWithSafeTemperatureFound = true;
			m_objective.execute(area, actor);
		}
	}
	else if(result.useCurrentPosition)
		// Current position is now safe.
		actors.objective_complete(actor, m_objective);
	else
		// Safe position found, go there.
		actors.move_setPath(actor, result.path);
}
Json GetToSafeTemperaturePathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = "temperature";
	return output;
}
GetToSafeTemperatureObjective::GetToSafeTemperatureObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_noWhereWithSafeTemperatureFound(data["noWhereSafeFound"].get<bool>()) { }
Json GetToSafeTemperatureObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noWhereSafeFound"] = m_noWhereWithSafeTemperatureFound;
	return data;
}
void GetToSafeTemperatureObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(m_noWhereWithSafeTemperatureFound)
	{
		Blocks& blocks = area.getBlocks();
		if(actors.predicateForAnyOccupiedBlock(actor, [&blocks](const BlockIndex& block){ return blocks.isEdge(block); }))
			// We are at the edge and can leave.
			actors.leaveArea(actor);
		else
			// No safe temperature and no escape.
			actors.objective_canNotFulfillNeed(actor, *this);
		return;
	}
	if(actors.temperature_isSafeAtCurrentLocation(actor))
		actors.objective_complete(actor, *this);
	else
		actors.move_pathRequestRecord(actor, std::make_unique<GetToSafeTemperaturePathRequest>(area, *this, actor));
}
void GetToSafeTemperatureObjective::cancel(Area& area, const ActorIndex& actor) { area.getActors().move_pathRequestMaybeCancel(actor); }
void GetToSafeTemperatureObjective::reset(Area& area, const ActorIndex& actor) 
{ 
	cancel(area, actor); 
	m_noWhereWithSafeTemperatureFound = false; 
	area.getActors().canReserve_clearAll(actor);
}
