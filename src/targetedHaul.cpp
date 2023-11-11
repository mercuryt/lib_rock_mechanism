#include "targetedHaul.h"
#include "block.h"
#include "area.h"
void TargetedHaulProject::onComplete()
{
	m_item.setLocation(m_location);
	auto workers = std::move(m_workers);
	m_location.m_area->m_targetedHauling.complete(*this);
	for(auto& [actor, projectWorker] : workers)
		actor->m_hasObjectives.objectiveComplete(projectWorker.objective);
}
TargetedHaulProject& AreaHasTargetedHauling::begin(std::vector<Actor*> actors, Item& item, Block& destination)
{
	TargetedHaulProject& project = m_projects.emplace_back(actors.front()->getFaction(), destination, item);
	for(Actor* actor : actors)
	{
		std::unique_ptr<Objective> objective = std::make_unique<TargetedHaulObjective>(*actor, project);
		actor->m_hasObjectives.addTaskToStart(std::move(objective));
	}
	return m_projects.back();
}
void AreaHasTargetedHauling::cancel(TargetedHaulProject& project)
{
	project.cancel();
	m_projects.remove(project);
}
void AreaHasTargetedHauling::complete(TargetedHaulProject& project)
{
	m_projects.remove(project);
}
