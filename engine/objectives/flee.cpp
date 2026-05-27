#include "flee.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../path/areaHasPaths.hpp"

// Objective.
FleeObjective::FleeObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo) {}
void FleeObjective::execute(Area& area, const ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(actors.vision_canSeeEnemy(actor))
		actors.move_pathRequestRecord(actor, std::make_unique<FleePathRequest>(area, *this, actor));
	else
		// Actor has fled far enough.
		actors.objective_complete(actor, *this);
}
void FleeObjective::cancel(Area& area, const ActorIndex actor) { area.getActors().move_pathRequestMaybeCancel(actor); }
void FleeObjective::reset(Area& area, const ActorIndex actor) { cancel(area, actor); }
// Path Request.
//TODO: Detour locked to true for emergency moves.
FleePathRequest::FleePathRequest(Area& area, FleeObjective& objective, const ActorIndex actorIndex) :
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
FleePathRequest::FleePathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequest(data, area),
	m_objective(static_cast<FleeObjective&>(*deserializationMemo.m_objectives[data["objective"]]))
{ }
PathResult FleePathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	Actors& actors = area.getActors();
	Space& space = area.getSpace();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	const Point3D enemyLocation = actors.getNearestVisibleEnemyLocation(actorIndex);
	const Point3D actorLocation = actors.getLocation(actorIndex);
	Distance targetDistance = Distance::create((actorLocation.distanceToFractional(enemyLocation) * 1.5).get());
	auto shortRangeCondition = [&actors, &space, actorIndex, enemyLocation, targetDistance](const Point3D proposedDestination, const Facing4) -> Point3D
	{
		if(proposedDestination.distanceTo(enemyLocation) >= targetDistance)
			return proposedDestination;
		else
			return Point3D::null();
	};
	auto longRangeCondition = [enemyLocation, targetDistance](const Cuboid cuboid) -> bool
	{
		Point3D farthestFromEnemyInCuboid = cuboid.furthestPointFrom(enemyLocation);
		return farthestFromEnemyInCuboid.distanceTo(enemyLocation) >= targetDistance;
	};
	constexpr bool useAnyOccupiedPoint = false;
	constexpr bool useAdjacent = false;
	return hasPaths.pathToCondition<useAdjacent, useAnyOccupiedPoint>(longRangeCondition, shortRangeCondition, toParamaters(area));
}
void FleePathRequest::writeStep(Area& area, bool useCurrentLocation)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(path.empty() && !useCurrentLocation)
		// No escape.
		// TODO: freeze in place and set a callback to run FleeObjective::execute again.
		actors.objective_canNotFulfillNeed(actorIndex, m_objective);
	else if(useCurrentLocation)
		// Current position is now safe.
		actors.objective_complete(actorIndex, m_objective);
	else
		// Safe position found, go there.
		actors.move_setPath(actorIndex, path);
}
Json FleePathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = "flee";
	return output;
}