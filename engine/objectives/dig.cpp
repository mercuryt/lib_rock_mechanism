#include "dig.h"
#include "../dig.h"
#include "../area.h"
#include "../terrainFacade.h"
DigPathRequest::DigPathRequest(Area& area, DigObjective& digObjective) : m_digObjective(digObjective)
{
	std::function<bool(BlockIndex)> predicate = [&area, this](BlockIndex block)
	{
		return m_digObjective.getJoinableProjectAt(area, block) != nullptr;
	};
	DistanceInBlocks maxRange = Config::maxRangeToSearchForDigDesignations;
	bool unreserved = true;
	//TODO: We don't need the whole path here, just the destination and facing.
	createGoAdjacentToCondition(area, m_digObjective.m_actor, predicate, m_digObjective.m_detour, unreserved, maxRange, BLOCK_INDEX_MAX);
}
void DigPathRequest::callback(Area& area, FindPathResult result)
{
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(m_digObjective.m_actor, m_digObjective);
	else
	{
		if(result.useCurrentPosition)
		{
			if(!actors.move_tryToReserveOccupied(m_digObjective.m_actor))
			{
				actors.objective_canNotCompleteSubobjective(m_digObjective.m_actor);
				return;
			}
		}
		else if(!actors.move_tryToReserveProposedDestination(m_digObjective.m_actor, result.path))
		{
			actors.objective_canNotCompleteSubobjective(m_digObjective.m_actor);
			return;
		}
		BlockIndex target = result.blockThatPassedPredicate;
		DigProject& project = area.m_hasDigDesignations.at(*actors.getFaction(m_digObjective.m_actor), target);
		if(project.canAddWorker(m_digObjective.m_actor))
			m_digObjective.joinProject(project);
		else
		{
			// Project can no longer accept this worker, try again.
			actors.objective_canNotCompleteSubobjective(m_digObjective.m_actor);
			return;
		}
	}
}
DigObjective::DigObjective(ActorIndex a) : Objective(a, Config::digObjectivePriority) { }
/*
DigObjective::DigObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_digPathRequest(m_actor.m_area->m_simulation.m_threadedTaskEngine), 
	m_project(data.contains("project") ? deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()) : nullptr)
{
	if(data.contains("threadedTask"))
		m_digPathRequest.create(*this);
}
Json DigObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = m_project;
	if(m_digPathRequest.exists())
		data["threadedTask"] = true;
	return data;
}
*/
void DigObjective::execute(Area& area)
{
	if(m_project != nullptr)
		m_project->commandWorker(m_actor);
	else
	{
		Actors& actors = area.getActors();
		DigProject* project = nullptr;
		std::function<bool(BlockIndex)> predicate = [&area, this, project, &actors](BlockIndex block) mutable
		{ 
			if(!getJoinableProjectAt(area, block))
				return false;
			project = &area.m_hasDigDesignations.at(*actors.getFaction(m_actor), block);
			if(project->canAddWorker(m_actor))
				return true;
			return false;
		};
		[[maybe_unused]] BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(m_actor, predicate);
		if(project != nullptr)
		{
			assert(adjacent != BLOCK_INDEX_MAX);
			joinProject(*project);
			return;
		}
		std::unique_ptr<PathRequest> request = std::make_unique<DigPathRequest>(area, *this);
		actors.move_pathRequestRecord(m_actor, std::move(request));
	}
}
void DigObjective::cancel(Area& area)
{
	if(m_project != nullptr)
		m_project->removeWorker(m_actor);
	area.getActors().move_pathRequestMaybeCancel(m_actor);
}
void DigObjective::delay(Area& area) 
{ 
	cancel(area); 
	m_project = nullptr;
	area.getActors().project_unset(m_actor);
}
void DigObjective::reset(Area& area) 
{ 
	Actors& actors = area.getActors();
	if(m_project)
	{
		assert(!m_project->getWorkers().contains(m_actor));
		m_project = nullptr; 
		actors.project_unset(m_actor);
	}
	else
		assert(!actors.project_exists(m_actor));
	area.getActors().move_pathRequestMaybeCancel(m_actor);
	actors.canReserve_clearAll(m_actor);
}
void DigObjective::onProjectCannotReserve(Area&)
{
	assert(m_project);
	m_cannotJoinWhileReservationsAreNotComplete.insert(m_project);
}
void DigObjective::joinProject(DigProject& project)
{
	assert(!m_project);
	m_project = &project;
	project.addWorkerCandidate(m_actor, *this);
}
DigProject* DigObjective::getJoinableProjectAt(Area& area, BlockIndex block)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Faction& faction = *actors.getFaction(m_actor);
	if(!blocks.designation_has(block, faction, BlockDesignation::Dig))
		return nullptr;
	DigProject& output = area.m_hasDigDesignations.at(faction, block);
	if(!output.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&output))
		return nullptr;
	if(!output.canAddWorker(m_actor))
		return nullptr;
	return &output;
}
bool DigObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	//TODO: check for any picks?
	return area.m_hasDigDesignations.areThereAnyForFaction(*area.getActors().getFaction(actor));
}
std::unique_ptr<Objective> DigObjectiveType::makeFor(Area&, ActorIndex actor) const
{
	std::unique_ptr<Objective> objective = std::make_unique<DigObjective>(actor);
	return objective;
}
