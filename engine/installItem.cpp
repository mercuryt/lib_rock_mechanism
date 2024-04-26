#include "installItem.h"
#include "block.h"
#include "actor.h"
#include "area.h"
#include "deserializationMemo.h"
#include "threadedTask.h"

// Project.
void InstallItemProject::onComplete()
{
	m_item.m_facing = m_facing;
	m_location.m_hasItems.add(m_item);
	m_item.m_installed = true;
}
// ThreadedTask.
InstallItemThreadedTask::InstallItemThreadedTask(InstallItemObjective& iio) : ThreadedTask(iio.m_actor.getThreadedTaskEngine()), m_installItemObjective(iio), m_findsPath(iio.m_actor, iio.m_detour) { }
void InstallItemThreadedTask::readStep()
{
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
		return block.m_area->m_hasInstallItemDesignations.at(*m_installItemObjective.m_actor.getFaction()).contains(block);
	};
	m_findsPath.pathToAdjacentToPredicate(predicate);
}
void InstallItemThreadedTask::writeStep()
{
	Actor& actor = m_installItemObjective.m_actor;
	if(!m_findsPath.found())
		actor.m_hasObjectives.cannotFulfillObjective(m_installItemObjective);
	else
	{
		Block& block = *m_findsPath.getBlockWhichPassedPredicate();
		auto& hasInstallItemDesignations = actor.m_location->m_area->m_hasInstallItemDesignations.at(*actor.getFaction());
		if(!hasInstallItemDesignations.contains(block) || !hasInstallItemDesignations.at(block).canAddWorker(actor))
			// Canceled or reserved, try again.
			m_installItemObjective.m_threadedTask.create(m_installItemObjective);
		else
			hasInstallItemDesignations.at(block).addWorkerCandidate(m_installItemObjective.m_actor, m_installItemObjective);
	}

}
void InstallItemThreadedTask::clearReferences() { m_installItemObjective.m_threadedTask.clearPointer(); }
// Objective.
InstallItemObjective::InstallItemObjective(Actor& a) : Objective(a, Config::installItemPriority), m_threadedTask(a.getThreadedTaskEngine()) { }
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
void InstallItemObjective::execute()
{
	if(m_project)
		m_project->commandWorker(m_actor);
	else
		m_threadedTask.create(*this); 
}
bool InstallItemObjectiveType::canBeAssigned(Actor& actor) const
{
	return !actor.m_location->m_area->m_hasInstallItemDesignations.at(*actor.getFaction()).empty();
}
std::unique_ptr<Objective> InstallItemObjectiveType::makeFor(Actor& actor) const
{
	std::unique_ptr<Objective> objective = std::make_unique<InstallItemObjective>(actor);
	return objective;
}
void InstallItemObjective::reset() { m_actor.m_canReserve.deleteAllWithoutCallback(); m_project = nullptr; }
// HasDesignations.
void HasInstallItemDesignationsForFaction::add(Block& block, Item& item, Facing facing, const Faction& faction)
{
	assert(!m_designations.contains(&block));
	m_designations.try_emplace(&block, item, block, facing, faction);
}
void HasInstallItemDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second.m_designations)
			pair2.second.clearReservations();
}
void HasInstallItemDesignationsForFaction::remove(Item& item)
{
	assert(m_designations.contains(item.m_location));
	m_designations.erase(item.m_location);
}
