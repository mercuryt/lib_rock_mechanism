#include "getToSafeTemperature.h"
#include "../area.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../pathRequest.h"
#include "../terrainFacade.hpp"
//TODO: Detour locked to true for emergency moves.
GetToSafeTemperaturePathRequest::GetToSafeTemperaturePathRequest(Area& area, GetToSafeTemperatureObjective& objective, const ActorIndex& actorIndex) :
	m_objective(objective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_objective.m_detour;
}
GetToSafeTemperaturePathRequest::GetToSafeTemperaturePathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<GetToSafeTemperatureObjective&>(*deserializationMemo.m_objectives[data["objective"]]))
{ }
FindPathResult GetToSafeTemperaturePathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	auto condition = [&actors, &blocks, actorIndex](const BlockIndex& location, const Facing& facing) ->std::pair<bool, BlockIndex>
	{
		for(BlockIndex adjacent : actors.getBlocksWhichWouldBeOccupiedAtLocationAndFacing(actorIndex, location, facing))
			if(actors.temperature_isSafe(actorIndex, blocks.temperature_get(adjacent)))
			return std::make_pair(true, location);
		return std::make_pair(false, BlockIndex::null());
	};
	constexpr FactionId faction;
	constexpr bool adjacent = false;
	// Multi tile creatures at at a safe temperature if their location block is. This could be improved.
	constexpr bool useAnyOccupiedBlock = false;
	return terrainFacade.findPathToConditionBreadthFirst<useAnyOccupiedBlock, decltype(condition)>(condition, memo, actors.getLocation(actorIndex), actors.getFacing(actorIndex), actors.getShape(actorIndex), m_objective.m_detour, adjacent, faction, maxRange);
}
void GetToSafeTemperaturePathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty() && !result.useCurrentPosition)
	{
		if(m_objective.m_noWhereWithSafeTemperatureFound)
			// Cannot leave area.
			actors.objective_canNotFulfillNeed(actorIndex, m_objective);
		else
		{
			// Try to leave area.
			m_objective.m_noWhereWithSafeTemperatureFound = true;
			m_objective.execute(area, actorIndex);
		}
	}
	else if(result.useCurrentPosition)
		// Current position is now safe.
		actors.objective_complete(actorIndex, m_objective);
	else
		// Safe position found, go there.
		actors.move_setPath(actorIndex, result.path);
}
Json GetToSafeTemperaturePathRequest::toJson() const
{
	Json output = static_cast<const PathRequestBreadthFirst&>(*this);
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
