#include "installItem.h"
#include "actors/actors.h"
#include "area.h"
#include "deserializationMemo.h"
#include "items/items.h"
#include "terrainFacade.h"

// Project.
InstallItemProject::InstallItemProject(Area& area, ItemIndex i, BlockIndex l, Facing facing, Faction& faction) :
	Project(faction, area, l, 1), m_item(i), m_facing(facing) { }
void InstallItemProject::onComplete() { m_area.getItems().setLocationAndFacing(m_item, m_location, m_facing); }
// PathRequest.
InstallItemPathRequest::InstallItemPathRequest(Area& area, InstallItemObjective& iio) : m_installItemObjective(iio)
{
	Actors& actors = area.getActors();
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block)
	{
		Faction& faction = *actors.getFaction(m_installItemObjective.m_actor);
		return area.m_hasInstallItemDesignations.at(faction).contains(block);
	};
	bool unreserved = false;
	createGoAdjacentToCondition(area, m_installItemObjective.m_actor, predicate, m_installItemObjective.m_detour, unreserved, BLOCK_DISTANCE_MAX, BLOCK_INDEX_MAX);
}
void InstallItemPathRequest::callback(Area& area, FindPathResult& result)
{
	ActorIndex actor = m_installItemObjective.m_actor;
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
		actors.objective_canNotCompleteObjective(actor, m_installItemObjective);
	else
	{
		BlockIndex block = result.blockThatPassedPredicate;
		auto& hasInstallItemDesignations = area.m_hasInstallItemDesignations.at(*actors.getFaction(actor));
		if(!hasInstallItemDesignations.contains(block) || !hasInstallItemDesignations.at(block).canAddWorker(actor))
			// Canceled or reserved, try again.
			m_installItemObjective.execute(area);
		else
			hasInstallItemDesignations.at(block).addWorkerCandidate(m_installItemObjective.m_actor, m_installItemObjective);
	}

}
// Objective.
InstallItemObjective::InstallItemObjective(ActorIndex a) : Objective(a, Config::installItemPriority) { }
/*
InstallItemObjective::InstallItemObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo), m_threadedTask(m_actor.getPathRequestEngine())
{ 
	if(data.contains("project"))
		m_project = static_cast<InstallItemProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()));
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
}
Json InstallItemObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project)
		data["project"] = m_project;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	return data;
}
*/
void InstallItemObjective::execute(Area& area)
{
	if(m_project)
		m_project->commandWorker(m_actor);
	else
		area.getActors().move_pathRequestRecord(m_actor, std::make_unique<InstallItemPathRequest>(area, *this));
}
void InstallItemObjective::cancel(Area& area) { area.getActors().move_pathRequestMaybeCancel(m_actor); m_project->removeWorker(m_actor); }
bool InstallItemObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	return !area.m_hasInstallItemDesignations.at(*area.getActors().getFaction(actor)).empty();
}
std::unique_ptr<Objective> InstallItemObjectiveType::makeFor(Area&, ActorIndex actor) const
{
	std::unique_ptr<Objective> objective = std::make_unique<InstallItemObjective>(actor);
	return objective;
}
void InstallItemObjective::reset(Area& area) { area.getActors().canReserve_clearAll(m_actor); m_project = nullptr; }
// HasDesignations.
void HasInstallItemDesignationsForFaction::add(Area& area, BlockIndex block, ItemIndex item, Facing facing, Faction& faction)
{
	assert(!m_designations.contains(block));
	m_designations.try_emplace(block, area, item, block, facing, faction);
}
void AreaHasInstallItemDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second.m_designations)
			pair2.second.clearReservations();
}
void HasInstallItemDesignationsForFaction::remove(Area& area, ItemIndex item)
{
	assert(m_designations.contains(area.getItems().getLocation(item)));
	m_designations.erase(area.getItems().getLocation(item));
}
