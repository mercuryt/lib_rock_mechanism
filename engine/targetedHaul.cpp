#include "targetedHaul.h"
#include "area.h"
#include "deserializationMemo.h"
#include "objective.h"
#include "project.h"
Json TargetedHaulObjective::toJson() const
{
	Json data = Objective::toJson();
	data["project"] = reinterpret_cast<uintptr_t>(&m_project);
	return data;
}
/*
TargetedHaulProject::TargetedHaulProject(const Json& data, DeserializationMemo& deserializationMemo) :
	Project(data, deserializationMemo), m_item(deserializationMemo.itemReference(data["item"])) { }
Json TargetedHaulProject::toJson() const
{
	Json data = Project::toJson();
	data["item"] = m_item;
	return data;
}
*/
void TargetedHaulProject::onComplete()
{
	auto workers = std::move(m_workers);
	m_area.m_hasTargetedHauling.complete(*this);
	Actors& actors = m_area.getActors();
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor, projectWorker.objective);
}
void TargetedHaulProject::onDelivered(ActorOrItemIndex delivered) { delivered.setLocationAndFacing(m_area, m_location, 0); }
// Objective.
TargetedHaulObjective::TargetedHaulObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_project(*static_cast<TargetedHaulProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()))) { }
void Objective::reset(Area& area) { area.getActors().canReserve_clearAll(m_actor); }
// AreaHas.
void AreaHasTargetedHauling::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& project : data["projects"])
		m_projects.emplace_back(project, deserializationMemo);
}
Json AreaHasTargetedHauling::toJson() const 
{
	Json data{{"projects", Json::array()}};
	for(const Project& project : m_projects)
		data["projects"].push_back(project);
	return data;
}
TargetedHaulProject& AreaHasTargetedHauling::begin(std::vector<ActorIndex> workers, ItemIndex item, BlockIndex destination)
{
	Actors& actors = m_area.getActors();
	TargetedHaulProject& project = m_projects.emplace_back(actors.getFaction(workers.front()), m_area, destination, item);
	for(ActorIndex actor : workers)
	{
		std::unique_ptr<Objective> objective = std::make_unique<TargetedHaulObjective>(actor, project);
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
