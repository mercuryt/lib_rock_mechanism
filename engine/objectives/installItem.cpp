#include "installItem.h"
#include "area.h"
#include "blocks/blocks.h"
#include "actors/actors.h"
#include "plants.h"
#include "items/items.h"
#include "types.h"
// PathRequest.
InstallItemPathRequest::InstallItemPathRequest(Area& area, InstallItemObjective& iio) : m_installItemObjective(iio)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block)
	{
		FactionId faction = actors.getFactionId(actor);
		return area.m_hasInstallItemDesignations.at(faction).contains(block);
	};
	bool unreserved = false;
	createGoAdjacentToCondition(area, actor, predicate, m_installItemObjective.m_detour, unreserved, BLOCK_DISTANCE_MAX, BLOCK_INDEX_MAX);
}
void InstallItemPathRequest::callback(Area& area, FindPathResult& result)
{
	ActorIndex actor = getActor();
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(actor, m_installItemObjective);
	else
	{
		BlockIndex block = result.blockThatPassedPredicate;
		auto& hasInstallItemDesignations = area.m_hasInstallItemDesignations.at(actors.getFactionId(actor));
		if(!hasInstallItemDesignations.contains(block) || !hasInstallItemDesignations.at(block).canAddWorker(actor))
			// Canceled or reserved, try again.
			m_installItemObjective.execute(area, actor);
		else
			hasInstallItemDesignations.at(block).addWorkerCandidate(actor, m_installItemObjective);
	}

}
// Objective.
InstallItemObjective::InstallItemObjective() : Objective(Config::installItemPriority) { }
InstallItemObjective::InstallItemObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data)
{ 
	if(data.contains("project"))
		m_project = static_cast<InstallItemProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()));
}
Json InstallItemObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project)
		data["project"] = m_project;
	return data;
}
void InstallItemObjective::execute(Area& area, ActorIndex actor)
{
	if(m_project)
		m_project->commandWorker(actor);
	else
		area.getActors().move_pathRequestRecord(actor, std::make_unique<InstallItemPathRequest>(area, *this));
}
void InstallItemObjective::cancel(Area& area, ActorIndex actor) { area.getActors().move_pathRequestMaybeCancel(actor); m_project->removeWorker(actor); }
bool InstallItemObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	return !area.m_hasInstallItemDesignations.at(area.getActors().getFactionId(actor)).empty();
}
std::unique_ptr<Objective> InstallItemObjectiveType::makeFor(Area&, ActorIndex) const
{
	std::unique_ptr<Objective> objective = std::make_unique<InstallItemObjective>();
	return objective;
}
void InstallItemObjective::reset(Area& area, ActorIndex actor) { area.getActors().canReserve_clearAll(actor); m_project = nullptr; }
