#include "dig.h"
#include "../dig.h"
#include "../area.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../terrainFacade.h"
#include "reference.h"
#include "types.h"
DigPathRequest::DigPathRequest(Area& area, DigObjective& digObjective, ActorIndex actor) : m_digObjective(digObjective)
{
	std::function<bool(BlockIndex)> predicate = [&area, this, actor](BlockIndex block)
	{
		return m_digObjective.getJoinableProjectAt(area, block, actor) != nullptr;
	};
	DistanceInBlocks maxRange = Config::maxRangeToSearchForDigDesignations;
	bool unreserved = true;
	//TODO: We don't need the whole path here, just the destination and facing.
	createGoAdjacentToCondition(area, actor, predicate, m_digObjective.m_detour, unreserved, maxRange, BLOCK_INDEX_MAX);
}
void DigPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(actor, m_digObjective);
	else
	{
		if(result.useCurrentPosition)
		{
			if(!actors.move_tryToReserveOccupied(actor))
			{
				actors.objective_canNotCompleteSubobjective(actor);
				return;
			}
		}
		else if(!actors.move_tryToReserveProposedDestination(actor, result.path))
		{
			actors.objective_canNotCompleteSubobjective(actor);
			return;
		}
		BlockIndex target = result.blockThatPassedPredicate;
		DigProject& project = area.m_hasDigDesignations.at(actors.getFactionId(actor), target);
		if(project.canAddWorker(actor))
			m_digObjective.joinProject(project, actor);
		else
		{
			// Project can no longer accept this worker, try again.
			actors.objective_canNotCompleteSubobjective(actor);
			return;
		}
	}
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
void DigObjective::execute(Area& area, ActorIndex actor)
{
	if(m_project != nullptr)
		m_project->commandWorker(actor);
	else
	{
		Actors& actors = area.getActors();
		DigProject* project = nullptr;
		std::function<bool(BlockIndex)> predicate = [&area, this, project, &actors, actor](BlockIndex block) mutable
		{ 
			if(!getJoinableProjectAt(area, block, actor))
				return false;
			project = &area.m_hasDigDesignations.at(actors.getFactionId(actor), block);
			if(project->canAddWorker(actor))
				return true;
			return false;
		};
		[[maybe_unused]] BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(actor, predicate);
		if(project != nullptr)
		{
			assert(adjacent != BLOCK_INDEX_MAX);
			joinProject(*project, actor);
			return;
		}
		std::unique_ptr<PathRequest> request = std::make_unique<DigPathRequest>(area, *this, actor);
		actors.move_pathRequestRecord(actor, std::move(request));
	}
}
void DigObjective::cancel(Area& area, ActorIndex actor)
{
	if(m_project != nullptr)
		m_project->removeWorker(actor);
	area.getActors().move_pathRequestMaybeCancel(actor);
}
void DigObjective::delay(Area& area, ActorIndex actor) 
{ 
	cancel(area, actor); 
	m_project = nullptr;
	area.getActors().project_unset(actor);
}
void DigObjective::reset(Area& area, ActorIndex actor) 
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
void DigObjective::onProjectCannotReserve(Area&, ActorIndex)
{
	assert(m_project);
	m_cannotJoinWhileReservationsAreNotComplete.insert(m_project);
}
void DigObjective::joinProject(DigProject& project, ActorIndex actor)
{
	assert(!m_project);
	m_project = &project;
	project.addWorkerCandidate(actor, *this);
}
DigProject* DigObjective::getJoinableProjectAt(Area& area, BlockIndex block, ActorIndex actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	FactionId faction = actors.getFactionId(actor);
	if(!blocks.designation_has(block, faction, BlockDesignation::Dig))
		return nullptr;
	DigProject& output = area.m_hasDigDesignations.at(faction, block);
	if(!output.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&output))
		return nullptr;
	if(!output.canAddWorker(actor))
		return nullptr;
	return &output;
}
bool DigObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	//TODO: check for any picks?
	return area.m_hasDigDesignations.areThereAnyForFaction(area.getActors().getFactionId(actor));
}
std::unique_ptr<Objective> DigObjectiveType::makeFor(Area&, ActorIndex) const
{
	std::unique_ptr<Objective> objective = std::make_unique<DigObjective>();
	return objective;
}
