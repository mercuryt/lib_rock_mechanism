#include "construct.h"
#include "../area/area.h"
#include "../projects/construct.h"
#include "../path/areaHasPaths.hpp"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../reference.h"
#include "../numericTypes/types.h"
// PathRequest.
ConstructPathRequest::ConstructPathRequest(Area& area, ConstructObjective& co, const ActorIndex actorIndex) :
	m_constructObjective(co)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForDigDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_constructObjective.m_detour;
	adjacent = true;
	reserveDestination = true;
}
ConstructPathRequest::ConstructPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequest(data, area),
	m_constructObjective(static_cast<ConstructObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())))
{ }
PathResult ConstructPathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	ActorIndex actorIndex = actor.getIndex(area.getActors().m_referenceData);
	auto shortRangeCondition = [&area, &constThis = std::as_const(*this), actorIndex](const Cuboid cuboid) -> Point3D { return constThis.m_constructObjective.joinableProjectExistsAt(area, cuboid, actorIndex); };
	auto longRangeCondition = [&shortRangeCondition](const Cuboid cuboid) -> bool { return shortRangeCondition(cuboid).exists(); };
	constexpr bool useAdjacent = true;
	constexpr bool useAnyPoint = false;
	return hasPaths.pathToConditionWithDesignation<useAdjacent, useAnyPoint>(SpaceDesignation::Construct, longRangeCondition, shortRangeCondition, toParamaters(area));
}
void ConstructPathRequest::writeStep(Area& area, bool useCurrentLocation)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(path.empty() && !useCurrentLocation)
		actors.objective_canNotCompleteObjective(actorIndex, m_constructObjective);
	else
	{
		// No need to reserve here, we are just checking for access.
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(actors.getFaction(actorIndex), target);
		if(project.canAddWorker(actorIndex))
			m_constructObjective.joinProject(project, actorIndex);
		else
			// Project can no longer accept this worker, try again.
			m_constructObjective.execute(area, actorIndex);
	}
}
Json ConstructPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = &m_constructObjective;
	output["type"] = "construct";
	return output;
}
// Objective.
ConstructObjective::ConstructObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_project(data.contains("project") ? deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()) : nullptr) { }
Json ConstructObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = m_project;
	return data;
}
void ConstructObjective::execute(Area& area, const ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(m_project != nullptr)
		m_project->commandWorker(actor);
	else
	{
		ConstructProject* project = nullptr;
		const FactionId& faction = actors.getFaction(actor);
		//TODO: optimize for single tile and other single cuboid shapes by using a Cuboid rather then a CuboidSet for adjacent.
		const CuboidSet& adjacent = actors.getAdjacentCuboids(actor);
		const auto condition = [&](const ConstructProject& p) { return p.canAddWorker(actor); };
		project = area.m_hasConstructionDesignations.getProjectWithCondition(faction, adjacent, condition);
		if(project != nullptr)
			joinProject(*project, actor);
		else
			actors.move_pathRequestRecord(actor, std::make_unique<ConstructPathRequest>(area, *this, actor));
	}
}
void ConstructObjective::cancel(Area& area, const ActorIndex actor)
{
	if(m_project != nullptr)
		m_project->removeWorker(actor);
	area.getActors().move_pathRequestMaybeCancel(actor);
}
void ConstructObjective::delay(Area& area, const ActorIndex actor)
{
	cancel(area, actor);
	m_project = nullptr;
	area.getActors().project_maybeUnset(actor);
}
void ConstructObjective::reset(Area& area, const ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(m_project)
	{
		ActorReference ref = area.getActors().getReference(actor);
		assert(!m_project->getWorkers().contains(ref));
		m_project = nullptr;
		actors.project_unset(actor);
	}
	else
		assert(!actors.project_exists(actor));
	area.getActors().move_pathRequestMaybeCancel(actor);
	actors.canReserve_clearAll(actor);
}
void ConstructObjective::onProjectCannotReserve(Area&, const ActorIndex)
{
	assert(m_project);
	m_cannotJoinWhileReservationsAreNotComplete.insert(m_project);
}
void ConstructObjective::joinProject(ConstructProject& project, const ActorIndex actor)
{
	assert(m_project == nullptr);
	m_project = &project;
	project.addWorkerCandidate(actor, *this);
}
ConstructProject* ConstructObjective::getProjectWhichActorCanJoinAdjacentTo(Area& area, const Point3D location, const Facing4 facing, const ActorIndex actor)
{
	Actors& actors = area.getActors();
	for(const Cuboid adjacentCuboid : actors.getAdjacentCuboidsAtLocationWithFacing(actor, location, facing))
	{
		ConstructProject* project = getProjectWhichActorCanJoinAt(area, adjacentCuboid, actor);
		if(project != nullptr)
			return project;
	}
	return nullptr;
}
const ConstructProject* ConstructObjective::getProjectWhichActorCanJoinAdjacentTo(Area& area, const Point3D location, const Facing4 facing, const ActorIndex actor) const
{
	return const_cast<ConstructObjective*>(this)->getProjectWhichActorCanJoinAdjacentTo(area, location, facing, actor);
}
ConstructProject* ConstructObjective::getProjectWhichActorCanJoinAt(Area& area, const Cuboid cuboid, const ActorIndex actor)
{
	const Actors& actors = area.getActors();
	const auto condition = [&](const ConstructProject& project)
	{
		if(!project.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains((Project*)&project))
			return false;
		if(project.canAddWorker(actor))
			return true;
		return false;
	};
	return area.m_hasConstructionDesignations.getProjectWithCondition(actors.getFaction(actor), cuboid, condition);
}
const ConstructProject* ConstructObjective::getProjectWhichActorCanJoinAt(Area& area, const Cuboid cuboid, const ActorIndex actor) const
{
	return const_cast<ConstructObjective*>(this)->getProjectWhichActorCanJoinAt(area, cuboid, actor);
}
Point3D ConstructObjective::joinableProjectExistsAt(Area& area, const Cuboid cuboid, const ActorIndex actor) const
{
	const ConstructProject* project = getProjectWhichActorCanJoinAt(area, cuboid, actor);
	if(project != nullptr)
		return project->getLocation();
	return Point3D::null();
}
bool ConstructObjective::canJoinProjectAdjacentToLocationAndFacing(Area& area, const Point3D location, const Facing4 facing, const ActorIndex actor) const
{
	return const_cast<ConstructObjective*>(this)->getProjectWhichActorCanJoinAdjacentTo(area, location, facing, actor) != nullptr;
}
// ObjectiveType.
bool ConstructObjectiveType::canBeAssigned(Area& area, const ActorIndex actor) const
{
	Actors& actors = area.getActors();
	// Pilots and passengers onDeck cannot construct.
	if(actors.mount_exists(actor))
		return false;
	return area.m_hasConstructionDesignations.areThereAnyForFaction(actors.getFaction(actor));
}
std::unique_ptr<Objective> ConstructObjectiveType::makeFor(Area&, const ActorIndex) const { return std::make_unique<ConstructObjective>(); }
