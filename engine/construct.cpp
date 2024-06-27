#include "construct.h"
#include "deserializationMemo.h"
#include "area.h"
#include "reservable.h"
#include "terrainFacade.h"
#include "types.h"
#include "util.h"
#include "simulation.h"
#include <memory>
/*
// Input.
void DesignateConstructInputAction::execute()
{
	BlockIndex block = *m_cuboid.begin();
	auto& constructDesginations = block.m_area->m_hasConstructionDesignations;
	Faction& faction = *(**m_actors.begin()).getFaction();
	for(BlockIndex block : m_cuboid)
		constructDesginations.designate(faction, block, m_blockFeatureType, m_materialType);
};
void UndesignateConstructInputAction::execute()
{
	BlockIndex block = *m_cuboid.begin();
	auto& constructDesginations = block.m_area->m_hasConstructionDesignations;
	Faction& faction = *(**m_actors.begin()).getFaction();
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
	data["materialType"] = m_materialType.name;
	return data;
}
std::vector<std::pair<ItemQuery, uint32_t>> ConstructProject::getConsumed() const
{
	assert(m_materialType.constructionData != nullptr);
	return m_materialType.constructionData->consumed;
}
std::vector<std::pair<ItemQuery, uint32_t>> ConstructProject::getUnconsumed() const
{
	assert(m_materialType.constructionData != nullptr);
	return m_materialType.constructionData->unconsumed;
}
std::vector<std::pair<ActorQuery, uint32_t>> ConstructProject::getActors() const { return {}; }
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> ConstructProject::getByproducts() const
{
	assert(m_materialType.constructionData != nullptr);
	return m_materialType.constructionData->byproducts;
}
uint32_t ConstructProject::getWorkerConstructScore(ActorIndex actor) const
{
	Actors& actors = m_area.getActors();
	const SkillType& constructSkill = m_materialType.constructionData->skill;
	return (actors.getStrength(actor) * Config::constructStrengthModifier) + 
		(actors.skill_getLevel(actor, constructSkill) * Config::constructSkillModifier);
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
		actors.objective_complete(actor, projectWorker.objective);
}
void ConstructProject::onCancel()
{
	std::vector<ActorIndex> copy = getWorkersAndCandidates();
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
		totalScore += getWorkerConstructScore(pair.first);
	return m_materialType.constructionData->duration / totalScore;
}
ConstructionLocationDishonorCallback::ConstructionLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) :
	m_faction(deserializationMemo.faction(data["faction"].get<std::wstring>())),
	m_area(deserializationMemo.area(data["area"])),
	m_location(data["location"].get<BlockIndex>()) { }
Json ConstructionLocationDishonorCallback::toJson() const { return Json({{"type", "ConstructionLocationDishonorCallback"}, {"faction", m_faction.name}, {"location", m_location}}); }
void ConstructionLocationDishonorCallback::execute([[maybe_unused]] uint32_t oldCount, [[maybe_unused]] uint32_t newCount)
{
	m_area.m_hasConstructionDesignations.at(m_faction).undesignate(m_location);
}
HasConstructionDesignationsForFaction::HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Faction& faction) :
	m_area(deserializationMemo.area(data["area"])), m_faction(faction)
{
	for(const Json& pair : data["projects"])
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data.try_emplace(block, pair[1], deserializationMemo);
	}
}
void HasConstructionDesignationsForFaction::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data["projects"])
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data.at(block).loadWorkers(pair[1], deserializationMemo);
	}
}
Json HasConstructionDesignationsForFaction::toJson() const
{
	Json projects;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second.toJson()};
		projects.push_back(jsonPair);
	}
	return {{"area", m_area}, {"projects", projects}};
}
// If blockFeatureType is null then construct a wall rather then a feature.
void HasConstructionDesignationsForFaction::designate(BlockIndex block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType)
{
	assert(!contains(block));
	Blocks& blocks = m_area.getBlocks();
	blocks.designation_set(block, m_faction, BlockDesignation::Construct);
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<ConstructionLocationDishonorCallback>(m_faction, m_area, block);
	m_data.try_emplace(block, &m_faction, m_area, block, blockFeatureType, materialType, std::move(locationDishonorCallback));
}
void HasConstructionDesignationsForFaction::undesignate(BlockIndex block)
{
	assert(m_data.contains(block));
	ConstructProject& project = m_data.at(block);
	project.cancel();
}
void HasConstructionDesignationsForFaction::remove(BlockIndex block)
{
	assert(contains(block));
	m_data.erase(block);
	m_area.getBlocks().designation_unset(block, m_faction, BlockDesignation::Construct);
}
void AreaHasConstructionDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second.m_data)
			pair2.second.clearReservations();
}
void HasConstructionDesignationsForFaction::removeIfExists(BlockIndex block)
{
	if(m_data.contains(block))
		remove(block);
}
bool HasConstructionDesignationsForFaction::contains(BlockIndex block) const { return m_data.contains(block); }
const BlockFeatureType* HasConstructionDesignationsForFaction::at(BlockIndex block) const { return m_data.at(block).m_blockFeatureType; }
bool HasConstructionDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void AreaHasConstructionDesignations::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		Faction& faction = deserializationMemo.faction(pair[0]);
		m_data.try_emplace(&faction, pair[1], deserializationMemo, faction);
	}
}
void AreaHasConstructionDesignations::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		Faction& faction = deserializationMemo.faction(pair[0]);
		m_data.at(&faction).loadWorkers(pair[1], deserializationMemo);
	}

}
Json AreaHasConstructionDesignations::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair;
		jsonPair[0] = pair.first->name;
		jsonPair[1] = pair.second.toJson();
		data.push_back(jsonPair);
	}
	return data;
}
void AreaHasConstructionDesignations::addFaction(Faction& faction)
{
	assert(!m_data.contains(&faction));
	m_data.try_emplace(&faction, faction, m_area);
}
void AreaHasConstructionDesignations::removeFaction(Faction& faction)
{
	assert(m_data.contains(&faction));
	m_data.erase(&faction);
}
void AreaHasConstructionDesignations::designate(Faction& faction, BlockIndex block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType)
{
	if(!m_data.contains(&faction))
		addFaction(faction);
	m_data.at(&faction).designate(block, blockFeatureType, materialType);
}
void AreaHasConstructionDesignations::undesignate(Faction& faction, BlockIndex block)
{
	m_data.at(&faction).undesignate(block);
}
void AreaHasConstructionDesignations::remove(Faction& faction, BlockIndex block)
{
	assert(m_data.contains(&faction));
	m_data.at(&faction).remove(block);
}
void AreaHasConstructionDesignations::clearAll(BlockIndex block)
{
	for(auto& pair : m_data)
		pair.second.removeIfExists(block);
}
bool AreaHasConstructionDesignations::areThereAnyForFaction(Faction& faction) const
{
	if(!m_data.contains(&faction))
		return false;
	return !m_data.at(&faction).empty();
}
bool AreaHasConstructionDesignations::contains(Faction& faction, BlockIndex block) const
{
	if(!m_data.contains(&faction))
		return false;
	return m_data.at(&faction).contains(block);
}
ConstructProject& AreaHasConstructionDesignations::getProject(Faction& faction, BlockIndex block) { return m_data.at(&faction).m_data.at(block); }
