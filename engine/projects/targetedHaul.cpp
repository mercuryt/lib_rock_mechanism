#include "targetedHaul.h"
#include "area/area.h"
#include "deserializationMemo.h"
#include "items/items.h"
#include "objective.h"
#include "project.h"
#include "actors/actors.h"
#include "objectives/targetedHaul.h"
TargetedHaulProject::TargetedHaulProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	Project(data, deserializationMemo, area)
{
	if(data.contains("target"))
		m_target.load(data["target"], area);
}
Json TargetedHaulProject::toJson() const
{
	Json data = Project::toJson();
	data["target"] = m_target;
	return data;
}
void TargetedHaulProject::onComplete()
{
	auto workers = std::move(m_workers);
	Actors& actors = m_area.getActors();
	Items& items = m_area.getItems();
	ActorOrItemIndex target = m_target.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData);
	m_target.clear();
	if(target.getLocation(m_area) != m_location)
		target.location_set(m_area, m_location, actors.getFacing(workers.begin()->first.getIndex(actors.m_referenceData)));
	m_area.m_hasTargetedHauling.complete(*this);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
void TargetedHaulProject::onDelivered(const ActorOrItemIndex& delivered)
{
	delivered.location_set(m_area, m_location, Facing4::North);
}