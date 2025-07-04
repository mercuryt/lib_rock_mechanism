#include "woodcutting.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "plants.h"
#include "area/area.h"
#include "blockFeature.h"
#include "deserializationMemo.h"
#include "random.h"
#include "reservable.h"
#include "path/terrainFacade.h"
#include "numericTypes/types.h"
#include "util.h"
#include "simulation/simulation.h"
#include "definitions/itemType.h"
#include "objectives/woodcutting.h"
#include "projects/woodcutting.h"
#include <memory>
#include <sys/types.h>
WoodCuttingLocationDishonorCallback::WoodCuttingLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) :
	m_faction(data["faction"].get<FactionId>()),
	m_area(deserializationMemo.area(data["area"])),
	m_location(data["location"].get<BlockIndex>()) { }
Json WoodCuttingLocationDishonorCallback::toJson() const { return Json({{"type", "WoodCuttingLocationDishonorCallback"}, {"faction", m_faction}, {"location", m_location}}); }
void WoodCuttingLocationDishonorCallback::execute([[maybe_unused]] const Quantity& oldCount, [[maybe_unused]] const Quantity& newCount)
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
		m_data.emplace(block, pair[1], deserializationMemo, m_area);
	}
}
Json HasWoodCuttingDesignationsForFaction::toJson() const
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second->toJson()};
		data.push_back(jsonPair);
	}
	return data;
}
void HasWoodCuttingDesignationsForFaction::designate(const BlockIndex& block)
{
	Blocks& blocks = m_area.getBlocks();
	[[maybe_unused]] Plants& plants = m_area.getPlants();
	assert(!m_data.contains(block));
	assert(blocks.plant_exists(block));
	assert(PlantSpecies::getIsTree(plants.getSpecies(blocks.plant_get(block))));
	assert(plants.getPercentGrown(blocks.plant_get(block)) >= Config::minimumPercentGrowthForWoodCutting);
	blocks.designation_set(block, m_faction, BlockDesignation::WoodCutting);
	// To be called when block is no longer a suitable location, for example if it got crushed by a collapse.
	std::unique_ptr<DishonorCallback> locationDishonorCallback = std::make_unique<WoodCuttingLocationDishonorCallback>(m_faction, m_area, block);
	m_data.emplace(block, m_faction, m_area, block, std::move(locationDishonorCallback));
}
void HasWoodCuttingDesignationsForFaction::undesignate(const BlockIndex& block)
{
	assert(m_data.contains(block));
	WoodCuttingProject& project = m_data[block];
	project.cancel();
}
void HasWoodCuttingDesignationsForFaction::remove(const BlockIndex& block)
{
	assert(m_data.contains(block));
	m_area.getBlocks().designation_unset(block, m_faction, BlockDesignation::WoodCutting);
	m_data.erase(block);
}
void HasWoodCuttingDesignationsForFaction::removeIfExists(const BlockIndex& block)
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
		m_data.emplace(faction, pair[1], deserializationMemo, faction);
	}
}
Json AreaHasWoodCuttingDesignations::toJson() const
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
void AreaHasWoodCuttingDesignations::addFaction(FactionId faction)
{
	assert(!m_data.contains(faction));
	m_data.emplace(faction, faction, m_area);
}
void AreaHasWoodCuttingDesignations::removeFaction(FactionId faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
// If blockFeatureType is null then woodCutting out fully rather then woodCuttingging out a feature.
void AreaHasWoodCuttingDesignations::designate(FactionId faction, const BlockIndex& block)
{
	if(!m_data.contains(faction))
		addFaction(faction);
	m_data[faction].designate(block);
}
void AreaHasWoodCuttingDesignations::undesignate(FactionId faction, const BlockIndex& block)
{
	assert(m_data.contains(faction));
	m_data[faction].undesignate(block);
}
void AreaHasWoodCuttingDesignations::remove(FactionId faction, const BlockIndex& block)
{
	assert(m_data.contains(faction));
	m_data[faction].remove(block);
}
void AreaHasWoodCuttingDesignations::clearAll(const BlockIndex& block)
{
	for(auto& pair : m_data)
		pair.second->removeIfExists(block);
}
void AreaHasWoodCuttingDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second->m_data)
			pair2.second->clearReservations();
}
bool AreaHasWoodCuttingDesignations::areThereAnyForFaction(FactionId faction) const
{
	if(!m_data.contains(faction))
		return false;
	return !m_data[faction].empty();
}
bool AreaHasWoodCuttingDesignations::contains(FactionId faction, const BlockIndex& block) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].m_data.contains(block);
}
WoodCuttingProject& AreaHasWoodCuttingDesignations::getForFactionAndBlock(FactionId faction, const BlockIndex& block)
{
	assert(m_data.contains(faction));
	assert(m_data[faction].m_data.contains(block));
	return m_data[faction].m_data[block];
}
