#include "construct.h"
#include "../area.h"
#include "../construct.h"
#include "../terrainFacade.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "types.h"
// PathRequest.
ConstructPathRequest::ConstructPathRequest(Area& area, ConstructObjective& co, ActorIndex actor) : m_constructObjective(co)
{
	std::function<bool(BlockIndex)> constructCondition = [&](BlockIndex block)
	{
		return m_constructObjective.joinableProjectExistsAt(area, block, actor);
	};
	bool unreserved = true;
	DistanceInBlocks maxRange = Config::maxRangeToSearchForConstructionDesignations;
	createGoAdjacentToCondition(area, actor, constructCondition, m_constructObjective.m_detour, unreserved, maxRange, BLOCK_INDEX_MAX);
}
void ConstructPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(m_actor, m_constructObjective);
	else
	{
		if(result.useCurrentPosition)
		{
			if(!actors.move_tryToReserveOccupied(m_constructObjective.m_actor))
			{
				// Proposed location while constructing has been reserved already, try to find another.
				m_constructObjective.execute(area);
				return;
			}
		}
		else if(!actors.move_tryToReserveProposedDestination(m_constructObjective.m_actor, result.path))
		{
			// Proposed location while constructing has been reserved already, try to find another.
			m_constructObjective.execute(area);
			return;
		}
		BlockIndex target = result.blockThatPassedPredicate;
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(*actors.getFaction(m_constructObjective.m_actor), target);
		if(project.canAddWorker(m_constructObjective.m_actor))
			m_constructObjective.joinProject(project);
		else
			// Project can no longer accept this worker, try again.
			m_constructObjective.execute(area);
	}
}
// Objective.
	/*
ConstructObjective::ConstructObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_constructPathRequest(m_actor.m_area->m_simulation.m_threadedTaskEngine),
	m_project(data.contains("project") ? deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()) : nullptr)
{
	if(data.contains("threadedTask"))
		m_constructPathRequest.create(*this);
}
Json ConstructObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = m_project;
	if(m_constructPathRequest.exists())
		data["threadedTask"] = true;
	return data;
}
*/
void ConstructObjective::execute(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(m_project != nullptr)
		m_project->commandWorker(actor);
	else
	{
		ConstructProject* project = nullptr;
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block)
		{
			if(joinableProjectExistsAt(area, block))
			{
				project = &area.m_hasConstructionDesignations.getProject(actors.getFactionId(actor), block);
				return project->canAddWorker(actor);
			}
			return false;
		};
		[[maybe_unused]] BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(actor, predicate);
		if(project != nullptr)
		{
			assert(adjacent != BLOCK_INDEX_MAX);
			joinProject(*project, actor);
			return;
		}
		std::unique_ptr<PathRequest> request = std::make_unique<ConstructPathRequest>(area, *this, actor);
		actors.move_pathRequestRecord(actor, std::move(request));
	}
}
void ConstructObjective::cancel(Area& area, ActorIndex actor)
{
	if(m_project != nullptr)
		m_project->removeWorker(actor);
	area.getActors().move_pathRequestMaybeCancel(actor);
}
void ConstructObjective::delay(Area& area, ActorIndex actor)
{
	cancel(area, actor);
	m_project = nullptr;
	area.getActors().project_unset(actor);
}
void ConstructObjective::reset(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(m_project)
	{
		assert(!m_project->getWorkers().contains(actor));
		m_project = nullptr;
		actors.project_unset(actor);
	}
	else
		assert(!actors.project_exists(actor));
	area.getActors().move_pathRequestMaybeCancel(actor);
	actors.canReserve_clearAll(actor);
}
void ConstructObjective::onProjectCannotReserve(Area&, ActorIndex)
{
	assert(m_project);
	m_cannotJoinWhileReservationsAreNotComplete.insert(m_project);
}
void ConstructObjective::joinProject(ConstructProject& project, ActorIndex actor)
{
	assert(m_project == nullptr);
	m_project = &project;
	project.addWorkerCandidate(actor, *this);
}
ConstructProject* ConstructObjective::getProjectWhichActorCanJoinAdjacentTo(Area& area, BlockIndex location, Facing facing, ActorIndex actor)
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
ConstructProject* ConstructObjective::getProjectWhichActorCanJoinAt(Area& area, BlockIndex block, ActorIndex actor)
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
bool ConstructObjective::joinableProjectExistsAt(Area& area, BlockIndex block, ActorIndex actor) const
{
	return const_cast<ConstructObjective*>(this)->getProjectWhichActorCanJoinAt(area, block, actor) != nullptr;
}
bool ConstructObjective::canJoinProjectAdjacentToLocationAndFacing(Area& area, BlockIndex location, Facing facing, ActorIndex actor) const
{
	return const_cast<ConstructObjective*>(this)->getProjectWhichActorCanJoinAdjacentTo(area, location, facing, actor) != nullptr;
}
// ObjectiveType.
bool ConstructObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	return area.m_hasConstructionDesignations.areThereAnyForFaction(*area.getActors().getFaction(actor));
}
std::unique_ptr<Objective> ConstructObjectiveType::makeFor(Area&, ActorIndex actor) const { return std::make_unique<ConstructObjective>(actor); }
