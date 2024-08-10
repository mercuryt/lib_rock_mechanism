#include "farmFields.h"
#include "area.h"
#include "datetime.h"
#include "deserializeDishonorCallbacks.h"
#include "designations.h"
#include "index.h"
#include "plantSpecies.h"
#include "simulation.h"
#include "plants.h"
#include "blocks/blocks.h"
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
	BlockIndices blocks = m_cuboid.toSet();
	hasFarmFields.undesignateBlocks(blocks);
}
void FarmFieldExpandInputAction::execute()
{
	auto& hasFarmFields = (*m_cuboid.begin()).m_area->m_hasFarmFields.at(m_faction);
	BlockIndices blocks = m_cuboid.toSet();
	hasFarmFields.designateBlocks(m_farmField, blocks);
}
void FarmFieldUpdateInputAction::execute()
{
	auto& hasFarmFields = (**m_farmField.blocks.begin()).m_area->m_hasFarmFields.at(m_faction);
	hasFarmFields.setSpecies(m_farmField, m_species);
}
*/
FarmField::FarmField(const Json& data, FactionId faction, Area& a) :
	area(a)
{
	for(const Json& blockReference : data["blocks"])
	{
		BlockIndex block = blockReference.get<BlockIndex>();
		blocks.add(block);
		area.getBlocks().farm_insert(block, faction, *this);
	}
	plantSpecies = data["plantSpecies"].get<PlantSpeciesId>();
	timeToSow = data["timeToSow"];
}
Json FarmField::toJson() const 
{
	Json data{{"blocks", Json::array()}, {"plantSpecies", plantSpecies}, {"timeToSow", timeToSow}};
	for(BlockIndex block : blocks)
		data["blocks"].push_back(block);
	return data;
}
HasFarmFieldsForFaction::HasFarmFieldsForFaction(const Json& data, DeserializationMemo& , Area& a, FactionId f) : m_area(a), m_faction(f), m_plantsNeedingFluidIsSorted(data["plantsNeedingFluidIsSorted"].get<bool>())
{
	for(const Json& farmFieldData : data["farmFields"])
		m_farmFields.emplace_back(farmFieldData, m_faction, m_area);
	for(const Json& plantReference : data["plantsNeedingFluid"])
		m_blocksWithPlantsNeedingFluid.add(plantReference.get<BlockIndex>());
	for(const Json& plantReference : data["plantsToHarvest"])
		m_blocksWithPlantsToHarvest.add(plantReference.get<BlockIndex>());
	for(const Json& blockReference : data["blocksNeedingSeedsSewn"])
		m_blocksNeedingSeedsSewn.add(blockReference.get<BlockIndex>());
}
Json HasFarmFieldsForFaction::toJson() const
{
	Json data{{"farmFields", Json::array()}, {"plantsNeedingFluid", Json::array()}, {"plantsToHarvest", Json::array()}, {"blocksNeedingSeedsSewn", Json::array()}, {"plantsNeedingFluidIsSorted", m_plantsNeedingFluidIsSorted}};
	for(const FarmField& farmField : m_farmFields)
		data["farmFields"].push_back(farmField.toJson());
	for(BlockIndex block : m_blocksWithPlantsNeedingFluid)
		data["plantsNeedingFluid"].push_back(block);
	for(BlockIndex block : m_blocksWithPlantsToHarvest)
		data["plantsToHarvest"].push_back(block);
	for(BlockIndex block : m_blocksNeedingSeedsSewn)
		data["blocksNeedingSeedsSewn"].push_back(block);
	return data;
}
bool HasFarmFieldsForFaction::hasGivePlantsFluidDesignations() const
{
	return !m_blocksWithPlantsNeedingFluid.empty();
}
PlantIndex HasFarmFieldsForFaction::getHighestPriorityPlantForGiveFluid()
{
	assert(!m_blocksWithPlantsNeedingFluid.empty());
	Plants& plants = m_area.getPlants();
	Blocks& blocks = m_area.getBlocks();
	m_blocksWithPlantsNeedingFluid.remove_if([&](BlockIndex block) { return !blocks.plant_exists(block); });
	if(!m_plantsNeedingFluidIsSorted)
	{
		m_blocksWithPlantsNeedingFluid.sort([&](const BlockIndex a, const BlockIndex b){
		       	return plants.getStepAtWhichPlantWillDieFromLackOfFluid(blocks.plant_get(a)) < plants.getStepAtWhichPlantWillDieFromLackOfFluid(blocks.plant_get(b));
		});
		m_plantsNeedingFluidIsSorted = true;
	}
	PlantIndex output = blocks.plant_get(m_blocksWithPlantsNeedingFluid.front());
	if(plants.getStepAtWhichPlantWillDieFromLackOfFluid(output) - m_area.m_simulation.m_step > Config::stepsTillDiePlantPriorityOveride)
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
PlantSpeciesId HasFarmFieldsForFaction::getPlantSpeciesFor(BlockIndex block) const
{
	for(const FarmField& farmField : m_farmFields)
		if(farmField.blocks.contains(block))
			return farmField.plantSpecies;
	assert(false);
	return PlantSpecies::byName("poplar tree");
}
void HasFarmFieldsForFaction::addGivePlantFluidDesignation(PlantIndex plant)
{
	Plants& plants = m_area.getPlants();
	BlockIndex location = plants.getLocation(plant);
	assert(!m_blocksWithPlantsNeedingFluid.contains(location));
	m_plantsNeedingFluidIsSorted = false;
	m_blocksWithPlantsNeedingFluid.add(plants.getLocation(plant));
	m_area.getBlocks().designation_set(plants.getLocation(plant), m_faction, BlockDesignation::GivePlantFluid);
}
void HasFarmFieldsForFaction::removeGivePlantFluidDesignation(PlantIndex plant)
{
	BlockIndex location = m_area.getPlants().getLocation(plant);
	assert(m_blocksWithPlantsNeedingFluid.contains(location));
	m_blocksWithPlantsNeedingFluid.remove(location);
	m_area.getBlocks().designation_unset(m_area.getPlants().getLocation(plant), m_faction, BlockDesignation::GivePlantFluid);
}
void HasFarmFieldsForFaction::addSowSeedsDesignation(BlockIndex block)
{
	assert(!m_blocksNeedingSeedsSewn.contains(block));
	m_blocksNeedingSeedsSewn.add(block);
	m_area.getBlocks().designation_set(block, m_faction, BlockDesignation::SowSeeds);
}
void HasFarmFieldsForFaction::removeSowSeedsDesignation(BlockIndex block)
{
	assert(m_blocksNeedingSeedsSewn.contains(block));
	m_blocksNeedingSeedsSewn.remove(block);
	m_area.getBlocks().designation_unset(block, m_faction, BlockDesignation::SowSeeds);
}
void HasFarmFieldsForFaction::addHarvestDesignation(PlantIndex plant)
{
	BlockIndex location = m_area.getPlants().getLocation(plant);
	assert(!m_blocksWithPlantsToHarvest.contains(location));
	m_blocksWithPlantsToHarvest.add(location);
	m_area.getBlocks().designation_set(location, m_faction, BlockDesignation::Harvest);
}
void HasFarmFieldsForFaction::removeHarvestDesignation(PlantIndex plant)
{
	BlockIndex location = m_area.getPlants().getLocation(plant);
	assert(m_blocksWithPlantsToHarvest.contains(location));
	m_blocksWithPlantsToHarvest.remove(location);
	m_area.getBlocks().designation_unset(location, m_faction, BlockDesignation::Harvest);
}
void HasFarmFieldsForFaction::setDayOfYear(uint32_t dayOfYear)
{
	//TODO: Add sow designation when blocking item or plant is removed.
	for(FarmField& farmField : m_farmFields)
		if(farmField.plantSpecies.exists())
		{
			if(PlantSpecies::getDayOfYearForSowStart(farmField.plantSpecies) == dayOfYear)
			{
				Blocks& blocks = m_area.getBlocks();
				for(BlockIndex block : farmField.blocks)
					if(blocks.plant_canGrowHereAtSomePointToday(block, farmField.plantSpecies))
						addSowSeedsDesignation(block);
				farmField.timeToSow = true;
			}
			if(PlantSpecies::getDayOfYearForSowEnd(farmField.plantSpecies) == dayOfYear)
			{
				assert(farmField.timeToSow);
				farmField.timeToSow = false;
				for(BlockIndex block : farmField.blocks)
					if(m_blocksNeedingSeedsSewn.contains(block))
						removeSowSeedsDesignation(block);
			}
		}
}
FarmField& HasFarmFieldsForFaction::create(BlockIndices blocks)
{
	FarmField& field = m_farmFields.emplace_back(m_area, blocks);
	for(BlockIndex block : blocks)
		m_area.getBlocks().farm_insert(block, m_faction, field);
	return field;
}
FarmField& HasFarmFieldsForFaction::create(Cuboid cuboid)
{
	auto set = cuboid.toSet();
	return create(set);
}
void HasFarmFieldsForFaction::extend(FarmField& farmField, BlockIndices blocks)
{
	farmField.blocks.merge(blocks);
	if(farmField.plantSpecies.exists())
		designateBlocks(farmField, blocks);
	for(BlockIndex block : blocks)
		m_area.getBlocks().farm_insert(block, m_faction, farmField);
}
void HasFarmFieldsForFaction::setSpecies(FarmField& farmField, PlantSpeciesId plantSpecies)
{
	assert(farmField.plantSpecies.empty());
	farmField.plantSpecies = plantSpecies;
	int32_t day = DateTime(m_area.m_simulation.m_step).day;
	if(day >= PlantSpecies::getDayOfYearForSowStart(plantSpecies) && day <= PlantSpecies::getDayOfYearForSowEnd(plantSpecies))
	{
		farmField.timeToSow = true;
		designateBlocks(farmField, farmField.blocks);
	}
}
void HasFarmFieldsForFaction::designateBlocks(FarmField& farmField, BlockIndices designated)
{
	// Designate for sow if the season is right.
	if(farmField.timeToSow)
		for(BlockIndex block : designated)
			addSowSeedsDesignation(block);
	Blocks& blocks = m_area.getBlocks();
	Plants& plants = m_area.getPlants();
	for(BlockIndex block : designated)
		if(blocks.plant_exists(block))
		{
			PlantIndex plant = blocks.plant_get(block);
			// Designate plants already existing for fluid if the species is right and they need fluid.
			if(plants.getSpecies(plant) == farmField.plantSpecies && plants.getVolumeFluidRequested(plant) != 0)
				addGivePlantFluidDesignation(plant);
			if(plants.readyToHarvest(plant))
				addHarvestDesignation(plant);
		}
}
void HasFarmFieldsForFaction::clearSpecies(FarmField& farmField)
{
	assert(farmField.plantSpecies.exists());
	farmField.plantSpecies.clear();
	undesignateBlocks(farmField.blocks);
}
void HasFarmFieldsForFaction::shrink(FarmField& farmField, BlockIndices undesignated)
{
	undesignateBlocks(undesignated);
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : undesignated)
		blocks.farm_remove(block, m_faction);
	farmField.blocks.erase_if([&](BlockIndex block){ return undesignated.contains(block); });
	if(farmField.blocks.empty())
		m_farmFields.remove_if([&](FarmField& other){ return &other == &farmField; });
}
void HasFarmFieldsForFaction::remove(FarmField& farmField)
{
	shrink(farmField, farmField.blocks);
}
void HasFarmFieldsForFaction::undesignateBlocks(BlockIndices undesignated)
{
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : undesignated)
	{
		if(blocks.plant_exists(block))
		{
			PlantIndex plant = blocks.plant_get(block);
			if(blocks.designation_has(block, m_faction, BlockDesignation::GivePlantFluid))
				m_area.m_hasFarmFields.getForFaction(m_faction).removeGivePlantFluidDesignation(plant);
			if(blocks.designation_has(block, m_faction, BlockDesignation::Harvest))
				m_area.m_hasFarmFields.getForFaction(m_faction).removeHarvestDesignation(plant);
		}
		else if(blocks.designation_has(block, m_faction, BlockDesignation::SowSeeds))
				m_area.m_hasFarmFields.getForFaction(m_faction).removeSowSeedsDesignation(block);
	}
}
HasFarmFieldsForFaction& AreaHasFarmFields::getForFaction(FactionId faction)
{
	if(!m_data.contains(faction))
		registerFaction(faction);
	return m_data.at(faction);
}
void AreaHasFarmFields::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		assert(!m_data.contains(faction));
		m_data.try_emplace(faction, pair[1], deserializationMemo, m_area, faction);
	}
}
Json AreaHasFarmFields::toJson() const
{
	Json data = Json::array();
	for(auto& [faction, hasFarmFieldsForFaction] : m_data)
		data.push_back({faction, hasFarmFieldsForFaction.toJson()});
	return data;
}
void AreaHasFarmFields::registerFaction(FactionId faction)
{
	assert(!m_data.contains(faction));
	m_data.try_emplace(faction, m_area, faction);
}
void AreaHasFarmFields::unregisterFaction(FactionId faction)
{
	assert(m_data.contains(faction));
	m_data.erase(faction);
}
PlantIndex AreaHasFarmFields::getHighestPriorityPlantForGiveFluid(FactionId faction)
{
	if(!m_data.contains(faction))
		return PlantIndex::null();
	return m_data.at(faction).getHighestPriorityPlantForGiveFluid();
}
void AreaHasFarmFields::removeAllSowSeedsDesignations(BlockIndex block)
{
	Blocks& blocks = m_area.getBlocks();
	for(auto& pair : m_data)
		if(blocks.designation_has(block, pair.first, BlockDesignation::SowSeeds))
			pair.second.removeSowSeedsDesignation(block);
}
void AreaHasFarmFields::setDayOfYear(uint32_t dayOfYear)
{
	for(auto& pair : m_data)
		pair.second.setDayOfYear(dayOfYear);
}
bool AreaHasFarmFields::hasGivePlantsFluidDesignations(FactionId faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data.at(faction).hasGivePlantsFluidDesignations();
}
bool AreaHasFarmFields::hasSowSeedsDesignations(FactionId faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data.at(faction).hasSowSeedsDesignations();
}
bool AreaHasFarmFields::hasHarvestDesignations(FactionId faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data.at(faction).hasHarvestDesignations();
}
PlantSpeciesId AreaHasFarmFields::getPlantSpeciesFor(FactionId faction, const  BlockIndex location) const
{
	assert(m_data.contains(faction));
	return m_data.at(faction).getPlantSpeciesFor(location);
}
