#include "woodcutting.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "plants.h"
#include "area.h"
#include "blockFeature.h"
#include "deserializationMemo.h"
#include "random.h"
#include "reservable.h"
#include "terrainFacade.h"
#include "types.h"
#include "util.h"
#include "simulation.h"
#include "itemType.h"
#include "objectives/woodcutting.h"
#include <memory>
#include <sys/types.h>
/*
// Input.
void DesignateWoodCuttingInputAction::execute()
{
	BlockIndex block = *m_blocks.begin();
	auto& woodCuttingDesginations = m_area->m_hasWoodCuttingDesignations;
	FactionId faction = *(**m_actors.begin()).getFaction();
	for(BlockIndex block : m_blocks)
		woodCuttingDesginations.designate(faction, block);
};
void UndesignateWoodCuttingInputAction::execute()
{
	BlockIndex block = *m_blocks.begin();
	auto& woodCuttingDesginations = block.m_area->m_hasWoodCuttingDesignations;
	FactionId faction = *(**m_actors.begin()).getFaction();
	for(BlockIndex block : m_blocks)
		woodCuttingDesginations.undesignate(faction, block);
}
*/
WoodCuttingProject::WoodCuttingProject(const Json& data, DeserializationMemo& deserializationMemo) : Project(data, deserializationMemo) { }
std::vector<std::pair<ItemQuery, Quantity>> WoodCuttingProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, Quantity>> WoodCuttingProject::getUnconsumed() const { return {{ItemType::byName("axe"), Quantity::create(1)}}; }
std::vector<std::pair<ActorQuery, Quantity>> WoodCuttingProject::getActors() const { return {}; }
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> WoodCuttingProject::getByproducts() const
{
	PlantIndex plant = m_area.getBlocks().plant_get(m_location);
	Plants& plants = m_area.getPlants();
	Percent percentGrown = plants.getPercentGrown(plant);
	PlantSpeciesId species = plants.getSpecies(plant);
	uint logs = PlantSpecies::getLogsGeneratedByFellingWhenFullGrown(species).get();
	uint branches = PlantSpecies::getLogsGeneratedByFellingWhenFullGrown(species).get();
	Quantity unitsLogsGenerated = Quantity::create(util::scaleByPercent(logs, percentGrown));
	Quantity unitsBranchesGenerated = Quantity::create(util::scaleByPercent(branches, percentGrown));
	assert(unitsLogsGenerated != 0);
	assert(unitsBranchesGenerated != 0);
	auto woodType = PlantSpecies::getWoodType(species);
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> output{
		
		{ItemType::byName("branch"), woodType, unitsBranchesGenerated},
		{ItemType::byName("log"), woodType, unitsLogsGenerated}
	};
	return output;
}
// Static.
uint32_t WoodCuttingProject::getWorkerWoodCuttingScore(Area& area, ActorIndex actor)
{
	static SkillTypeId woodCuttingType = SkillType::byName("wood cutting");
	Actors& actors = area.getActors();
	uint32_t strength = actors.getStrength(actor).get();
	uint32_t skill = actors.skill_getLevel(actor, woodCuttingType).get();
	return (strength * Config::woodCuttingStrengthModifier) + (skill * Config::woodCuttingSkillModifier);
}
void WoodCuttingProject::onComplete()
{
	if(m_area.getBlocks().plant_exists(m_location))
	{
		PlantIndex plant = m_area.getBlocks().plant_get(m_location);
		m_area.getPlants().die(plant);
	}
	// Remove designations for other factions as well as owning faction.
	auto workers = std::move(m_workers);
	m_area.m_hasWoodCuttingDesignations.clearAll(m_location);
	Actors& actors = m_area.getActors();
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(), *projectWorker.objective);
}
void WoodCuttingProject::onCancel()
{
	m_area.m_hasWoodCuttingDesignations.remove(m_faction, m_location);
	Actors& actors = m_area.getActors();
	for(ActorIndex actor : getWorkersAndCandidates())
	{
		// TODO: These two lines seem redundant with the third.
		actors.project_unset(actor);
		actors.objective_getCurrent<WoodCuttingObjective>(actor).reset(m_area, actor);
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
void WoodCuttingProject::onDelay()
{
	m_area.getBlocks().designation_maybeUnset(m_location, m_faction, BlockDesignation::WoodCutting);
}
void WoodCuttingProject::offDelay()
{
	m_area.getBlocks().designation_set(m_location, m_faction, BlockDesignation::WoodCutting);
}
// What would the total delay time be if we started from scratch now with current workers?
Step WoodCuttingProject::getDuration() const
{
	uint32_t totalScore = 0u;
	for(auto& pair : m_workers)
		totalScore += getWorkerWoodCuttingScore(m_area, pair.first.getIndex());
	return std::max(Step::create(1u), Config::woodCuttingMaxSteps / totalScore);
}
WoodCuttingLocationDishonorCallback::WoodCuttingLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) : 
	m_faction(data["faction"].get<FactionId>()),
	m_area(deserializationMemo.area(data["area"])),
	m_location(data["location"].get<BlockIndex>()) { }
Json WoodCuttingLocationDishonorCallback::toJson() const { return Json({{"type", "WoodCuttingLocationDishonorCallback"}, {"faction", m_faction}, {"location", m_location}}); }
void WoodCuttingLocationDishonorCallback::execute([[maybe_unused]] Quantity oldCount, [[maybe_unused]] Quantity newCount)
{
	m_area.m_hasWoodCuttingDesignations.undesignate(m_faction, m_location);
}
HasWoodCuttingDesignationsForFaction::HasWoodCuttingDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, FactionId faction) : 
	m_area(deserializationMemo.area(data["area"])),
	m_faction(faction)
{
	for(const Json& pair : data)
	{
		BlockIndex block = pair[0].get<BlockIndex>();
		m_data.try_emplace(block, pair[1], deserializationMemo);
	}
}
Json HasWoodCuttingDesignationsForFaction::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second.toJson()};
		data.push_back(jsonPair);
	}
	return data;
}
void HasWoodCuttingDesignationsForFaction::designate(BlockIndex block)
{
	Blocks& blocks = m_area.getBlocks();
	Plants& plants = m_area.getPlants();
	assert(!m_data.contains(block));
	assert(blocks.plant_exists(block));
	assert(PlantSpecies::getIsTree(plants.getSpecies(blocks.plant_get(block))));
	assert(plants.getPercentGrown(blocks.plant_get(block)) >= Config::minimumPercentGrowthForWoodCutting);
	blocks.designation_set(block, m_faction, BlockDesignation::WoodCutting);
	// To be called when block is no longer a suitable location, for example if it got crushed by a collapse.
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<WoodCuttingLocationDishonorCallback>(m_faction, m_area, block);
	m_data.try_emplace(block, m_faction, m_area, block, std::move(locationDishonorCallback));
}
void HasWoodCuttingDesignationsForFaction::undesignate(BlockIndex block)
{
	assert(m_data.contains(block));
	WoodCuttingProject& project = m_data.at(block);
	project.cancel();
}
void HasWoodCuttingDesignationsForFaction::remove(BlockIndex block)
{
	assert(m_data.contains(block));
	m_area.getBlocks().designation_unset(block, m_faction, BlockDesignation::WoodCutting);
	m_data.erase(block); 
}
void HasWoodCuttingDesignationsForFaction::removeIfExists(BlockIndex block)
{
	if(m_data.contains(block))
		remove(block);
}
bool HasWoodCuttingDesignationsForFaction::empty() const { return m_data.empty(); }
// To be used by Area.
void AreaHasWoodCuttingDesignations::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.try_emplace(faction, pair[1], deserializationMemo, faction);
	}
}
Json AreaHasWoodCuttingDesignations::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair;
		jsonPair[0] = pair.first;
		jsonPair[1] = pair.second.toJson();
		data.push_back(jsonPair);
	}
	return data;
}
void AreaHasWoodCuttingDesignations::addFaction(FactionId faction)
{
	assert(!m_data.contains(faction));
	m_data.try_emplace(faction, faction, m_area);
}
void AreaHasWoodCuttingDesignations::removeFaction(FactionId faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
// If blockFeatureType is null then woodCutting out fully rather then woodCuttingging out a feature.
void AreaHasWoodCuttingDesignations::designate(FactionId faction, BlockIndex block)
{
	if(!m_data.contains(faction))
		addFaction(faction);
	m_data.at(faction).designate(block);
}
void AreaHasWoodCuttingDesignations::undesignate(FactionId faction, BlockIndex block)
{
	assert(m_data.contains(faction));
	m_data.at(faction).undesignate(block);
}
void AreaHasWoodCuttingDesignations::remove(FactionId faction, BlockIndex block)
{
	assert(m_data.contains(faction));
	m_data.at(faction).remove(block);
}
void AreaHasWoodCuttingDesignations::clearAll(BlockIndex block)
{
	for(auto& pair : m_data)
		pair.second.removeIfExists(block);
}
void AreaHasWoodCuttingDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second.m_data)
			pair2.second.clearReservations();
}
bool AreaHasWoodCuttingDesignations::areThereAnyForFaction(FactionId faction) const
{
	if(!m_data.contains(faction))
		return false;
	return !m_data.at(faction).empty();
}
bool AreaHasWoodCuttingDesignations::contains(FactionId faction, BlockIndex block) const 
{ 
	if(!m_data.contains(faction))
		return false;
	return m_data.at(faction).m_data.contains(block); 
}
WoodCuttingProject& AreaHasWoodCuttingDesignations::getForFactionAndBlock(FactionId faction, BlockIndex block) 
{ 
	assert(m_data.contains(faction));
	assert(m_data.at(faction).m_data.contains(block)); 
	return m_data.at(faction).m_data.at(block); 
}
