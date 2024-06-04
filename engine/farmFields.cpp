#include "farmFields.h"
#include "area.h"
#include "datetime.h"
#include "deserializeDishonorCallbacks.h"
#include "designations.h"
#include "simulation.h"
#include "plant.h"
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
	std::unordered_set<BlockIndex> blocks = m_cuboid.toSet();
	hasFarmFields.undesignateBlocks(blocks);
}
void FarmFieldExpandInputAction::execute()
{
	auto& hasFarmFields = (*m_cuboid.begin()).m_area->m_hasFarmFields.at(m_faction);
	std::unordered_set<BlockIndex> blocks = m_cuboid.toSet();
	hasFarmFields.designateBlocks(m_farmField, blocks);
}
void FarmFieldUpdateInputAction::execute()
{
	auto& hasFarmFields = (**m_farmField.blocks.begin()).m_area->m_hasFarmFields.at(m_faction);
	hasFarmFields.setSpecies(m_farmField, m_species);
}
*/
FarmField::FarmField(const Json& data, DeserializationMemo& deserializationMemo, Faction& faction) :
	area(deserializationMemo.area(data["area"]))
{
	for(const Json& blockReference : data["blocks"])
	{
		BlockIndex block = blockReference.get<BlockIndex>();
		blocks.insert(block);
		area.getBlocks().farm_insert(block, faction, *this);
	}
	plantSpecies = data["plantSpecies"].get<const PlantSpecies*>();
	timeToSow = data["timeToSow"];
}
Json FarmField::toJson() const 
{
	Json data{{"blocks", Json::array()}, {"plantSpecies", plantSpecies}, {"timeToSow", timeToSow}};
	for(BlockIndex block : blocks)
		data["blocks"].push_back(block);
	return data;
}
HasFarmFieldsForFaction::HasFarmFieldsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& a, Faction& f) : m_area(a), m_faction(f), m_plantsNeedingFluidIsSorted(data["plantsNeedingFluidIsSorted"].get<bool>())
{
	for(const Json& farmFieldData : data["farmFields"])
		m_farmFields.emplace_back(farmFieldData, deserializationMemo, m_faction);
	for(const Json& plantReference : data["plantsNeedingFluid"])
		m_plantsNeedingFluid.push_back(&deserializationMemo.plantReference(plantReference));
	for(const Json& plantReference : data["plantsToHarvest"])
		m_plantsToHarvest.insert(&deserializationMemo.plantReference(plantReference));
	for(const Json& blockReference : data["blocksNeedingSeedsSewn"])
		m_blocksNeedingSeedsSewn.insert(blockReference.get<BlockIndex>());
}
Json HasFarmFieldsForFaction::toJson() const
{
	Json data{{"farmFields", Json::array()}, {"plantsNeedingFluid", Json::array()}, {"plantsToHarvest", Json::array()}, {"blocksNeedingSeedsSewn", Json::array()}, {"plantsNeedingFluidIsSorted", m_plantsNeedingFluidIsSorted}};
	for(const FarmField& farmField : m_farmFields)
		data["farmFields"].push_back(farmField.toJson());
	for(const Plant* plant : m_plantsNeedingFluid)
		data["plantsNeedingFluid"].push_back(plant);
	for(const Plant* plant : m_plantsToHarvest)
		data["plantsToHarvest"].push_back(plant);
	for(BlockIndex block : m_blocksNeedingSeedsSewn)
		data["blocksNeedingSeedsSewn"].push_back(block);
	return data;
}
bool HasFarmFieldsForFaction::hasGivePlantsFluidDesignations() const
{
	return !m_plantsNeedingFluid.empty();
}
Plant* HasFarmFieldsForFaction::getHighestPriorityPlantForGiveFluid()
{
	assert(!m_plantsNeedingFluid.empty());
	if(!m_plantsNeedingFluidIsSorted)
	{
		std::ranges::sort(m_plantsNeedingFluid, [](Plant* a, Plant* b){ return a->getStepAtWhichPlantWillDieFromLackOfFluid() < b->getStepAtWhichPlantWillDieFromLackOfFluid(); });
		m_plantsNeedingFluidIsSorted = true;
	}
	Plant* output = m_plantsNeedingFluid.front();
	if(output->getStepAtWhichPlantWillDieFromLackOfFluid() - m_area.m_simulation.m_step > Config::stepsTillDiePlantPriorityOveride)
		return nullptr;
	return output;
}
bool HasFarmFieldsForFaction::hasSowSeedsDesignations() const
{
	return !m_blocksNeedingSeedsSewn.empty();
}
bool HasFarmFieldsForFaction::hasHarvestDesignations() const
{
	return !m_plantsToHarvest.empty();
}
const PlantSpecies& HasFarmFieldsForFaction::getPlantSpeciesFor(BlockIndex block) const
{
	for(const FarmField& farmField : m_farmFields)
		if(farmField.blocks.contains(block))
			return *farmField.plantSpecies;
	assert(false);
	return PlantSpecies::byName("poplar tree");
}
void HasFarmFieldsForFaction::addGivePlantFluidDesignation(Plant& plant)
{
	assert(std::ranges::find(m_plantsNeedingFluid, &plant) == m_plantsNeedingFluid.end());
	m_plantsNeedingFluidIsSorted = false;
	m_plantsNeedingFluid.push_back(&plant);
	m_area.getBlocks().designation_set(plant.m_location, m_faction, BlockDesignation::GivePlantFluid);
}
void HasFarmFieldsForFaction::removeGivePlantFluidDesignation(Plant& plant)
{
	assert(std::ranges::find(m_plantsNeedingFluid, &plant) != m_plantsNeedingFluid.end());
	std::erase(m_plantsNeedingFluid, &plant);
	m_area.getBlocks().designation_unset(plant.m_location, m_faction, BlockDesignation::GivePlantFluid);
}
void HasFarmFieldsForFaction::addSowSeedsDesignation(BlockIndex block)
{
	assert(!m_blocksNeedingSeedsSewn.contains(block));
	m_blocksNeedingSeedsSewn.insert(block);
	m_area.getBlocks().designation_set(block, m_faction, BlockDesignation::SowSeeds);
}
void HasFarmFieldsForFaction::removeSowSeedsDesignation(BlockIndex block)
{
	assert(m_blocksNeedingSeedsSewn.contains(block));
	m_blocksNeedingSeedsSewn.erase(block);
	m_area.getBlocks().designation_unset(block, m_faction, BlockDesignation::SowSeeds);
}
void HasFarmFieldsForFaction::addHarvestDesignation(Plant& plant)
{
	assert(!m_plantsToHarvest.contains(&plant));
	m_plantsToHarvest.insert(&plant);
	m_area.getBlocks().designation_set(plant.m_location, m_faction, BlockDesignation::Harvest);
}
void HasFarmFieldsForFaction::removeHarvestDesignation(Plant& plant)
{
	assert(m_plantsToHarvest.contains(&plant));
	m_plantsToHarvest.erase(&plant);
	m_area.getBlocks().designation_unset(plant.m_location, m_faction, BlockDesignation::Harvest);
}
void HasFarmFieldsForFaction::setDayOfYear(uint32_t dayOfYear)
{
	//TODO: Add sow designation when blocking item or plant is removed.
	for(FarmField& farmField : m_farmFields)
		if(farmField.plantSpecies != nullptr)
		{
			if(farmField.plantSpecies->dayOfYearForSowStart == dayOfYear)
			{
				Blocks& blocks = m_area.getBlocks();
				for(BlockIndex block : farmField.blocks)
					if(blocks.plant_canGrowHereAtSomePointToday(block, *farmField.plantSpecies))
						addSowSeedsDesignation(block);
				farmField.timeToSow = true;
			}
			if(farmField.plantSpecies->dayOfYearForSowEnd == dayOfYear)
			{
				assert(farmField.timeToSow);
				farmField.timeToSow = false;
				for(BlockIndex block : farmField.blocks)
					if(m_blocksNeedingSeedsSewn.contains(block))
						removeSowSeedsDesignation(block);
			}
		}
}
FarmField& HasFarmFieldsForFaction::create(std::unordered_set<BlockIndex>& blocks)
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
void HasFarmFieldsForFaction::extend(FarmField& farmField, std::unordered_set<BlockIndex>& blocks)
{
	farmField.blocks.insert(blocks.begin(), blocks.end());
	if(farmField.plantSpecies != nullptr)
		designateBlocks(farmField, blocks);
	for(BlockIndex block : blocks)
		m_area.getBlocks().farm_insert(block, m_faction, farmField);
}
void HasFarmFieldsForFaction::setSpecies(FarmField& farmField, const PlantSpecies& plantSpecies)
{
	assert(farmField.plantSpecies == nullptr);
	farmField.plantSpecies = &plantSpecies;
	int32_t day = DateTime(m_area.m_simulation.m_step).day;
	if(day >= plantSpecies.dayOfYearForSowStart && day <= plantSpecies.dayOfYearForSowEnd)
	{
		farmField.timeToSow = true;
		designateBlocks(farmField, farmField.blocks);
	}
}
void HasFarmFieldsForFaction::designateBlocks(FarmField& farmField, std::unordered_set<BlockIndex>& designated)
{
	// Designate for sow if the season is right.
	if(farmField.timeToSow)
		for(BlockIndex block : designated)
			addSowSeedsDesignation(block);
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : designated)
		if(blocks.plant_exists(block))
		{
			Plant& plant = blocks.plant_get(block);
			// Designate plants already existing for fluid if the species is right and they need fluid.
			if(&plant.m_plantSpecies == farmField.plantSpecies && plant.m_volumeFluidRequested)
				addGivePlantFluidDesignation(plant);
			if(plant.m_quantityToHarvest)
				addHarvestDesignation(plant);
		}
}
void HasFarmFieldsForFaction::clearSpecies(FarmField& farmField)
{
	assert(farmField.plantSpecies != nullptr);
	farmField.plantSpecies = nullptr;
	undesignateBlocks(farmField.blocks);
}
void HasFarmFieldsForFaction::shrink(FarmField& farmField, std::unordered_set<BlockIndex>& undesignated)
{
	undesignateBlocks(undesignated);
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : undesignated)
		blocks.farm_remove(block, m_faction);
	std::erase_if(farmField.blocks, [&](BlockIndex block){ return undesignated.contains(block); });
	if(farmField.blocks.empty())
		m_farmFields.remove_if([&](FarmField& other){ return &other == &farmField; });
}
void HasFarmFieldsForFaction::remove(FarmField& farmField)
{
	shrink(farmField, farmField.blocks);
}
void HasFarmFieldsForFaction::undesignateBlocks(std::unordered_set<BlockIndex>& undesignated)
{
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : undesignated)
	{
		if(blocks.plant_exists(block))
		{
			Plant& plant = blocks.plant_get(block);
			if(blocks.designation_has(block, m_faction, BlockDesignation::GivePlantFluid))
				m_area.m_hasFarmFields.at(m_faction).removeGivePlantFluidDesignation(plant);
			if(blocks.designation_has(block, m_faction, BlockDesignation::Harvest))
				m_area.m_hasFarmFields.at(m_faction).removeHarvestDesignation(plant);
		}
		else if(blocks.designation_has(block, m_faction, BlockDesignation::SowSeeds))
				m_area.m_hasFarmFields.at(m_faction).removeSowSeedsDesignation(block);
	}
}
HasFarmFieldsForFaction& AreaHasFarmFields::at(Faction& faction)
{
	if(!m_data.contains(&faction))
		registerFaction(faction);
	return m_data.at(&faction);
}
void AreaHasFarmFields::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		Faction& faction = deserializationMemo.faction(pair[0]);
		assert(!m_data.contains(&faction));
		m_data.try_emplace(&faction, pair[1], deserializationMemo, m_area, faction);
	}
}
Json AreaHasFarmFields::toJson() const
{
	Json data = Json::array();
	for(auto& [faction, hasFarmFieldsForFaction] : m_data)
		data.push_back({faction, hasFarmFieldsForFaction.toJson()});
	return data;
}
void AreaHasFarmFields::registerFaction(Faction& faction)
{
	assert(!m_data.contains(&faction));
	m_data.try_emplace(&faction, m_area, faction);
}
void AreaHasFarmFields::unregisterFaction(Faction& faction)
{
	assert(m_data.contains(&faction));
	m_data.erase(&faction);
}
Plant* AreaHasFarmFields::getHighestPriorityPlantForGiveFluid(Faction& faction)
{
	if(!m_data.contains(&faction))
		return nullptr;
	return m_data.at(&faction).getHighestPriorityPlantForGiveFluid();
}
void AreaHasFarmFields::removeAllSowSeedsDesignations(BlockIndex block)
{
	Blocks& blocks = m_area.getBlocks();
	for(auto& pair : m_data)
		if(blocks.designation_has(block, *pair.first, BlockDesignation::SowSeeds))
			pair.second.removeSowSeedsDesignation(block);
}
void AreaHasFarmFields::setDayOfYear(uint32_t dayOfYear)
{
	for(auto& pair : m_data)
		pair.second.setDayOfYear(dayOfYear);
}
bool AreaHasFarmFields::hasGivePlantsFluidDesignations(Faction& faction) const
{
	if(!m_data.contains(&faction))
		return false;
	return m_data.at(&faction).hasGivePlantsFluidDesignations();
}
bool AreaHasFarmFields::hasSowSeedsDesignations(Faction& faction) const
{
	if(!m_data.contains(&faction))
		return false;
	return m_data.at(&faction).hasSowSeedsDesignations();
}
bool AreaHasFarmFields::hasHarvestDesignations(Faction& faction) const
{
	if(!m_data.contains(&faction))
		return false;
	return m_data.at(&faction).hasHarvestDesignations();
}
const PlantSpecies& AreaHasFarmFields::getPlantSpeciesFor(Faction& faction, const  BlockIndex location) const
{
	assert(m_data.contains(&faction));
	return m_data.at(&faction).getPlantSpeciesFor(location);
}
