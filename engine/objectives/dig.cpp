#include "dig.h"
#include "../projects/dig.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../path/terrainFacade.hpp"
#include "../reference.h"
#include "../numericTypes/types.h"
#include "../path/pathRequest.h"
DigPathRequest::DigPathRequest(Area& area, DigObjective& digObjective, const ActorIndex& actorIndex) :
	m_digObjective(digObjective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForDigDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_digObjective.m_detour;
	adjacent = true;
	reserveDestination = true;
}
FindPathResult DigPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	ActorIndex actorIndex = actor.getIndex(area.getActors().m_referenceData);
	auto predicate = [&area, this, actorIndex](const Cuboid& cuboid) -> bool
	{
		return m_digObjective.joinableProjectExistsAt(area, cuboid, actorIndex).exists();
	};
	constexpr bool useAnyPoint = false;
	constexpr bool useAdjacent = true;
	constexpr bool unreserved = false;
	return terrainFacade.findPathToSpaceDesignationAndCondition<decltype(predicate), useAnyPoint, useAdjacent>(predicate, memo, SpaceDesignation::Dig, faction, start, facing, shape, detour, unreserved, maxRange);
}
DigPathRequest::DigPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_digObjective(static_cast<DigObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))){ }
void DigPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(actorIndex, m_digObjective);
	else
	{
		// No need to reserve here, we are just checking for access.
		Point3D target = result.pointThatPassedPredicate;
		DigProject& project = area.m_hasDigDesignations.getForFactionAndPoint(actors.getFaction(actorIndex), target);
		if(project.canAddWorker(actorIndex))
			m_digObjective.joinProject(project, actorIndex);
		else
		{
			// Project can no longer accept this worker, try again.
			actors.objective_canNotCompleteSubobjective(actorIndex);
			return;
		}
	}
}
Json DigPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = &m_digObjective;
	output["type"] = "dig";
	return output;
}
DigObjective::DigObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_project(data.contains("project") ? static_cast<DigProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) : nullptr) { }
Json DigObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = m_project;
	return data;
}
void DigObjective::execute(Area& area, const ActorIndex& actor)
{
	if(m_project != nullptr)
		m_project->commandWorker(actor);
	else
	{
		Actors& actors = area.getActors();
		const CuboidSet& adjacent = actors.getAdjacentCuboids(actor);
		DigProject* project = getJoinableProjectAt(area, adjacent, actor);
		if(project != nullptr)
			joinProject(*project, actor);
		else
			actors.move_pathRequestRecord(actor, std::make_unique<DigPathRequest>(area, *this, actor));
	}
}
void DigObjective::cancel(Area& area, const ActorIndex& actor)
{
	if(m_project != nullptr)
		m_project->removeWorker(actor);
	area.getActors().move_pathRequestMaybeCancel(actor);
}
void DigObjective::delay(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
	m_project = nullptr;
	area.getActors().project_maybeUnset(actor);
}
void DigObjective::reset(Area& area, const ActorIndex& actor)
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
void DigObjective::onProjectCannotReserve(Area&, const ActorIndex&)
{
	assert(m_project);
	m_cannotJoinWhileReservationsAreNotComplete.insert(m_project);
}
void DigObjective::joinProject(DigProject& project, const ActorIndex& actor)
{
	assert(!m_project);
	m_project = &project;
	project.addWorkerCandidate(actor, *this);
}
template<typename ShapeT>
DigProject* DigObjective::getJoinableProjectAt(Area& area, const ShapeT& shape, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	const auto condition = [&](const DigProject& project)
	{
		if(!project.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains((DigProject*)&project))
			return false;
		return project.canAddWorker(actor);
	};
	return area.m_hasDigDesignations.getProjectWithCondition(actors.getFaction(actor), shape, condition);
}
template DigProject* DigObjective::getJoinableProjectAt(Area& area, const Cuboid& shape, const ActorIndex& actor);
template DigProject* DigObjective::getJoinableProjectAt(Area& area, const CuboidSet& shape, const ActorIndex& actor);
Point3D DigObjective::joinableProjectExistsAt(Area& area, const Cuboid& cuboid, const ActorIndex& actor) const
{
	const DigProject* project = const_cast<DigObjective*>(this)->getJoinableProjectAt(area, cuboid, actor);
	if(project != nullptr)
		return project->getLocation();
	return Point3D::null();
}
bool DigObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	// Pilots and passengers onDeck cannot dig.
	if(actors.mount_exists(actor))
		return false;
	//TODO: check for any picks?
	return area.m_hasDigDesignations.areThereAnyForFaction(actors.getFaction(actor));
}
std::unique_ptr<Objective> DigObjectiveType::makeFor(Area&, const ActorIndex&) const
{
	std::unique_ptr<Objective> objective = std::make_unique<DigObjective>();
	return objective;
}
