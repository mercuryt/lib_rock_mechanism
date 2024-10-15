#include "construct.h"
#include "deserializationMemo.h"
#include "area.h"
#include "reservable.h"
#include "terrainFacade.h"
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
ConstructProject::ConstructProject(const Json& data, DeserializationMemo& deserializationMemo) : Project(data, deserializationMemo),
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
std::vector<std::pair<ActorQuery, Quantity>> ConstructProject::getActors() const { return {}; }
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> ConstructProject::getByproducts() const
{
	return MaterialType::construction_getByproducts(m_materialType);
}
uint32_t ConstructProject::getWorkerConstructScore(ActorIndex actor) const
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
	if(m_blockFeatureType == nullptr)
	{
		blocks.item_disperseAll(m_location);
		//TODO: disperse actors.
		// Arguments: materialType, constructed.
		blocks.solid_set(m_location, m_materialType, true);
	}
	else
		blocks.blockFeature_construct(m_location, *m_blockFeatureType, m_materialType);
	auto workers = std::move(m_workers);
	m_area.m_hasConstructionDesignations.clearAll(m_location);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(), *projectWorker.objective);
}
void ConstructProject::onCancel()
{
	ActorIndices copy = getWorkersAndCandidates();
	m_area.m_hasConstructionDesignations.remove(m_faction, m_location);
	Actors& actors = m_area.getActors();
	for(ActorIndex actor : copy)
	{
		actors.objective_getCurrent<ConstructObjective>(actor).m_project = nullptr;
		actors.project_unset(actor);
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
	for(auto& pair : m_workers)
		totalScore += getWorkerConstructScore(pair.first.getIndex());
	return MaterialType::construction_getDuration(m_materialType) / totalScore;
}
ConstructionLocationDishonorCallback::ConstructionLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) :
	m_faction(data["faction"].get<FactionId>()),
	m_area(deserializationMemo.area(data["area"])),
	m_location(data["location"].get<BlockIndex>()) { }
Json ConstructionLocationDishonorCallback::toJson() const { return Json({{"type", "ConstructionLocationDishonorCallback"}, {"faction", m_faction}, {"location", m_location}}); }
void ConstructionLocationDishonorCallback::execute([[maybe_unused]] Quantity oldCount, [[maybe_unused]] Quantity newCount)
{
	m_area.m_hasConstructionDesignations.getForFaction(m_faction).undesignate(m_location);
}
HasConstructionDesignationsForFaction::HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, FactionId faction) :
	m_faction(faction)
{
	for(const Json& pair : data["projects"])
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data.emplace(block, pair[1], deserializationMemo);
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
void HasConstructionDesignationsForFaction::designate(Area& area, BlockIndex block, const BlockFeatureType* blockFeatureType, MaterialTypeId materialType)
{
	assert(!contains(block));
	Blocks& blocks = area.getBlocks();
	blocks.designation_set(block, m_faction, BlockDesignation::Construct);
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<ConstructionLocationDishonorCallback>(m_faction, area, block);
	m_data.emplace(block, m_faction, area, block, blockFeatureType, materialType, std::move(locationDishonorCallback));
}
void HasConstructionDesignationsForFaction::undesignate(BlockIndex block)
{
	assert(m_data.contains(block));
	ConstructProject& project = m_data[block];
	project.cancel();
}
void HasConstructionDesignationsForFaction::remove(Area& area, BlockIndex block)
{
	assert(contains(block));
	m_data.erase(block);
	area.getBlocks().designation_unset(block, m_faction, BlockDesignation::Construct);
}
void AreaHasConstructionDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second->m_data)
			pair2.second->clearReservations();
}
void HasConstructionDesignationsForFaction::removeIfExists(Area& area, BlockIndex block)
{
	if(m_data.contains(block))
		remove(area, block);
}
bool HasConstructionDesignationsForFaction::contains(BlockIndex block) const { return m_data.contains(block); }
const BlockFeatureType* HasConstructionDesignationsForFaction::getForBlock(BlockIndex block) const { return m_data[block].m_blockFeatureType; }
bool HasConstructionDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void AreaHasConstructionDesignations::load([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.emplace(faction, pair[1], deserializationMemo, faction);
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
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair;
		jsonPair[0] = pair.first;
		jsonPair[1] = pair.second->toJson();
		data.push_back(jsonPair);
	}
	return data;
}
void AreaHasConstructionDesignations::addFaction(FactionId faction)
{
	assert(!m_data.contains(faction));
	m_data.emplace(faction, faction);
}
void AreaHasConstructionDesignations::removeFaction(FactionId faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
void AreaHasConstructionDesignations::designate(FactionId faction, BlockIndex block, const BlockFeatureType* blockFeatureType, MaterialTypeId materialType)
{
	if(!m_data.contains(faction))
		addFaction(faction);
	m_data[faction].designate(m_area, block, blockFeatureType, materialType);
}
void AreaHasConstructionDesignations::undesignate(FactionId faction, BlockIndex block)
{
	m_data[faction].undesignate(block);
}
void AreaHasConstructionDesignations::remove(FactionId faction, BlockIndex block)
{
	assert(m_data.contains(faction));
	m_data[faction].remove(m_area, block);
}
void AreaHasConstructionDesignations::clearAll(BlockIndex block)
{
	for(auto& pair : m_data)
		pair.second->removeIfExists(m_area, block);
}
bool AreaHasConstructionDesignations::areThereAnyForFaction(FactionId faction) const
{
	if(!m_data.contains(faction))
		return false;
	return !m_data[faction].empty();
}
bool AreaHasConstructionDesignations::contains(FactionId faction, BlockIndex block) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].contains(block);
}
ConstructProject& AreaHasConstructionDesignations::getProject(FactionId faction, BlockIndex block) { return m_data[faction].m_data[block]; }
