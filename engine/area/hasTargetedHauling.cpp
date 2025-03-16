#include "hasTargetedHauling.h"
#include "../actors/actors.h"
#include "../objectives/targetedHaul.h"
#include "area.h"
TargetedHaulProject& AreaHasTargetedHauling::begin(const SmallSet<ActorIndex>& workers, const ActorOrItemIndex& target, const BlockIndex& destination)
{
	Actors& actors = m_area.getActors();
	ActorOrItemReference ref = target.toReference(m_area);
	TargetedHaulProject& project = m_projects.emplace_back(actors.getFaction(workers.front()), m_area, destination, ref);
	for(ActorIndex actor : workers)
	{
		std::unique_ptr<Objective> objective = std::make_unique<TargetedHaulObjective>(project);
		actors.objective_addTaskToStart(actor, std::move(objective));
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
void AreaHasTargetedHauling::clearReservations()
{
	for(Project& project : m_projects)
		project.clearReservations();
}
Json AreaHasTargetedHauling::toJson() const
{
	Json data{{"projects", Json::array()}};
	for(const Project& project : m_projects)
		data["projects"].push_back(project);
	return data;
}
void AreaHasTargetedHauling::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	for(const Json& project : data["projects"])
		m_projects.emplace_back(project, deserializationMemo, area);
}