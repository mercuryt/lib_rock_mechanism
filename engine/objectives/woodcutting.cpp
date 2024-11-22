#include "woodcutting.h"
#include "../area.h"
#include "../woodcutting.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "reference.h"
#include "types.h"
WoodCuttingPathRequest::WoodCuttingPathRequest(Area& area, WoodCuttingObjective& woodCuttingObjective, const ActorIndex& actor) :
	m_woodCuttingObjective(woodCuttingObjective)
{
	std::function<bool(const BlockIndex&)> predicate = [this, &area, actor](const BlockIndex& block)
	{
		return m_woodCuttingObjective.getJoinableProjectAt(area, block, actor) != nullptr;
	};
	DistanceInBlocks maxRange = Config::maxRangeToSearchForWoodCuttingDesignations;
	bool unreserved = true;
	createGoAdjacentToCondition(area, actor, predicate, m_woodCuttingObjective.m_detour, unreserved, maxRange, BlockIndex::null());
}
WoodCuttingPathRequest::WoodCuttingPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	PathRequest(data),
	m_woodCuttingObjective(static_cast<WoodCuttingObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))) { }
void WoodCuttingPathRequest::callback(Area& area, const FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(actor, m_woodCuttingObjective);
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
		WoodCuttingProject& project = area.m_hasWoodCuttingDesignations.getForFactionAndBlock(actors.getFactionId(actor), target);
		if(project.canAddWorker(actor))
			m_woodCuttingObjective.joinProject(project, actor);
		else
		{
			// Project can no longer accept this worker, try again.
			actors.objective_canNotCompleteSubobjective(actor);
			return;
		}
	}
}
Json WoodCuttingPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = &m_woodCuttingObjective;
	return output;
}
WoodCuttingObjective::WoodCuttingObjective() : Objective(Config::woodCuttingObjectivePriority) { }
WoodCuttingObjective::WoodCuttingObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_project(data.contains("project") ? static_cast<WoodCuttingProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) : nullptr) { }
Json WoodCuttingObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = m_project;
	return data;
}
void WoodCuttingObjective::execute(Area& area, const ActorIndex& actor)
{
	if(m_project != nullptr)
		m_project->commandWorker(actor);
	else
	{
		Actors& actors = area.getActors();
		WoodCuttingProject* project = nullptr;
		std::function<bool(const BlockIndex&)> predicate = [&](const BlockIndex& block) 
		{ 
			if(!getJoinableProjectAt(area, block, actor))
				return false;
			project = &area.m_hasWoodCuttingDesignations.getForFactionAndBlock(actors.getFactionId(actor), block);
			if(project->canAddWorker(actor))
				return true;
			return false;
		};
		BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(actor, predicate);
		if(project != nullptr)
		{
			assert(adjacent.exists());
			joinProject(*project, actor);
			return;
		}
		actors.move_pathRequestRecord(actor, std::make_unique<WoodCuttingPathRequest>(area, *this, actor));
	}
}
void WoodCuttingObjective::cancel(Area& area, const ActorIndex& actor)
{
	if(m_project != nullptr)
		m_project->removeWorker(actor);
	area.getActors().move_pathRequestMaybeCancel(actor);
}
void WoodCuttingObjective::delay(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
	m_project = nullptr;
	area.getActors().project_unset(actor);
}
void WoodCuttingObjective::reset(Area& area, const ActorIndex& actor) 
{ 
	Actors& actors = area.getActors();
	if(m_project)
	{
		ActorReference ref = area.getActors().getReference(actor);
		assert(!m_project->getWorkers().contains(ref));
		m_project = nullptr; 
		area.getActors().project_unset(actor);
	}
	else 
		assert(!actors.project_exists(actor));
	actors.move_pathRequestMaybeCancel(actor);
	actors.canReserve_clearAll(actor);
}
void WoodCuttingObjective::onProjectCannotReserve(Area&, const ActorIndex&)
{
	assert(m_project);
	m_cannotJoinWhileReservationsAreNotComplete.insert(m_project);
}
void WoodCuttingObjective::joinProject(WoodCuttingProject& project, const ActorIndex& actor)
{
	assert(m_project == nullptr);
	m_project = &project;
	project.addWorkerCandidate(actor, *this);
}
WoodCuttingProject* WoodCuttingObjective::getJoinableProjectAt(Area& area, const BlockIndex& block, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	FactionId faction = actors.getFactionId(actor);
	if(!area.getBlocks().designation_has(block, faction, BlockDesignation::WoodCutting))
		return nullptr;
	WoodCuttingProject& output = area.m_hasWoodCuttingDesignations.getForFactionAndBlock(faction, block);
	if(!output.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&output))
		return nullptr;
	if(!output.canAddWorker(actor))
		return nullptr;
	return &output;
}
bool WoodCuttingObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	//TODO: check for any axes?
	return area.m_hasWoodCuttingDesignations.areThereAnyForFaction(area.getActors().getFactionId(actor));
}
std::unique_ptr<Objective> WoodCuttingObjectiveType::makeFor(Area&, const ActorIndex&) const
{
	std::unique_ptr<Objective> objective = std::make_unique<WoodCuttingObjective>();
	return objective;
}
