#include "farmFields.h"
#include "area/area.h"
#include "datetime.h"
#include "deserializeDishonorCallbacks.h"
#include "designations.h"
#include "numericTypes/index.h"
#include "definitions/plantSpecies.h"
#include "simulation/simulation.h"
#include "plants.h"
#include "blocks/blocks.h"
#include "hasShapes.hpp"
#include "actors/actors.h"
#include <algorithm>
//Input
/*
void FarmFieldCreateInputAction::execute()
{
	auto& hasFarmFields = (*m_cuboid.begin()).m_area->m_hasFarmFields.at(m_faction);
	FarmField& farmField = hasFarmFields.create(m_cuboid);
	hasFarmFields.setSpecies(farmField, m_species);

}
void FarmFieldRemoveInputAction::execute()
{
	auto& hasFarmFields = (*m_cuboid.begin()).m_area->m_hasFarmFields.at(m_faction);
	SmallSet<BlockIndex> blocks = m_cuboid.toSet();
	hasFarmFields.undesignateBlocks(blocks);
}
void FarmFieldExpandInputAction::execute()
{
	auto& hasFarmFields = (*m_cuboid.begin()).m_area->m_hasFarmFields.at(m_faction);
	SmallSet<BlockIndex> blocks = m_cuboid.toSet();
	hasFarmFields.designateBlocks(m_farmField, blocks);
}
void FarmFieldUpdateInputAction::execute()
{
	auto& hasFarmFields = (**m_farmField.blocks.begin()).m_area->m_hasFarmFields.at(m_faction);
	hasFarmFields.setSpecies(m_farmField, m_species);
}
*/
bool HasFarmFieldsForFaction::hasGivePlantsFluidDesignations() const
{
	return !m_blocksWithPlantsNeedingFluid.empty();
}
PlantIndex HasFarmFieldsForFaction::getHighestPriorityPlantForGiveFluid(Area& area)
{
	assert(!m_blocksWithPlantsNeedingFluid.empty());
	Plants& plants = area.getPlants();
	Blocks& blocks = area.getBlocks();
	m_blocksWithPlantsNeedingFluid.eraseIf([&](const BlockIndex& block) { return !blocks.plant_exists(block); });
	if(!m_plantsNeedingFluidIsSorted)
	{
		m_blocksWithPlantsNeedingFluid.sort([&](const BlockIndex& a, const BlockIndex& b){
		       	return plants.getStepAtWhichPlantWillDieFromLackOfFluid(blocks.plant_get(a)) < plants.getStepAtWhichPlantWillDieFromLackOfFluid(blocks.plant_get(b));
		});
		m_plantsNeedingFluidIsSorted = true;
	}
	PlantIndex output = blocks.plant_get(m_blocksWithPlantsNeedingFluid.front());
	if(plants.getStepAtWhichPlantWillDieFromLackOfFluid(output) - area.m_simulation.m_step > Config::stepsTillDiePlantPriorityOveride)
		return PlantIndex::null();
	return output;
}
bool HasFarmFieldsForFaction::hasSowSeedsDesignations() const
{
	return !m_blocksNeedingSeedsSewn.empty();
}
bool HasFarmFieldsForFaction::hasHarvestDesignations() const
{
	return !m_blocksWithPlantsToHarvest.empty();
}
PlantSpeciesId HasFarmFieldsForFaction::getPlantSpeciesFor(const BlockIndex& block) const
{
	for(const FarmField& farmField : m_farmFields)
		if(farmField.blocks.contains(block))
			return farmField.plantSpecies;
	std::unreachable();
	return PlantSpecies::byName("");
}
void HasFarmFieldsForFaction::addGivePlantFluidDesignation(Area& area, const BlockIndex& location)
{
	assert(!m_blocksWithPlantsNeedingFluid.contains(location));
	m_plantsNeedingFluidIsSorted = false;
	m_blocksWithPlantsNeedingFluid.insert(location);
	area.getBlocks().designation_set(location, m_faction, BlockDesignation::GivePlantFluid);
}
void HasFarmFieldsForFaction::removeGivePlantFluidDesignation(Area& area, const BlockIndex& location)
{
	assert(m_blocksWithPlantsNeedingFluid.contains(location));
	m_blocksWithPlantsNeedingFluid.erase(location);
	area.getBlocks().designation_unset(location, m_faction, BlockDesignation::GivePlantFluid);
}
void HasFarmFieldsForFaction::addSowSeedsDesignation(Area& area, const BlockIndex& block)
{
	assert(!m_blocksNeedingSeedsSewn.contains(block));
	m_blocksNeedingSeedsSewn.insert(block);
	area.getBlocks().designation_set(block, m_faction, BlockDesignation::SowSeeds);
}
void HasFarmFieldsForFaction::removeSowSeedsDesignation(Area& area, const BlockIndex& block)
{
	assert(m_blocksNeedingSeedsSewn.contains(block));
	m_blocksNeedingSeedsSewn.erase(block);
	area.getBlocks().designation_unset(block, m_faction, BlockDesignation::SowSeeds);
}
void HasFarmFieldsForFaction::addHarvestDesignation(Area& area, const PlantIndex& plant)
{
	BlockIndex location = area.getPlants().getLocation(plant);
	assert(!m_blocksWithPlantsToHarvest.contains(location));
	m_blocksWithPlantsToHarvest.insert(location);
	area.getBlocks().designation_set(location, m_faction, BlockDesignation::Harvest);
}
void HasFarmFieldsForFaction::removeHarvestDesignation(Area& area, const PlantIndex& plant)
{
	BlockIndex location = area.getPlants().getLocation(plant);
	assert(m_blocksWithPlantsToHarvest.contains(location));
	m_blocksWithPlantsToHarvest.erase(location);
	area.getBlocks().designation_unset(location, m_faction, BlockDesignation::Harvest);
}
void HasFarmFieldsForFaction::setDayOfYear(Area& area, uint32_t dayOfYear)
{
	//TODO: Add sow designation when blocking item or plant is removed.
	for(FarmField& farmField : m_farmFields)
		if(farmField.plantSpecies.exists())
		{
			if(PlantSpecies::getDayOfYearForSowStart(farmField.plantSpecies) == dayOfYear)
			{
				Blocks& blocks = area.getBlocks();
				for(const BlockIndex& block : farmField.blocks)
					if(blocks.plant_canGrowHereAtSomePointToday(block, farmField.plantSpecies))
						addSowSeedsDesignation(area, block);
				farmField.timeToSow = true;
			}
			if(PlantSpecies::getDayOfYearForSowEnd(farmField.plantSpecies) == dayOfYear)
			{
				assert(farmField.timeToSow);
				farmField.timeToSow = false;
				for(const BlockIndex& block : farmField.blocks)
					if(m_blocksNeedingSeedsSewn.contains(block))
						removeSowSeedsDesignation(area, block);
			}
		}
}
FarmField& HasFarmFieldsForFaction::create(Area& area, SmallSet<BlockIndex>&& blocks)
{
	m_farmFields.emplace(std::move(blocks));
	auto& field = m_farmFields.back();
	for(const BlockIndex& block : field.blocks)
		area.getBlocks().farm_insert(block, m_faction, field);
	return field;
}
FarmField& HasFarmFieldsForFaction::create(Area& area, const SmallSet<BlockIndex>& blocks)
{
	SmallSet<BlockIndex> adapter;
	adapter.maybeInsertAll(blocks.begin(), blocks.end());
	return create(area, std::move(adapter));
}
FarmField& HasFarmFieldsForFaction::create(Area& area, const Cuboid& cuboid)
{
	auto set = cuboid.toSet(area.getBlocks());
	return create(area, std::move(set));
}
FarmField& HasFarmFieldsForFaction::create(Area& area, const CuboidSet& cuboidSet)
{
	auto set = cuboidSet.toBlockSet(area.getBlocks());
	return create(area, std::move(set));
}
void HasFarmFieldsForFaction::extend(Area& area, FarmField& farmField, SmallSet<BlockIndex>& blocks)
{
	farmField.blocks.maybeInsertAll(blocks.begin(), blocks.end());
	if(farmField.plantSpecies.exists())
		designateBlocks(area, farmField, blocks);
	for(const BlockIndex& block : blocks)
		area.getBlocks().farm_insert(block, m_faction, farmField);
}
void HasFarmFieldsForFaction::setSpecies(Area& area, FarmField& farmField, const PlantSpeciesId& plantSpecies)
{
	assert(farmField.plantSpecies.empty());
	farmField.plantSpecies = plantSpecies;
	int32_t day = DateTime(area.m_simulation.m_step).day;
	if(day >= PlantSpecies::getDayOfYearForSowStart(plantSpecies) && day <= PlantSpecies::getDayOfYearForSowEnd(plantSpecies))
	{
		farmField.timeToSow = true;
		designateBlocks(area, farmField, farmField.blocks);
	}
}
void HasFarmFieldsForFaction::designateBlocks(Area& area, FarmField& farmField, SmallSet<BlockIndex>& designated)
{
	// Designate for sow if the season is right.
	if(farmField.timeToSow)
		for(const BlockIndex& block : designated)
			addSowSeedsDesignation(area, block);
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	for(const BlockIndex& block : designated)
		if(blocks.plant_exists(block))
		{
			const PlantIndex& plant = blocks.plant_get(block);
			// Designate plants already existing for fluid if the species is right and they need fluid.
			if(plants.getSpecies(plant) == farmField.plantSpecies && plants.getVolumeFluidRequested(plant) != 0)
				addGivePlantFluidDesignation(area, block);
			if(plants.readyToHarvest(plant))
				addHarvestDesignation(area, plant);
		}
}
void HasFarmFieldsForFaction::clearSpecies(Area& area, FarmField& farmField)
{
	assert(farmField.plantSpecies.exists());
	farmField.plantSpecies.clear();
	undesignateBlocks(area, farmField.blocks);
}
void HasFarmFieldsForFaction::shrink(Area& area, FarmField& farmField, SmallSet<BlockIndex>& undesignated)
{
	undesignateBlocks(area, undesignated);
	Blocks& blocks = area.getBlocks();
	for(const BlockIndex& block : undesignated)
		blocks.farm_remove(block, m_faction);
	farmField.blocks.eraseIf([&](const BlockIndex& block){ return undesignated.contains(block); });
	if(farmField.blocks.empty())
		m_farmFields.erase(farmField);
}
void HasFarmFieldsForFaction::remove(Area& area, FarmField& farmField)
{
	shrink(area, farmField, farmField.blocks);
}
void HasFarmFieldsForFaction::undesignateBlocks(Area& area, SmallSet<BlockIndex>& undesignated)
{
	Blocks& blocks = area.getBlocks();
	for(const BlockIndex& block : undesignated)
	{
		if(blocks.plant_exists(block))
		{
			const PlantIndex& plant = blocks.plant_get(block);
			if(blocks.designation_has(block, m_faction, BlockDesignation::GivePlantFluid))
				area.m_hasFarmFields.getForFaction(m_faction).removeGivePlantFluidDesignation(area, block);
			if(blocks.designation_has(block, m_faction, BlockDesignation::Harvest))
				area.m_hasFarmFields.getForFaction(m_faction).removeHarvestDesignation(area, plant);
		}
		else if(blocks.designation_has(block, m_faction, BlockDesignation::SowSeeds))
				area.m_hasFarmFields.getForFaction(m_faction).removeSowSeedsDesignation(area, block);
	}
}
void HasFarmFieldsForFaction::setBlockData(Area& area)
{
	Blocks& blocks = area.getBlocks();
	for(FarmField& field : m_farmFields)
		for(const BlockIndex& block : field.blocks)
			blocks.farm_insert(block, m_faction, field);
}
HasFarmFieldsForFaction& AreaHasFarmFields::getForFaction(const FactionId& faction)
{
	if(!m_data.contains(faction))
		registerFaction(faction);
	return m_data[faction];
}
void AreaHasFarmFields::load(const Json& data)
{
	for(const Json& pair : data)
	{
		const FactionId& faction = pair[0].get<FactionId>();
		assert(!m_data.contains(faction));
		m_data.emplace(faction, pair[1]);
	}
	for(auto& [faction, data] : m_data)
		data.setBlockData(m_area);
}
Json AreaHasFarmFields::toJson() const
{
	Json data = Json::array();
	for(auto& [faction, hasFarmFieldsForFaction] : m_data)
		data.push_back({faction, hasFarmFieldsForFaction});
	return data;
}
void AreaHasFarmFields::registerFaction(const FactionId& faction)
{
	assert(!m_data.contains(faction));
	m_data.emplace(faction, faction);
}
void AreaHasFarmFields::unregisterFaction(const FactionId& faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
PlantIndex AreaHasFarmFields::getHighestPriorityPlantForGiveFluid(Area& area, const FactionId& faction)
{
	if(!m_data.contains(faction))
		return PlantIndex::null();
	return m_data[faction].getHighestPriorityPlantForGiveFluid(area);
}
void AreaHasFarmFields::removeAllSowSeedsDesignations(const BlockIndex& block)
{
	Blocks& blocks = m_area.getBlocks();
	for(auto& pair : m_data)
		if(blocks.designation_has(block, pair.first, BlockDesignation::SowSeeds))
			pair.second.removeSowSeedsDesignation(m_area, block);
}
void AreaHasFarmFields::setDayOfYear(uint32_t dayOfYear)
{
	for(auto& pair : m_data)
		pair.second.setDayOfYear(m_area, dayOfYear);
}
bool AreaHasFarmFields::hasGivePlantsFluidDesignations(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].hasGivePlantsFluidDesignations();
}
bool AreaHasFarmFields::hasSowSeedsDesignations(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].hasSowSeedsDesignations();
}
bool AreaHasFarmFields::hasHarvestDesignations(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].hasHarvestDesignations();
}
PlantSpeciesId AreaHasFarmFields::getPlantSpeciesFor(const FactionId& faction, const BlockIndex& location) const
{
	assert(m_data.contains(faction));
	return m_data[faction].getPlantSpeciesFor(location);
}
