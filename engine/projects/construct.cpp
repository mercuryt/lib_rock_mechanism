#include "construct.h"
#include "deserializationMemo.h"
#include "area/area.h"
#include "reservable.h"
#include "path/terrainFacade.h"
#include "numericTypes/types.h"
#include "util.h"
#include "pointFeature.h"
#include "simulation/simulation.h"
#include "definitions/materialType.h"
#include "actors/actors.h"
#include "space/space.h"
#include "objectives/construct.h"
#include <memory>
// Project.
ConstructProject::ConstructProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) : Project(data, deserializationMemo, area),
	m_pointFeatureType(data.contains("pointFeatureType") ? data["pointFeatureType"].get<PointFeatureTypeId>() : PointFeatureTypeId::Null),
	m_solid(MaterialType::byName(data["materialType"].get<std::string>())) { }
Json ConstructProject::toJson() const
{
	Json data = Project::toJson();
	if(m_pointFeatureType != PointFeatureTypeId::Null)
		data["pointFeatureType"] = PointFeatureType::byId(m_pointFeatureType).name;
	data["materialType"] = m_solid;
	return data;
}
std::vector<std::pair<ItemQuery, Quantity>> ConstructProject::getConsumed() const
{
	return MaterialType::construction_getConsumed(m_solid);
}
std::vector<std::pair<ItemQuery, Quantity>> ConstructProject::getUnconsumed() const
{
	return MaterialType::construction_getUnconsumed(m_solid);
}
std::vector<ActorReference> ConstructProject::getActors() const { return {}; }
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> ConstructProject::getByproducts() const
{
	return MaterialType::construction_getByproducts(m_solid);
}
uint32_t ConstructProject::getWorkerConstructScore(const ActorIndex& actor) const
{
	Actors& actors = m_area.getActors();
	SkillTypeId constructSkill = MaterialType::construction_getSkill(m_solid);
	return (actors.getStrength(actor).get() * Config::constructStrengthModifier) +
		(actors.skill_getLevel(actor, constructSkill).get() * Config::constructSkillModifier);
}
void ConstructProject::onComplete()
{
	Space& space = m_area.getSpace();
	Actors& actors = m_area.getActors();
	assert(!space.solid_isAny(m_location));
	// Store values we will need to copy before destroying construction project.
	const auto pointFeatureType = m_pointFeatureType;
	const auto location = m_location;
	const auto materialType = m_solid;
	auto workers = std::move(m_workers);
	// Destroy project.
	m_area.m_hasConstructionDesignations.remove(m_faction, m_location);
	if(pointFeatureType == PointFeatureTypeId::Null)
	{
		space.item_disperseAll(location);
		//TODO: disperse actors.
		// Arguments: materialType, constructed.
		space.solid_set(location, materialType, true);
	}
	else
		space.pointFeature_construct(location, pointFeatureType, materialType);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
void ConstructProject::onCancel()
{
	SmallSet<ActorIndex> copy = getWorkersAndCandidates();
	Actors& actors = m_area.getActors();
	m_area.m_hasConstructionDesignations.remove(m_faction, m_location);
	for(ActorIndex actor : copy)
	{
		actors.objective_getCurrent<ConstructObjective>(actor).m_project = nullptr;
		actors.objective_reset(actor);
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
void ConstructProject::onDelay()
{
	m_area.getSpace().designation_maybeUnset(m_location, m_faction, SpaceDesignation::Construct);
}
void ConstructProject::offDelay()
{
	m_area.getSpace().designation_set(m_location, m_faction, SpaceDesignation::Construct);
}
// What would the total delay time be if we started from scratch now with current workers?
Step ConstructProject::getDuration() const
{
	uint32_t totalScore = 0;
	Actors& actors = m_area.getActors();
	for(auto& pair : m_workers)
		totalScore += getWorkerConstructScore(pair.first.getIndex(actors.m_referenceData));
	return MaterialType::construction_getDuration(m_solid) / totalScore;
}
ConstructionLocationDishonorCallback::ConstructionLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) :
	m_faction(data["faction"].get<FactionId>()),
	m_area(deserializationMemo.area(data["area"])),
	m_location(data["location"].get<Point3D>()) { }
Json ConstructionLocationDishonorCallback::toJson() const { return Json({{"type", "ConstructionLocationDishonorCallback"}, {"faction", m_faction}, {"location", m_location}}); }
void ConstructionLocationDishonorCallback::execute([[maybe_unused]] const Quantity& oldCount, [[maybe_unused]] const Quantity& newCount)
{
	m_area.m_hasConstructionDesignations.getForFaction(m_faction).undesignate(m_location);
}