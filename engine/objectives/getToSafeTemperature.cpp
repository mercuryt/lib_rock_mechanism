#include "getToSafeTemperature.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../path/pathRequest.h"
#include "../path/terrainFacade.hpp"
//TODO: Detour locked to true for emergency moves.
GetToSafeTemperaturePathRequest::GetToSafeTemperaturePathRequest(Area& area, GetToSafeTemperatureObjective& objective, const ActorIndex& actorIndex) :
	m_objective(objective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Distance::max();
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
	Space& space = area.getSpace();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	auto condition = [&actors, &space, actorIndex](const Point3D& location, const Facing4& facingAtLocation) ->std::pair<bool, Point3D>
	{
		for(const Cuboid& occupied : actors.getCuboidsWhichWouldBeOccupiedAtLocationAndFacing(actorIndex, location, facingAtLocation))
			for(const Point3D& point : occupied)
				//TODO: temperature_getMax(const CuboidSet& cuboids)
				if(!actors.temperature_isSafe(actorIndex, space.temperature_get(point)))
					return std::make_pair(false, Point3D::null());
		return std::make_pair(true, location);
	};
	const auto [startResult, startPoint] = condition(actors.getLocation(actorIndex), actors.getFacing(actorIndex));
	if(startResult)
		return {{}, startPoint, true};
	constexpr bool useAnyOccupiedPoint = false;
	constexpr bool useAdjacent = false;
	return terrainFacade.findPathToConditionBreadthFirst<decltype(condition), useAnyOccupiedPoint, useAdjacent>(condition, memo, actors.getLocation(actorIndex), actors.getFacing(actorIndex), actors.getShape(actorIndex), m_objective.m_detour, FactionId::null(), maxRange);
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
	Json output = PathRequestBreadthFirst::toJson();
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
		Space& space = area.getSpace();
		if(actors.predicateForAnyOccupiedCuboid(actor, [&space](const Cuboid& cuboid){ return space.isEdge(cuboid); }))
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
