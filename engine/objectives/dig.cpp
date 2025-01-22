#include "dig.h"
#include "../dig.h"
#include "../area.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../terrainFacade.hpp"
#include "../reference.h"
#include "../types.h"
#include "../pathRequest.h"
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
	auto predicate = [&area, this, actorIndex](const BlockIndex& block, const Facing4&) -> std::pair<bool, BlockIndex>
	{
		return {m_digObjective.joinableProjectExistsAt(area, block, actorIndex), block};
	};
	constexpr bool useAnyBlock = true;
	constexpr bool unreserved = false;
	return terrainFacade.findPathToBlockDesignationAndCondition<useAnyBlock, decltype(predicate)>(predicate, memo, BlockDesignation::Dig, faction, start, facing, shape, detour, adjacent, unreserved, maxRange);

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
		BlockIndex target = result.blockThatPassedPredicate;
		DigProject& project = area.m_hasDigDesignations.getForFactionAndBlock(actors.getFactionId(actorIndex), target);
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
		DigProject* project = nullptr;
		std::function<bool(const BlockIndex&)> predicate = [&](const BlockIndex& block) mutable
		{
			if(!getJoinableProjectAt(area, block, actor))
				return false;
			project = &area.m_hasDigDesignations.getForFactionAndBlock(actors.getFactionId(actor), block);
			if(project->canAddWorker(actor))
				return true;
			return false;
		};
		BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(actor, predicate);
		if(adjacent.exists())
		{
			assert(project != nullptr);
			joinProject(*project, actor);
			return;
		}
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
DigProject* DigObjective::getJoinableProjectAt(Area& area, BlockIndex block, const ActorIndex& actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	FactionId faction = actors.getFactionId(actor);
	if(!blocks.designation_has(block, faction, BlockDesignation::Dig))
		return nullptr;
	DigProject& output = area.m_hasDigDesignations.getForFactionAndBlock(faction, block);
	if(!output.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&output))
		return nullptr;
	if(!output.canAddWorker(actor))
		return nullptr;
	return &output;
}
bool DigObjective::joinableProjectExistsAt(Area& area, BlockIndex block, const ActorIndex& actor) const
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	FactionId faction = actors.getFactionId(actor);
	assert(blocks.designation_has(block, faction, BlockDesignation::Dig));
	DigProject& output = area.m_hasDigDesignations.getForFactionAndBlock(faction, block);
	if(!output.reservationsComplete() && m_cannotJoinWhileReservationsAreNotComplete.contains(&output))
		return false;
	if(!output.canAddWorker(actor))
		return false;
	return true;
}
bool DigObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	//TODO: check for any picks?
	return area.m_hasDigDesignations.areThereAnyForFaction(area.getActors().getFactionId(actor));
}
std::unique_ptr<Objective> DigObjectiveType::makeFor(Area&, const ActorIndex&) const
{
	std::unique_ptr<Objective> objective = std::make_unique<DigObjective>();
	return objective;
}
