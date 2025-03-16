#include "construct.h"
#include "deserializationMemo.h"
#include "area/area.h"
#include "reservable.h"
#include "path/terrainFacade.h"
#include "types.h"
#include "util.h"
#include "blockFeature.h"
#include "simulation/simulation.h"
#include "materialType.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "objectives/construct.h"
#include <memory>
// Project.
ConstructProject::ConstructProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) : Project(data, deserializationMemo, area),
	m_blockFeatureType(data.contains("blockFeatureType") ? &BlockFeatureType::byName(data["blockFeatureType"].get<std::string>()) : nullptr),
	m_materialType(MaterialType::byName(data["materialType"].get<std::string>())) { }
Json ConstructProject::toJson() const
{
	Json data = Project::toJson();
	if(m_blockFeatureType)
		data["blockFeatureType"] = m_blockFeatureType->name;
	data["materialType"] = m_materialType;
	return data;
}
std::vector<std::pair<ItemQuery, Quantity>> ConstructProject::getConsumed() const
{
	return MaterialType::construction_getConsumed(m_materialType);
}
std::vector<std::pair<ItemQuery, Quantity>> ConstructProject::getUnconsumed() const
{
	return MaterialType::construction_getUnconsumed(m_materialType);
}
std::vector<ActorReference> ConstructProject::getActors() const { return {}; }
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> ConstructProject::getByproducts() const
{
	return MaterialType::construction_getByproducts(m_materialType);
}
uint32_t ConstructProject::getWorkerConstructScore(const ActorIndex& actor) const
{
	Actors& actors = m_area.getActors();
	SkillTypeId constructSkill = MaterialType::construction_getSkill(m_materialType);
	return (actors.getStrength(actor).get() * Config::constructStrengthModifier) +
		(actors.skill_getLevel(actor, constructSkill).get() * Config::constructSkillModifier);
}
void ConstructProject::onComplete()
{
	Blocks& blocks = m_area.getBlocks();
	Actors& actors = m_area.getActors();
	assert(!blocks.solid_is(m_location));
	// Store values we will need to complete before destroying construction project.
	const auto blockFeatureType = m_blockFeatureType;
	const auto location = m_location;
	const auto materialType = m_materialType;
	auto workers = std::move(m_workers);
	// Destroy project.
	m_area.m_hasConstructionDesignations.remove(m_faction, m_location);
	if(blockFeatureType == nullptr)
	{
		blocks.item_disperseAll(location);
		//TODO: disperse actors.
		// Arguments: materialType, constructed.
		blocks.solid_set(location, materialType, true);
	}
	else
		blocks.blockFeature_construct(location, *blockFeatureType, materialType);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
void ConstructProject::onCancel()
{
	ActorIndices copy = getWorkersAndCandidates();
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
	m_area.getBlocks().designation_maybeUnset(m_location, m_faction, BlockDesignation::Construct);
}
void ConstructProject::offDelay()
{
	m_area.getBlocks().designation_set(m_location, m_faction, BlockDesignation::Construct);
}
// What would the total delay time be if we started from scratch now with current workers?
Step ConstructProject::getDuration() const
{
	uint32_t totalScore = 0;
	Actors& actors = m_area.getActors();
	for(auto& pair : m_workers)
		totalScore += getWorkerConstructScore(pair.first.getIndex(actors.m_referenceData));
	return MaterialType::construction_getDuration(m_materialType) / totalScore;
}
ConstructionLocationDishonorCallback::ConstructionLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) :
	m_faction(data["faction"].get<FactionId>()),
	m_area(deserializationMemo.area(data["area"])),
	m_location(data["location"].get<BlockIndex>()) { }
Json ConstructionLocationDishonorCallback::toJson() const { return Json({{"type", "ConstructionLocationDishonorCallback"}, {"faction", m_faction}, {"location", m_location}}); }
void ConstructionLocationDishonorCallback::execute([[maybe_unused]] const Quantity& oldCount, [[maybe_unused]] const Quantity& newCount)
{
	m_area.m_hasConstructionDesignations.getForFaction(m_faction).undesignate(m_location);
}