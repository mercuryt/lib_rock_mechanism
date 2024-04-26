#include "targetedHaul.h"
#include "block.h"
#include "area.h"
#include "actor.h"
#include "deserializationMemo.h"
#include "objective.h"
#include "project.h"
#include "stockpile.h"
Json TargetedHaulObjective::toJson() const
{
	Json data = Objective::toJson();
	data["project"] = reinterpret_cast<uintptr_t>(&m_project);
	return data;
}
TargetedHaulProject::TargetedHaulProject(const Json& data, DeserializationMemo& deserializationMemo) : Project(data, deserializationMemo), m_item(deserializationMemo.itemReference(data["item"])) { }
Json TargetedHaulProject::toJson() const
{
	Json data = Project::toJson();
	data["item"] = m_item;
	return data;
}
void TargetedHaulProject::onComplete()
{
	auto workers = std::move(m_workers);
	m_location.m_area->m_hasTargetedHauling.complete(*this);
	for(auto& [actor, projectWorker] : workers)
		actor->m_hasObjectives.objectiveComplete(projectWorker.objective);
}
// Objective.
TargetedHaulObjective::TargetedHaulObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_project(*static_cast<TargetedHaulProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()))) { }
void Objective::reset() { m_actor.m_canReserve.deleteAllWithoutCallback(); }
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
void AreaHasTargetedHauling::clearReservations()
{
	for(Project& project : m_projects)
		project.clearReservations();
}
