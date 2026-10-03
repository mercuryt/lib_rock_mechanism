#include "joinExpedition.h"
#include "../area/area.h"
#include "../expedition/transitZone.h"
#include "../simulation/simulation.h"
#include "../simulation/hasAreas.h"
#include "../actors/actors.h"
#include "../path/areaHasPaths.hpp"

JoinExpeditionObjective::JoinExpeditionObjective(Priority priority, ExpeditionId expedition) :
	Objective(priority),
	m_expedition(expedition)
	{ }
void JoinExpeditionObjective::execute(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	Expedition& expedition = area.m_simulation.m_hasExpeditions.byId(m_expedition);
	const Space& space = area.getSpace();
	Facing6 facingFromAreaToExpedition = area.m_location.getFacing6Twords(expedition.location);
	Cuboid exitFace = space.boundry().getFace(facingFromAreaToExpedition);
	if(actors.getOccupied(actor).intersects(exitFace))
		// We are at the corrent edge and can leave.
		area.m_simulation.m_hasExpeditions.departAreaToJoinExpedition(area, actor, expedition);
	else
		actors.move_pathRequestRecord(actor, std::make_unique<JoinExpeditionPathRequest>(area, *this, actor));
}
JoinExpeditionPathRequest::JoinExpeditionPathRequest(Area& area, JoinExpeditionObjective& objective, ActorIndex actorIndex) :
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
PathResult JoinExpeditionPathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	Expedition& expedition = area.m_simulation.m_hasExpeditions.byId(m_objective.m_expedition);
	const Space& space = area.getSpace();
	Facing6 facingFromAreaToExpedition = area.m_location.getFacing6Twords(expedition.location);
	Cuboid exitFace = space.boundry().getFace(facingFromAreaToExpedition);
	auto longRangeCondition = [exitFace](Cuboid cuboid) -> bool { return cuboid.isTouchingFaceFromInside(exitFace); };
	auto shortRangeCondition = [this, &space, exitFace](Point3D point, Facing4 facingCandidate) -> Point3D
	{
		Cuboid shapeBoundry = Shape::getBoundryAtWithFacing(shape, space, point, facingCandidate);
		return shapeBoundry.isTouchingFaceFromInside(exitFace) ? point : Point3D::null();
	};
	constexpr bool anyOccupied = false;
	constexpr bool useAdjacent = false;
	return hasPaths.pathToCondition<anyOccupied, useAdjacent>(longRangeCondition, shortRangeCondition, toParamaters(area));
}
void JoinExpeditionPathRequest::writeStep(Area& area, bool useCurrentLocation)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(!path.empty())
		actors.move_setPath(actorIndex, path);
	else if(useCurrentLocation)
	{
		assert(actors.isOnEdge(actorIndex));
		m_objective.execute(area, actorIndex);
	}
	else
		actors.objective_canNotCompleteObjective(actorIndex, m_objective);
}
JoinExpeditionPathRequest::JoinExpeditionPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequest(data, area),
	m_objective(static_cast<JoinExpeditionObjective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
Json JoinExpeditionPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = "join expedition";
	return output;
}
