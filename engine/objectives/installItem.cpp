#include "installItem.h"
#include "area.h"
#include "blocks/blocks.h"
#include "actors/actors.h"
#include "plants.h"
#include "items/items.h"
#include "types.h"
// PathRequest.
InstallItemPathRequest::InstallItemPathRequest(Area& area, InstallItemObjective& iio, const ActorIndex& actor) : m_installItemObjective(iio)
{
	Actors& actors = area.getActors();
	std::function<bool(BlockIndex)> predicate = [&, actor](BlockIndex block)
	{
		FactionId faction = actors.getFactionId(actor);
		return area.m_hasInstallItemDesignations.getForFaction(faction).contains(block);
	};
	bool unreserved = false;
	createGoAdjacentToCondition(area, actor, predicate, m_installItemObjective.m_detour, unreserved, DistanceInBlocks::max(), BlockIndex::null());
}
InstallItemPathRequest::InstallItemPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	PathRequest(data),
	m_installItemObjective(static_cast<InstallItemObjective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
Json InstallItemPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_installItemObjective);
	output["type"] = "install item";
	return output;
}
void InstallItemPathRequest::callback(Area& area, const FindPathResult& result)
{
	ActorIndex actor = getActor();
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(actor, m_installItemObjective);
	else
	{
		BlockIndex block = result.blockThatPassedPredicate;
		auto& hasInstallItemDesignations = area.m_hasInstallItemDesignations.getForFaction(actors.getFactionId(actor));
		if(!hasInstallItemDesignations.contains(block) || !hasInstallItemDesignations.getForBlock(block).canAddWorker(actor))
			// Canceled or reserved, try again.
			m_installItemObjective.execute(area, actor);
		else
			hasInstallItemDesignations.getForBlock(block).addWorkerCandidate(actor, m_installItemObjective);
	}

}
// Objective.
InstallItemObjective::InstallItemObjective() : Objective(Config::installItemPriority) { }
InstallItemObjective::InstallItemObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo)
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
void InstallItemObjective::execute(Area& area, const ActorIndex& actor)
{
	if(m_project)
		m_project->commandWorker(actor);
	else
		area.getActors().move_pathRequestRecord(actor, std::make_unique<InstallItemPathRequest>(area, *this, actor));
}
void InstallItemObjective::cancel(Area& area, const ActorIndex& actor) { area.getActors().move_pathRequestMaybeCancel(actor); m_project->removeWorker(actor); }
bool InstallItemObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	return !area.m_hasInstallItemDesignations.getForFaction(area.getActors().getFactionId(actor)).empty();
}
std::unique_ptr<Objective> InstallItemObjectiveType::makeFor(Area&, const ActorIndex&) const
{
	std::unique_ptr<Objective> objective = std::make_unique<InstallItemObjective>();
	return objective;
}
void InstallItemObjective::reset(Area& area, const ActorIndex& actor) { area.getActors().canReserve_clearAll(actor); m_project = nullptr; }
