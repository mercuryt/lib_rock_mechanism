#include "dig.h"
#include "../dig.h"
#include "../area.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../terrainFacade.h"
#include "reference.h"
#include "types.h"
DigPathRequest::DigPathRequest(Area& area, DigObjective& digObjective, const ActorIndex& actor) : m_digObjective(digObjective)
{
	std::function<bool(const BlockIndex&)> predicate = [&area, this, actor](BlockIndex block)
	{
		return m_digObjective.getJoinableProjectAt(area, block, actor) != nullptr;
	};
	DistanceInBlocks maxRange = Config::maxRangeToSearchForDigDesignations;
	bool unreserved = true;
	//TODO: We don't need the whole path here, just the destination.
	createGoAdjacentToCondition(area, actor, predicate, m_digObjective.m_detour, unreserved, maxRange, BlockIndex::null());
}
DigPathRequest::DigPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	m_digObjective(static_cast<DigObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())))
{
	nlohmann::from_json(data, *this);
}
void DigPathRequest::callback(Area& area, const FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(actor, m_digObjective);
	else
	{
		// No need to reserve here, we are just checking for access.
		BlockIndex target = result.blockThatPassedPredicate;
		DigProject& project = area.m_hasDigDesignations.getForFactionAndBlock(actors.getFactionId(actor), target);
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
Json DigPathRequest::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output["objective"] = &m_digObjective;
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
		std::unique_ptr<PathRequest> request = std::make_unique<DigPathRequest>(area, *this, actor);
		actors.move_pathRequestRecord(actor, std::move(request));
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
