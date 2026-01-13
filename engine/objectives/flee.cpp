#include "flee.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../path/terrainFacade.hpp"

// Objective.
FleeObjective::FleeObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo) {}
void FleeObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(actors.vision_canSeeEnemy(actor))
		actors.move_pathRequestRecord(actor, std::make_unique<FleePathRequest>(area, *this, actor));
	else
		// Actor has fled far enough.
		actors.objective_complete(actor, *this);
}
void FleeObjective::cancel(Area& area, const ActorIndex& actor) { area.getActors().move_pathRequestMaybeCancel(actor); }
void FleeObjective::reset(Area& area, const ActorIndex& actor) { cancel(area, actor); }
// Path Request.
//TODO: Detour locked to true for emergency moves.
FleePathRequest::FleePathRequest(Area& area, FleeObjective& objective, const ActorIndex& actorIndex) :
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
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<FleeObjective&>(*deserializationMemo.m_objectives[data["objective"]]))
{ }
FindPathResult FleePathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Actors& actors = area.getActors();
	Space& space = area.getSpace();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	const Point3D& enemyLocation = actors.getNearestVisibleEnemyLocation(actorIndex);
	const Point3D& location = actors.getLocation(actorIndex);
	Distance targetDistance = Distance::create((location.distanceToFractional(enemyLocation) * 1.5).get());
	auto destinationCondition = [&actors, &space, actorIndex, enemyLocation, targetDistance](const Point3D& proposedDestination, const Facing4&) -> std::pair<bool, Point3D>
	{
		if(proposedDestination.distanceTo(enemyLocation) >= targetDistance)
			return std::make_pair(true, proposedDestination);
		else
			return std::make_pair(false, Point3D::null());
	};
	const auto [startResult, startPoint] = destinationCondition(actors.getLocation(actorIndex), actors.getFacing(actorIndex));
	if(startResult)
		return {{}, startPoint, true};
	constexpr bool useAnyOccupiedPoint = false;
	constexpr bool useAdjacent = false;
	return terrainFacade.findPathToConditionBreadthFirst<decltype(destinationCondition), useAnyOccupiedPoint, useAdjacent>(destinationCondition, memo, actors.getLocation(actorIndex), actors.getFacing(actorIndex), actors.getShape(actorIndex), m_objective.m_detour, FactionId::null(), maxRange);
}
void FleePathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty() && !result.useCurrentPosition)
		// No escape.
		// TODO: freeze in place and set a callback to run FleeObjective::execute again.
		actors.objective_canNotFulfillNeed(actorIndex, m_objective);
	else if(result.useCurrentPosition)
		// Current position is now safe.
		actors.objective_complete(actorIndex, m_objective);
	else
		// Safe position found, go there.
		actors.move_setPath(actorIndex, result.path);
}
Json FleePathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = "flee";
	return output;
}