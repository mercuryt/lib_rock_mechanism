#include "woodcutting.h"
#include "../area.h"
#include "../woodcutting.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
WoodCuttingPathRequest::WoodCuttingPathRequest(Area& area, WoodCuttingObjective& woodCuttingObjective) :
	m_woodCuttingObjective(woodCuttingObjective)
{
	std::function<bool(BlockIndex)> predicate = [this, &area](BlockIndex block)
	{
		return m_woodCuttingObjective.getJoinableProjectAt(area, block) != nullptr;
	};
	DistanceInBlocks maxRange = Config::maxRangeToSearchForWoodCuttingDesignations;
	bool unreserved = true;
	createGoAdjacentToCondition(area, m_woodCuttingObjective.m_actor, predicate, m_woodCuttingObjective.m_detour, unreserved, maxRange, BLOCK_INDEX_MAX);
}
void WoodCuttingPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(m_woodCuttingObjective.m_actor, m_woodCuttingObjective);
	else
	{
		if(result.useCurrentPosition)
		{
			if(!actors.move_tryToReserveOccupied(m_woodCuttingObjective.m_actor))
			{
				actors.objective_canNotCompleteSubobjective(m_woodCuttingObjective.m_actor);
				return;
			}
		}
		else if(!actors.move_tryToReserveProposedDestination(m_woodCuttingObjective.m_actor, result.path))
		{
			actors.objective_canNotCompleteSubobjective(m_woodCuttingObjective.m_actor);
			return;
		}
		BlockIndex target = result.blockThatPassedPredicate;
		WoodCuttingProject& project = area.m_hasWoodCuttingDesignations.at(*actors.getFaction(m_woodCuttingObjective.m_actor), target);
		if(project.canAddWorker(m_woodCuttingObjective.m_actor))
			m_woodCuttingObjective.joinProject(project);
		else
		{
			// Project can no longer accept this worker, try again.
			actors.objective_canNotCompleteSubobjective(m_woodCuttingObjective.m_actor);
			return;
		}
	}
}
WoodCuttingObjective::WoodCuttingObjective(ActorIndex a) :
	Objective(a, Config::woodCuttingObjectivePriority) { }
/*
WoodCuttingObjective::WoodCuttingObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_woodCuttingPathRequest(m_actor.m_area->m_simulation.m_threadedTaskEngine), 
	m_project(data.contains("project") ? static_cast<WoodCuttingProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) : nullptr)
{
	if(data.contains("threadedTask"))
		m_woodCuttingPathRequest.create(*this);
}
Json WoodCuttingObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = m_project;
	if(m_woodCuttingPathRequest.exists())
		data["threadedTask"] = true;
	return data;
}
*/
void WoodCuttingObjective::execute(Area& area)
{
	if(m_project != nullptr)
		m_project->commandWorker(m_actor);
	else
	{
		Actors& actors = area.getActors();
		WoodCuttingProject* project = nullptr;
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) 
		{ 
			if(!getJoinableProjectAt(area, block))
				return false;
			project = &area.m_hasWoodCuttingDesignations.at(*actors.getFaction(m_actor), block);
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
		actors.move_pathRequestRecord(m_actor, std::make_unique<WoodCuttingPathRequest>(area, *this));
	}
}
void WoodCuttingObjective::cancel(Area& area)
{
	if(m_project != nullptr)
		m_project->removeWorker(m_actor);
	area.getActors().move_pathRequestMaybeCancel(m_actor);
}
void WoodCuttingObjective::delay(Area& area)
{
	cancel(area);
	m_project = nullptr;
	area.getActors().project_unset(m_actor);
}
void WoodCuttingObjective::reset(Area& area) 
{ 
	Actors& actors = area.getActors();
	if(m_project)
	{
		assert(!m_project->getWorkers().contains(m_actor));
		m_project = nullptr; 
		area.getActors().project_unset(m_actor);
	}
	else 
		assert(!actors.project_exists(m_actor));
	actors.move_pathRequestMaybeCancel(m_actor);
	actors.canReserve_clearAll(m_actor);
}
void WoodCuttingObjective::onProjectCannotReserve(Area&)
{
	assert(m_project);
	m_cannotJoinWhileReservationsAreNotComplete.insert(m_project);
}
void WoodCuttingObjective::joinProject(WoodCuttingProject& project)
{
	assert(m_project == nullptr);
	m_project = &project;
	project.addWorkerCandidate(m_actor, *this);
}
WoodCuttingProject* WoodCuttingObjective::getJoinableProjectAt(Area& area, BlockIndex block)
{
	Actors& actors = area.getActors();
	Faction& faction = *actors.getFaction(m_actor);
	if(!area.getBlocks().designation_has(block, faction, BlockDesignation::WoodCutting))
		return nullptr;
	WoodCuttingProject& output = area.m_hasWoodCuttingDesignations.at(faction, block);
	if(!output.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&output))
		return nullptr;
	if(!output.canAddWorker(m_actor))
		return nullptr;
	return &output;
}
bool WoodCuttingObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	//TODO: check for any axes?
	return area.m_hasWoodCuttingDesignations.areThereAnyForFaction(*area.getActors().getFaction(actor));
}
std::unique_ptr<Objective> WoodCuttingObjectiveType::makeFor(Area&, ActorIndex actor) const
{
	std::unique_ptr<Objective> objective = std::make_unique<WoodCuttingObjective>(actor);
	return objective;
}
