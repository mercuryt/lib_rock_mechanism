#include "construct.h"
#include "deserializationMemo.h"
#include "area.h"
#include "reservable.h"
#include "path/terrainFacade.h"
#include "types.h"
#include "util.h"
#include "blockFeature.h"
#include "simulation.h"
#include "materialType.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "objectives/construct.h"
#include <memory>
/*
// Input.
void DesignateConstructInputAction::execute()
{
	BlockIndex block = *m_cuboid.begin();
	auto& constructDesginations = block.m_area->m_hasConstructionDesignations;
	FactionId faction = *(**m_actors.begin()).getFaction();
	for(BlockIndex block : m_cuboid)
		constructDesginations.designate(faction, block, m_blockFeatureType, m_materialType);
};
void UndesignateConstructInputAction::execute()
{
	BlockIndex block = *m_cuboid.begin();
	auto& constructDesginations = block.m_area->m_hasConstructionDesignations;
	FactionId faction = *(**m_actors.begin()).getFaction();
	for(BlockIndex block : m_cuboid)
		constructDesginations.undesignate(faction, block);
}
*/
// Project.
ConstructProject::ConstructProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) : Project(data, deserializationMemo, area),
	m_blockFeatureType(data.contains("blockFeatureType") ? &BlockFeatureType::byName(data["blockFeatureType"].get<std::wstring>()) : nullptr),
	m_materialType(MaterialType::byName(data["materialType"].get<std::wstring>())) { }
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
std::vector<std::pair<ActorQuery, Quantity>> ConstructProject::getActors() const { return {}; }
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
HasConstructionDesignationsForFaction::HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction, Area& area) :
	m_faction(faction)
{
	for(const Json& pair : data["projects"])
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data.emplace(block, pair[1], deserializationMemo, area);
	}
}
void HasConstructionDesignationsForFaction::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data["projects"])
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data[block].loadWorkers(pair[1], deserializationMemo);
	}
}
Json HasConstructionDesignationsForFaction::toJson() const
{
	Json projects;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second->toJson()};
		projects.push_back(jsonPair);
	}
	return {{"projects", projects}};
}
// If blockFeatureType is null then construct a wall rather then a feature.
void HasConstructionDesignationsForFaction::designate(Area& area, const BlockIndex& block, const BlockFeatureType* blockFeatureType, const MaterialTypeId& materialType)
{
	assert(!contains(block));
	Blocks& blocks = area.getBlocks();
	blocks.designation_set(block, m_faction, BlockDesignation::Construct);
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<ConstructionLocationDishonorCallback>(m_faction, area, block);
	m_data.emplace(block, m_faction, area, block, blockFeatureType, materialType, std::move(locationDishonorCallback));
}
void HasConstructionDesignationsForFaction::undesignate(const BlockIndex& block)
{
	assert(m_data.contains(block));
	ConstructProject& project = m_data[block];
	project.cancel();
}
void HasConstructionDesignationsForFaction::remove(Area& area, const BlockIndex& block)
{
	assert(contains(block));
	area.getBlocks().designation_unset(block, m_faction, BlockDesignation::Construct);
	m_data.erase(block);
}
void AreaHasConstructionDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second->m_data)
			pair2.second->clearReservations();
}
void HasConstructionDesignationsForFaction::removeIfExists(Area& area, const BlockIndex& block)
{
	if(m_data.contains(block))
		remove(area, block);
}
bool HasConstructionDesignationsForFaction::contains(const BlockIndex& block) const { return m_data.contains(block); }
const BlockFeatureType* HasConstructionDesignationsForFaction::getForBlock(const BlockIndex& block) const { return m_data[block].m_blockFeatureType; }
bool HasConstructionDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void AreaHasConstructionDesignations::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.emplace(faction, pair[1], deserializationMemo, faction, area);
	}
}
void AreaHasConstructionDesignations::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data[faction].loadWorkers(pair[1], deserializationMemo);
	}

}
Json AreaHasConstructionDesignations::toJson() const
{
	Json data = Json::array();
	for(auto& pair : m_data)
	{
		Json jsonPair;
		jsonPair[0] = pair.first;
		jsonPair[1] = pair.second->toJson();
		data.push_back(jsonPair);
	}
	return data;
}
void AreaHasConstructionDesignations::addFaction(const FactionId& faction)
{
	assert(!m_data.contains(faction));
	m_data.emplace(faction, faction);
}
void AreaHasConstructionDesignations::removeFaction(const FactionId& faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
void AreaHasConstructionDesignations::designate(const FactionId& faction, const BlockIndex& block, const BlockFeatureType* blockFeatureType, const MaterialTypeId& materialType)
{
	if(!m_data.contains(faction))
		addFaction(faction);
	m_data[faction].designate(m_area, block, blockFeatureType, materialType);
}
void AreaHasConstructionDesignations::undesignate(const FactionId& faction, const BlockIndex& block)
{
	m_data[faction].undesignate(block);
}
void AreaHasConstructionDesignations::remove(const FactionId& faction, const BlockIndex& block)
{
	assert(m_data.contains(faction));
	m_data[faction].remove(m_area, block);
}
void AreaHasConstructionDesignations::clearAll(const BlockIndex& block)
{
	for(auto& pair : m_data)
		pair.second->removeIfExists(m_area, block);
}
bool AreaHasConstructionDesignations::areThereAnyForFaction(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return !m_data[faction].empty();
}
bool AreaHasConstructionDesignations::contains(const FactionId& faction, const BlockIndex& block) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].contains(block);
}
ConstructProject& AreaHasConstructionDesignations::getProject(const FactionId& faction, const BlockIndex& block) { return m_data[faction].m_data[block]; }
