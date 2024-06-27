#include "installItem.h"
#include "area.h"
#include "deserializationMemo.h"
#include "threadedTask.h"

// Project.
InstallItemProject::InstallItemProject(Area& area, ItemIndex i, BlockIndex l, Facing facing, Faction& faction) : Project(&faction, area, l, 1), m_item(i), m_facing(facing) { }
void InstallItemProject::onComplete() { m_area.m_items.setLocationAndFacing(m_item, m_location, m_facing); }
// ThreadedTask.
InstallItemThreadedTask::InstallItemThreadedTask(Area& area, InstallItemObjective& iio) : 
	m_installItemObjective(iio), m_findsPath(area, area.m_actors.getShape(iio.m_actor), area.m_actors.getMoveType(iio.m_actor), area.m_actors.getLocation(iio.m_actor), area.m_actors.getFacing(iio.m_actor), iio.m_detour) { }
void InstallItemThreadedTask::readStep(Simulation&, Area* area)
{
	Actors& actors = area->m_actors;
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block)
	{
		Faction& faction = *actors.getFaction(m_installItemObjective.m_actor);
		return area->m_hasInstallItemDesignations.at(faction).contains(block);
	};
	m_findsPath.pathToAdjacentToPredicate(predicate);
}
void InstallItemThreadedTask::writeStep(Simulation&, Area* area)
{
	ActorIndex actor = m_installItemObjective.m_actor;
	Actors& actors = area->m_actors;
	if(!m_findsPath.found())
		actors.objective_canNotCompleteObjective(actor, m_installItemObjective);
	else
	{
		BlockIndex block = m_findsPath.getBlockWhichPassedPredicate();
		auto& hasInstallItemDesignations = area->m_hasInstallItemDesignations.at(*actors.getFaction(actor));
		if(!hasInstallItemDesignations.contains(block) || !hasInstallItemDesignations.at(block).canAddWorker(actor))
			// Canceled or reserved, try again.
			m_installItemObjective.m_threadedTask.create(m_installItemObjective);
		else
			hasInstallItemDesignations.at(block).addWorkerCandidate(m_installItemObjective.m_actor, m_installItemObjective);
	}

}
void InstallItemThreadedTask::clearReferences(Simulation&, Area*) { m_installItemObjective.m_threadedTask.clearPointer(); }
// Objective.
InstallItemObjective::InstallItemObjective(Area& area, ActorIndex a) : Objective(a, Config::installItemPriority), m_threadedTask(area.m_threadedTaskEngine) { }
/*
InstallItemObjective::InstallItemObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo), m_threadedTask(m_actor.getThreadedTaskEngine())
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
void InstallItemObjective::execute(Area&)
{
	if(m_project)
		m_project->commandWorker(m_actor);
	else
		m_threadedTask.create(*this); 
}
void InstallItemObjective::cancel(Area&) { m_threadedTask.maybeCancel(); m_project->removeWorker(m_actor); }
bool InstallItemObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	return !area.m_hasInstallItemDesignations.at(*area.m_actors.getFaction(actor)).empty();
}
std::unique_ptr<Objective> InstallItemObjectiveType::makeFor(Area& area, ActorIndex actor) const
{
	std::unique_ptr<Objective> objective = std::make_unique<InstallItemObjective>(area, actor);
	return objective;
}
void InstallItemObjective::reset(Area& area) { area.m_actors.canReserve_clearAll(m_actor); m_project = nullptr; }
// HasDesignations.
void HasInstallItemDesignationsForFaction::add(BlockIndex block, ItemIndex item, Facing facing, Faction& faction)
{
	assert(!m_designations.contains(block));
	m_designations.try_emplace(block, item, block, facing, faction);
}
void AreaHasInstallItemDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second.m_designations)
			pair2.second.clearReservations();
}
void HasInstallItemDesignationsForFaction::remove(Area& area, ItemIndex item)
{
	assert(m_designations.contains(area.m_items.getLocation(item)));
	m_designations.erase(area.m_items.getLocation(item));
}
