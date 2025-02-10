#include "targetedHaul.h"
#include "area.h"
#include "deserializationMemo.h"
#include "items/items.h"
#include "objective.h"
#include "project.h"
#include "actors/actors.h"
#include "objectives/targetedHaul.h"
TargetedHaulProject::TargetedHaulProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	Project(data, deserializationMemo, area)
{
	if(data.contains("item"))
		m_item.load(data["item"], area.getItems().m_referenceData);
}
Json TargetedHaulProject::toJson() const
{
	Json data = Project::toJson();
	data["item"] = m_item;
	return data;
}
void TargetedHaulProject::onComplete()
{
	auto workers = std::move(m_workers);
	Actors& actors = m_area.getActors();
	Items& items = m_area.getItems();
	ItemIndex item = m_item.getIndex(items.m_referenceData);
	m_item.clear();
	if(items.getLocation(item) != m_location)
		items.setLocationAndFacing(item, m_location, actors.getFacing(workers.begin()->first.getIndex(actors.m_referenceData)));
	m_area.m_hasTargetedHauling.complete(*this);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
void TargetedHaulProject::onDelivered(const ActorOrItemIndex& delivered) { delivered.setLocationAndFacing(m_area, m_location, Facing4::North); }
// Objective.
// AreaHas.
void AreaHasTargetedHauling::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	for(const Json& project : data["projects"])
		m_projects.emplace_back(project, deserializationMemo, area);
}
TargetedHaulProject& AreaHasTargetedHauling::begin(const SmallSet<ActorIndex>& workers, const ItemIndex& item, const BlockIndex& destination)
{
	Actors& actors = m_area.getActors();
	ItemReference ref = m_area.getItems().getReference(item);
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
