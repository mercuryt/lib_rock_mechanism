#include "getToSafeTemperature.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../path/pathRequest.h"
#include "../path/areaHasPaths.hpp"
//TODO: Detour locked to true for emergency moves.
GetToSafeTemperaturePathRequest::GetToSafeTemperaturePathRequest(Area& area, GetToSafeTemperatureObjective& objective, const ActorIndex actorIndex) :
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
	PathRequest(data, area),
	m_objective(static_cast<GetToSafeTemperatureObjective&>(*deserializationMemo.m_objectives[data["objective"]]))
{ }
PathResult GetToSafeTemperaturePathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	Actors& actors = area.getActors();
	Space& space = area.getSpace();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	Temperature maxSafeTemperature = actors.temperature_getMaxSafe(actorIndex);
	Temperature minSafeTemperature = actors.temperature_getMinSafe(actorIndex);
	auto shortRangeCondition = [&actors, &space, actorIndex, maxSafeTemperature, minSafeTemperature](const Point3D location, const Facing4 facingAtLocation) -> Point3D
	{
		for(const Cuboid occupied : actors.getCuboidsWhichWouldBeOccupiedAtLocationAndFacing(actorIndex, location, facingAtLocation))
			for(const Point3D point : occupied)
			{
				Temperature pointTemperature = space.temperature_get(point);
				if(pointTemperature > maxSafeTemperature || pointTemperature < minSafeTemperature)
					return Point3D::null();
			}
		return location;
	};
	auto longRangeCondition = [&area, maxSafeTemperature, minSafeTemperature](const Cuboid cuboid) -> bool
	{
		auto [upperBound, lowerBound] = area.m_hasTemperature.upperAndLowerBounds(area, cuboid);
		return lowerBound <= maxSafeTemperature && upperBound >= minSafeTemperature;
	};
	constexpr bool useAnyOccupiedPoint = false;
	constexpr bool useAdjacent = false;
	return hasPaths.pathToCondition<useAdjacent, useAnyOccupiedPoint>(longRangeCondition, shortRangeCondition, toParamaters(area));
}
void GetToSafeTemperaturePathRequest::writeStep(Area& area, bool useCurrentLoction)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(path.empty() && !useCurrentLoction)
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
	else if(useCurrentLoction)
		// Current position is now safe.
		actors.objective_complete(actorIndex, m_objective);
	else
		// Safe position found, go there.
		actors.move_setPath(actorIndex, path);
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
void GetToSafeTemperatureObjective::execute(Area& area, const ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(m_noWhereWithSafeTemperatureFound)
	{
		Space& space = area.getSpace();
		if(actors.predicateForAnyOccupiedCuboid(actor, [&space](const Cuboid cuboid){ return space.isEdge(cuboid); }))
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
void GetToSafeTemperatureObjective::cancel(Area& area, const ActorIndex actor) { area.getActors().move_pathRequestMaybeCancel(actor); }
void GetToSafeTemperatureObjective::reset(Area& area, const ActorIndex actor)
{
	cancel(area, actor);
	m_noWhereWithSafeTemperatureFound = false;
	area.getActors().canReserve_clearAll(actor);
}
