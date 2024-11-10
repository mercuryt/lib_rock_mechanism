#include "construct.h"
#include "../area.h"
#include "../construct.h"
#include "../terrainFacade.h"
#include "../hasShapes.hpp"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "reference.h"
#include "types.h"
// PathRequest.
ConstructPathRequest::ConstructPathRequest(Area& area, ConstructObjective& co, const ActorIndex& actor) : m_constructObjective(co)
{
	std::function<bool(BlockIndex)> constructCondition = [&, actor](BlockIndex block)
	{
		return m_constructObjective.joinableProjectExistsAt(area, block, actor);
	};
	bool unreserved = true;
	DistanceInBlocks maxRange = Config::maxRangeToSearchForConstructionDesignations;
	createGoAdjacentToCondition(area, actor, constructCondition, m_constructObjective.m_detour, unreserved, maxRange, BlockIndex::null());
}
ConstructPathRequest::ConstructPathRequest(const Json& data, DeserializationMemo& deserializationMemo) : 
	m_constructObjective(static_cast<ConstructObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())))
{
	nlohmann::from_json(data, *this);
}
void ConstructPathRequest::callback(Area& area, const FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(actor, m_constructObjective);
	else
	{
		// No need to reserve here, we are just checking for access.
		BlockIndex target = result.blockThatPassedPredicate;
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(actors.getFactionId(actor), target);
		if(project.canAddWorker(actor))
			m_constructObjective.joinProject(project, actor);
		else
			// Project can no longer accept this worker, try again.
			m_constructObjective.execute(area, actor);
	}
}
Json ConstructPathRequest::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output["objective"] = &m_constructObjective;
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
void ConstructObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(m_project != nullptr)
		m_project->commandWorker(actor);
	else
	{
		ConstructProject* project = nullptr;
		std::function<bool(const BlockIndex&)> predicate = [&](const BlockIndex& block)
		{
			if(joinableProjectExistsAt(area, block, actor))
			{
				project = &area.m_hasConstructionDesignations.getProject(actors.getFactionId(actor), block);
				return project->canAddWorker(actor);
			}
			return false;
		};
		[[maybe_unused]] BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(actor, predicate);
		if(project != nullptr)
		{
			assert(adjacent.exists());
			joinProject(*project, actor);
			return;
		}
		std::unique_ptr<PathRequest> request = std::make_unique<ConstructPathRequest>(area, *this, actor);
		actors.move_pathRequestRecord(actor, std::move(request));
	}
}
void ConstructObjective::cancel(Area& area, const ActorIndex& actor)
{
	if(m_project != nullptr)
		m_project->removeWorker(actor);
	area.getActors().move_pathRequestMaybeCancel(actor);
}
void ConstructObjective::delay(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
	m_project = nullptr;
	area.getActors().project_maybeUnset(actor);
}
void ConstructObjective::reset(Area& area, const ActorIndex& actor)
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
void ConstructObjective::onProjectCannotReserve(Area&, const ActorIndex&)
{
	assert(m_project);
	m_cannotJoinWhileReservationsAreNotComplete.insert(m_project);
}
void ConstructObjective::joinProject(ConstructProject& project, const ActorIndex& actor)
{
	assert(m_project == nullptr);
	m_project = &project;
	project.addWorkerCandidate(actor, *this);
}
ConstructProject* ConstructObjective::getProjectWhichActorCanJoinAdjacentTo(Area& area, const BlockIndex& location, const Facing& facing, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	for(BlockIndex adjacent : actors.getAdjacentBlocksAtLocationWithFacing(actor, location, facing))
	{
		ConstructProject* project = getProjectWhichActorCanJoinAt(area, adjacent, actor);
		if(project != nullptr)
			return project;
	}
	return nullptr;
}
ConstructProject* ConstructObjective::getProjectWhichActorCanJoinAt(Area& area, const BlockIndex& block, const ActorIndex& actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	if(!blocks.designation_has(block, actors.getFactionId(actor), BlockDesignation::Construct))
		return nullptr;
	ConstructProject& project = area.m_hasConstructionDesignations.getProject(actors.getFactionId(actor), block);
	if(!project.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&project))
		return nullptr;
	if(project.canAddWorker(actor))
		return &project;
	return nullptr;
}
bool ConstructObjective::joinableProjectExistsAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const
{
	return const_cast<ConstructObjective*>(this)->getProjectWhichActorCanJoinAt(area, block, actor) != nullptr;
}
bool ConstructObjective::canJoinProjectAdjacentToLocationAndFacing(Area& area, const BlockIndex& location, const Facing& facing, const ActorIndex& actor) const
{
	return const_cast<ConstructObjective*>(this)->getProjectWhichActorCanJoinAdjacentTo(area, location, facing, actor) != nullptr;
}
// ObjectiveType.
bool ConstructObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	return area.m_hasConstructionDesignations.areThereAnyForFaction(area.getActors().getFactionId(actor));
}
std::unique_ptr<Objective> ConstructObjectiveType::makeFor(Area&, const ActorIndex&) const { return std::make_unique<ConstructObjective>(); }
