#include "farmFields.h"
#include "block.h"
#include "area.h"
#include "designations.h"
#include "simulation.h"
#include "plant.h"
#include <algorithm>
//Input
void FarmFieldCreateInputAction::execute()
{
	auto& hasFarmFields = (*m_cuboid.begin()).m_area->m_hasFarmFields.at(m_faction);
	FarmField& farmField = hasFarmFields.create(m_cuboid);
	hasFarmFields.setSpecies(farmField, m_species);

}
void FarmFieldRemoveInputAction::execute()
{
	auto& hasFarmFields = (*m_cuboid.begin()).m_area->m_hasFarmFields.at(m_faction);
	std::unordered_set<Block*> blocks = m_cuboid.toSet();
	hasFarmFields.undesignateBlocks(blocks);
}
void FarmFieldExpandInputAction::execute()
{
	auto& hasFarmFields = (*m_cuboid.begin()).m_area->m_hasFarmFields.at(m_faction);
	std::unordered_set<Block*> blocks = m_cuboid.toSet();
	hasFarmFields.designateBlocks(m_farmField, blocks);
}
void FarmFieldUpdateInputAction::execute()
{
	auto& hasFarmFields = (**m_farmField.blocks.begin()).m_area->m_hasFarmFields.at(m_faction);
	hasFarmFields.setSpecies(m_farmField, m_species);
}
FarmField::FarmField(const Json& data, DeserializationMemo& deserializationMemo, const Faction& faction)
{
	for(const Json& blockReference : data["blocks"])
	{
		Block& block = deserializationMemo.blockReference(blockReference);
		blocks.insert(&block);
		block.m_isPartOfFarmField.insert(faction, *this);
	}
	plantSpecies = data["plantSpecies"].get<const PlantSpecies*>();
	timeToSow = data["timeToSow"];
}
Json FarmField::toJson() const 
{
	Json data{{"blocks", Json::array()}, {"plantSpecies", plantSpecies}, {"timeToSow", timeToSow}};
	for(Block* block : blocks)
		data["blocks"].push_back(block);
	return data;
}
void IsPartOfFarmField::insert(const Faction& faction, FarmField& farmField)
{
	assert(!m_farmFields.contains(&faction));
	m_farmFields[&faction] = &farmField;
}
void IsPartOfFarmField::remove(const Faction& faction)
{
	assert(m_farmFields.contains(&faction));
	m_farmFields.erase(&faction);
	m_block.m_hasDesignations.removeIfExists(faction, BlockDesignation::SowSeeds);
	m_block.m_hasDesignations.removeIfExists(faction, BlockDesignation::GivePlantFluid);
	m_block.m_hasDesignations.removeIfExists(faction, BlockDesignation::Harvest);
}
void IsPartOfFarmField::designateForHarvestIfPartOfFarmField(Plant& plant)
{
	for(auto& [faction, farmField] : m_farmFields)
		if(farmField->plantSpecies == &plant.m_plantSpecies)
			plant.m_location->m_area->m_hasFarmFields.at(*faction).addHarvestDesignation(plant);
}
void IsPartOfFarmField::designateForGiveFluidIfPartOfFarmField(Plant& plant)
{
	for(auto& [faction, farmField] : m_farmFields)
		if(farmField->plantSpecies == &plant.m_plantSpecies)
			plant.m_location->m_area->m_hasFarmFields.at(*faction).addGivePlantFluidDesignation(plant);
}
void IsPartOfFarmField::maybeDesignateForSowingIfPartOfFarmField()
{
	assert(!m_block.m_hasPlant.exists());
	for(auto& [faction, farmField] : m_farmFields)
		if(isSowingSeasonFor(*farmField->plantSpecies))
			m_block.m_area->m_hasFarmFields.at(*faction).addSowSeedsDesignation(m_block);
}
void IsPartOfFarmField::removeAllHarvestDesignations()
{
	Plant& plant = m_block.m_hasPlant.get();	
	for(auto& [faction, farmField] : m_farmFields)
		if(farmField->plantSpecies == &plant.m_plantSpecies)
			if(m_block.m_hasDesignations.contains(*faction, BlockDesignation::Harvest))
				m_block.m_area->m_hasFarmFields.at(*faction).removeHarvestDesignation(plant);
}
void IsPartOfFarmField::removeAllGiveFluidDesignations()
{
	Plant& plant = m_block.m_hasPlant.get();	
	for(auto& [faction, farmField] : m_farmFields)
		if(farmField->plantSpecies == &plant.m_plantSpecies)
			if(m_block.m_hasDesignations.contains(*faction, BlockDesignation::GivePlantFluid))
				m_block.m_area->m_hasFarmFields.at(*faction).removeGivePlantFluidDesignation(plant);
}
void IsPartOfFarmField::removeAllSowSeedsDesignations()
{
	for(auto& [faction, farmField] : m_farmFields)
		if(m_block.m_hasDesignations.contains(*faction, BlockDesignation::SowSeeds))
			m_block.m_area->m_hasFarmFields.at(*faction).removeSowSeedsDesignation(m_block);
}
bool IsPartOfFarmField::isSowingSeasonFor(const PlantSpecies& species) const
{
	return m_block.m_area->m_simulation.m_now.day >= species.dayOfYearForSowStart && m_block.m_area->m_simulation.m_now.day <= species.dayOfYearForSowEnd;
}
HasFarmFieldsForFaction::HasFarmFieldsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& a, const Faction& f) : m_area(a), m_faction(f), m_plantsNeedingFluidIsSorted(data["plantsNeedingFluidIsSorted"].get<bool>())
{
	for(const Json& farmFieldData : data["farmFields"])
		m_farmFields.emplace_back(farmFieldData, deserializationMemo, m_faction);
	for(const Json& plantReference : data["plantsNeedingFluid"])
		m_plantsNeedingFluid.push_back(&deserializationMemo.plantReference(plantReference));
	for(const Json& plantReference : data["plantsToHarvest"])
		m_plantsToHarvest.insert(&deserializationMemo.plantReference(plantReference));
	for(const Json& blockReference : data["blocksNeedingSeedsSewn"])
		m_blocksNeedingSeedsSewn.insert(&deserializationMemo.blockReference(blockReference));
}
Json HasFarmFieldsForFaction::toJson() const
{
	Json data{{"farmFields", Json::array(), "plantsNeedingFluid", Json::array()}, {"plantsToHarvest", Json::array()}, {"blocksNeedingSeedsSewn", Json::array()}, {"plantsNeedingFluidIsSorted", m_plantsNeedingFluidIsSorted}};
	for(const FarmField& farmField : m_farmFields)
		data["farmFields"].push_back(farmField.toJson());
	for(const Plant* plant : m_plantsNeedingFluid)
		data["plantsNeedingFluid"].push_back(plant);
	for(const Plant* plant : m_plantsToHarvest)
		data["plantsToHarvest"].push_back(plant);
	for(const Block* block : m_blocksNeedingSeedsSewn)
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
const PlantSpecies& HasFarmFieldsForFaction::getPlantSpeciesFor(const Block& block) const
{
	for(const FarmField& farmField : m_farmFields)
		if(farmField.blocks.contains(const_cast<Block*>(&block)))
			return *farmField.plantSpecies;
	assert(false);
}
void HasFarmFieldsForFaction::addGivePlantFluidDesignation(Plant& plant)
{
	assert(std::ranges::find(m_plantsNeedingFluid, &plant) == m_plantsNeedingFluid.end());
	m_plantsNeedingFluidIsSorted = false;
	m_plantsNeedingFluid.push_back(&plant);
	plant.m_location->m_hasDesignations.insert(m_faction, BlockDesignation::GivePlantFluid);
}
void HasFarmFieldsForFaction::removeGivePlantFluidDesignation(Plant& plant)
{
	assert(std::ranges::find(m_plantsNeedingFluid, &plant) != m_plantsNeedingFluid.end());
	std::erase(m_plantsNeedingFluid, &plant);
	plant.m_location->m_hasDesignations.remove(m_faction, BlockDesignation::GivePlantFluid);
}
void HasFarmFieldsForFaction::addSowSeedsDesignation(Block& block)
{
	assert(!m_blocksNeedingSeedsSewn.contains(&block));
	m_blocksNeedingSeedsSewn.insert(&block);
	block.m_hasDesignations.insert(m_faction, BlockDesignation::SowSeeds);
}
void HasFarmFieldsForFaction::removeSowSeedsDesignation(Block& block)
{
	assert(m_blocksNeedingSeedsSewn.contains(&block));
	m_blocksNeedingSeedsSewn.erase(&block);
	block.m_hasDesignations.remove(m_faction, BlockDesignation::SowSeeds);
}
void HasFarmFieldsForFaction::addHarvestDesignation(Plant& plant)
{
	assert(!m_plantsToHarvest.contains(&plant));
	m_plantsToHarvest.insert(&plant);
	plant.m_location->m_hasDesignations.insert(m_faction, BlockDesignation::Harvest);
}
void HasFarmFieldsForFaction::removeHarvestDesignation(Plant& plant)
{
	assert(m_plantsToHarvest.contains(&plant));
	m_plantsToHarvest.erase(&plant);
	plant.m_location->m_hasDesignations.remove(m_faction, BlockDesignation::Harvest);
}
void HasFarmFieldsForFaction::setDayOfYear(uint32_t dayOfYear)
{
	//TODO: Add sow designation when blocking item or plant is removed.
	for(FarmField& farmField : m_farmFields)
		if(farmField.plantSpecies != nullptr)
		{
			if(farmField.plantSpecies->dayOfYearForSowStart == dayOfYear)
			{
				for(Block* block : farmField.blocks)
					if(block->m_hasPlant.canGrowHereAtSomePointToday(*farmField.plantSpecies))
						addSowSeedsDesignation(*block);
				farmField.timeToSow = true;
			}
			if(farmField.plantSpecies->dayOfYearForSowEnd == dayOfYear)
			{
				assert(farmField.timeToSow);
				farmField.timeToSow = false;
				for(Block* block : farmField.blocks)
					if(m_blocksNeedingSeedsSewn.contains(block))
						removeSowSeedsDesignation(*block);
			}
		}
}
FarmField& HasFarmFieldsForFaction::create(std::unordered_set<Block*>& blocks)
{
	m_farmFields.emplace_back(blocks);
	for(Block* block : blocks)
		block->m_isPartOfFarmField.insert(m_faction, m_farmFields.back());
	return m_farmFields.back();
}
FarmField& HasFarmFieldsForFaction::create(Cuboid cuboid)
{
	auto set = cuboid.toSet();
	return create(set);
}
void HasFarmFieldsForFaction::extend(FarmField& farmField, std::unordered_set<Block*>& blocks)
{
	farmField.blocks.insert(blocks.begin(), blocks.end());
	if(farmField.plantSpecies != nullptr)
		designateBlocks(farmField, blocks);
	for(Block* block : blocks)
		block->m_isPartOfFarmField.insert(m_faction, farmField);
}
void HasFarmFieldsForFaction::setSpecies(FarmField& farmField, const PlantSpecies& plantSpecies)
{
	assert(farmField.plantSpecies == nullptr);
	farmField.plantSpecies = &plantSpecies;
	if(m_area.m_simulation.m_now.day >= plantSpecies.dayOfYearForSowStart && m_area.m_simulation.m_now.day <= plantSpecies.dayOfYearForSowEnd)
		designateBlocks(farmField, farmField.blocks);
}
void HasFarmFieldsForFaction::designateBlocks(FarmField& farmField, std::unordered_set<Block*>& blocks)
{
	// Designate for sow if the season is right.
	if(farmField.timeToSow)
		for(Block* block : blocks)
			addSowSeedsDesignation(*block);
	for(Block* block : blocks)
	{
		// Designate plants already existing for fluid if the species is right and they need fluid.
		if(block->m_hasPlant.exists())
		{
			Plant& plant = block->m_hasPlant.get();
			if(&plant.m_plantSpecies == farmField.plantSpecies && plant.m_volumeFluidRequested)
				addGivePlantFluidDesignation(plant);
		}
	}
}
void HasFarmFieldsForFaction::clearSpecies(FarmField& farmField)
{
	assert(farmField.plantSpecies != nullptr);
	farmField.plantSpecies = nullptr;
	undesignateBlocks(farmField.blocks);
}
void HasFarmFieldsForFaction::shrink(FarmField& farmField, std::unordered_set<Block*>& blocks)
{
	undesignateBlocks(blocks);
	for(Block* block : blocks)
		block->m_isPartOfFarmField.remove(m_faction);
	std::erase_if(farmField.blocks, [&](Block* block){ return blocks.contains(block); });
	if(farmField.blocks.empty())
		m_farmFields.remove_if([&](FarmField& other){ return &other == &farmField; });
}
void HasFarmFieldsForFaction::remove(FarmField& farmField)
{
	shrink(farmField, farmField.blocks);
}
void HasFarmFieldsForFaction::undesignateBlocks(std::unordered_set<Block*>& blocks)
{
	for(Block* block : blocks)
	{
		if(block->m_hasPlant.exists())
		{
			Plant& plant = block->m_hasPlant.get();
			if(block->m_hasDesignations.contains(m_faction, BlockDesignation::GivePlantFluid))
				block->m_area->m_hasFarmFields.at(m_faction).removeGivePlantFluidDesignation(plant);
			if(block->m_hasDesignations.contains(m_faction, BlockDesignation::Harvest))
				block->m_area->m_hasFarmFields.at(m_faction).removeHarvestDesignation(plant);
		}
		else if(block->m_hasDesignations.contains(m_faction, BlockDesignation::SowSeeds))
				block->m_area->m_hasFarmFields.at(m_faction).removeSowSeedsDesignation(*block);
	}
}
HasFarmFieldsForFaction& HasFarmFields::at(const Faction& faction)
{
	return m_data.at(&faction);
}
void HasFarmFields::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		const Faction& faction = deserializationMemo.faction(pair[0]);
		assert(!m_data.contains(&faction));
		m_data.try_emplace(&faction, pair[1], deserializationMemo, m_area, faction);
	}
}
Json HasFarmFields::toJson() const
{
	Json data;
	for(auto& [faction, hasFarmFieldsForFaction] : m_data)
		Json pair{faction, hasFarmFieldsForFaction.toJson()};
	return data;
}
void HasFarmFields::registerFaction(const Faction& faction)
{
	assert(!m_data.contains(&faction));
	m_data.try_emplace(&faction, m_area, faction);
}
void HasFarmFields::unregisterFaction(const Faction& faction)
{
	assert(m_data.contains(&faction));
	m_data.erase(&faction);
}
Plant* HasFarmFields::getHighestPriorityPlantForGiveFluid(const Faction& faction)
{
	assert(m_data.contains(&faction));
	return m_data.at(&faction).getHighestPriorityPlantForGiveFluid();
}
void HasFarmFields::removeAllSowSeedsDesignations(Block& block)
{
	for(auto& pair : m_data)
		if(block.m_hasDesignations.contains(*pair.first, BlockDesignation::SowSeeds))
			pair.second.removeSowSeedsDesignation(block);
}
void HasFarmFields::setDayOfYear(uint32_t dayOfYear)
{
	for(auto& pair : m_data)
		pair.second.setDayOfYear(dayOfYear);
}
bool HasFarmFields::hasGivePlantsFluidDesignations(const Faction& faction) const
{
	assert(m_data.contains(&faction));
	return m_data.at(&faction).hasGivePlantsFluidDesignations();
}
bool HasFarmFields::hasSowSeedsDesignations(const Faction& faction) const
{
	return m_data.at(&faction).hasSowSeedsDesignations();
}
bool HasFarmFields::hasHarvestDesignations(const Faction& faction) const
{
	return m_data.at(&faction).hasHarvestDesignations();
}
const PlantSpecies& HasFarmFields::getPlantSpeciesFor(const Faction& faction, const  Block& location) const
{
	return m_data.at(&faction).getPlantSpeciesFor(location);
}