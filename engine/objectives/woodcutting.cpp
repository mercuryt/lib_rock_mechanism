#include "woodcutting.h"
#include "../area.h"
#include "../woodcutting.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../reference.h"
#include "../types.h"
#include "../path/terrainFacade.hpp"
#include "../hasShapes.hpp"
WoodCuttingPathRequest::WoodCuttingPathRequest(Area& area, WoodCuttingObjective& woodCuttingObjective, const ActorIndex& actorIndex) :
	m_woodCuttingObjective(woodCuttingObjective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForWoodCuttingDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_woodCuttingObjective.m_detour;
	adjacent = true;
	reserveDestination = true;
}
WoodCuttingPathRequest::WoodCuttingPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_woodCuttingObjective(static_cast<WoodCuttingObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())))
{ }
FindPathResult WoodCuttingPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	ActorIndex actorIndex = actor.getIndex(area.getActors().m_referenceData);
	auto predicate = [this, &area, actorIndex](const BlockIndex& block, const Facing4&) -> std::pair<bool, BlockIndex>
	{
		return {m_woodCuttingObjective.joinableProjectExistsAt(area, block, actorIndex), block};
	};
	constexpr bool useAnyOccupiedBlock = true;
	constexpr bool unreserved = false;
	return terrainFacade.findPathToBlockDesignationAndCondition<useAnyOccupiedBlock, decltype(predicate)>(predicate, memo, BlockDesignation::WoodCutting, faction, start, facing, shape, detour, adjacent, unreserved, maxRange);
}
void WoodCuttingPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(actorIndex, m_woodCuttingObjective);
	else
	{
		if(result.useCurrentPosition)
		{
			if(!actors.move_tryToReserveOccupied(actorIndex))
			{
				actors.objective_canNotCompleteSubobjective(actorIndex);
				return;
			}
		}
		else if(!actors.move_tryToReserveProposedDestination(actorIndex, result.path))
		{
			actors.objective_canNotCompleteSubobjective(actorIndex);
			return;
		}
		BlockIndex target = result.blockThatPassedPredicate;
		WoodCuttingProject& project = area.m_hasWoodCuttingDesignations.getForFactionAndBlock(actors.getFaction(actorIndex), target);
		if(project.canAddWorker(actorIndex))
			m_woodCuttingObjective.joinProject(project, actorIndex);
		else
		{
			// Project can no longer accept this worker, try again.
			actors.objective_canNotCompleteSubobjective(actorIndex);
			return;
		}
	}
}
Json WoodCuttingPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
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
			project = &area.m_hasWoodCuttingDesignations.getForFactionAndBlock(actors.getFaction(actor), block);
			if(project->canAddWorker(actor))
				return true;
			return false;
		};
		[[maybe_unused]] BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(actor, predicate);
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
	FactionId faction = actors.getFaction(actor);
	if(!area.getBlocks().designation_has(block, faction, BlockDesignation::WoodCutting))
		return nullptr;
	WoodCuttingProject& output = area.m_hasWoodCuttingDesignations.getForFactionAndBlock(faction, block);
	if(!output.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&output))
		return nullptr;
	if(!output.canAddWorker(actor))
		return nullptr;
	return &output;
}
bool WoodCuttingObjective::joinableProjectExistsAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	FactionId faction = actors.getFaction(actor);
	assert(area.getBlocks().designation_has(block, faction, BlockDesignation::WoodCutting));
	WoodCuttingProject& project = area.m_hasWoodCuttingDesignations.getForFactionAndBlock(faction, block);
	return !project.reservationsComplete() && !m_cannotJoinWhileReservationsAreNotComplete.contains(&project) && project.canAddWorker(actor);
}
bool WoodCuttingObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	//TODO: check for any axes?
	return area.m_hasWoodCuttingDesignations.areThereAnyForFaction(area.getActors().getFaction(actor));
}
std::unique_ptr<Objective> WoodCuttingObjectiveType::makeFor(Area&, const ActorIndex&) const
{
	std::unique_ptr<Objective> objective = std::make_unique<WoodCuttingObjective>();
	return objective;
}
