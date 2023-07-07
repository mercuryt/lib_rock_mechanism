#include "farmFields.h"
void IsPartOfFarmField::insert(Player& player, FarmField& farmField)
{
	assert(!m_farmFields.contains(&player));
	m_farmFields[&player] = &farmField;
}
void IsPartOfFarmField::remove(Player& player)
{
	assert(m_farmFields.contains(&player));
	m_farmFields.remove(&player);
}
void IsPartOfFarmField::designateForHarvestIfPartOfFarmField(Plant& plant)
{
	for(auto& [player, farmField] : m_farmFields)
		if(farmField.plantSpecies == plant.plantSpecies)
			block.m_area->m_hasFarmFields.at(player).addHarvestDesignation(plant);
}
void IsPartOfFarmField::designateForGiveFluidIfPartOfFarmField(Plant& plant)
{
	for(auto& [player, farmField] : m_farmFields)
		if(farmField.plantSpecies == plant.plantSpecies)
			block.m_area->m_hasFarmFields.at(player).addGivePlantFluidDesignation(plant);
}
void IsPartOfFarmField::removeAllHarvestDesignations()
{
	Plant& plant = m_block.m_hasPlant.get();	
	for(auto& [player, farmField] : m_farmFields)
		if(plant.plantSpecies == farmField.plantSpecies)
			m_block.m_area->m_hasFarmFields.at(player).removeHarvestDesignation(plant);
}
void IsPartOfFarmField::removeAllGiveFluidDesignations()
{
	Plant& plant = m_block.m_hasPlant.get();	
	for(auto& [player, farmField] : m_farmFields)
		if(plant.plantSpecies == farmField.plantSpecies)
			m_block.m_area->m_hasFarmFields.at(player).removeGiveFluidDesignation(plant);
}
void IsPartOfFarmField::removeAllSowSeedsDesignations()
{
	for(auto& [player, farmField] : m_farmFields)
		m_block.m_area->m_hasFarmFields.at(player).removeSowSeedsDesignation(m_block);
}
bool HasFarmFieldsForPlayer::hasGivePlantsFluidDesignations() const
{
	return !m_plantsNeedingFluid.empty();
}
Plant* HasFarmFieldsForPlayer::getHighPriorityPlantForGivingFluidIfAny()
{
	if(!m_plantsNeedingFluidIsSorted)
	{
		std::ranges::sort_by(m_plantsNeedingFluid, [](Plant* a, Plant* b){ return a->m_needsFluid.getDeathStep() < b->m_needsFluid.getDeathStep(); });
		m_plantsNeedingFluidIsSorted = true;
	}
	Plant* output = m_plantsNeedingFluid.first();
	if(output.m_needsFluid.getDeathStep() - simulation::step > Config::stepsTillDiePlantPriorityOveride)
		return nullptr;
	return *m_plantsNeedingFluid.first();
}
bool HasFarmFieldsForPlayer::hasSowSeedDesignations() const
{
	return !m_blocksNeedingSeedSewn.empty();
}
const HasFarmFieldsForPlayer::PlantSpecies& getPlantTypeFor(Block& block) const
{
	for(FarmField& farmField : m_farmFields)
		if(farmField.blocks.contains(&block))
			return farmField.plantSpecies;
	assert(false);
}
void HasFarmFieldsForPlayer::addGivePlantFluidDesignation(Plant& plant)
{
	assert(!m_plantsNeedingFluid.contains(plant));
	m_plantsNeedingFluidIsSorted = false;
	m_plantsNeedingFluid.push_back(&plant);
}
void HasFarmFieldsForPlayer::removeGivePlantFluidDesignation(Plant& plant)
{
	assert(m_plantsNeedingFluid.contains(plant));
	m_plantsNeedingFluid.remove(plant);
	m_plant.m_location->m_hasDesignations.remove(m_player, BlockDesignation::GiveFluid);
}
void HasFarmFieldsForPlayer::addSowSeedDesignation(Block& block)
{
	assert(!m_blocksNeedingSeedSewn.contains(&block));
	m_blocksNeedingSeedSewn.insert(&block);
	block.m_hasDesignations.insert(m_player, BlockDesignation::SowSeed);
}
void HasFarmFieldsForPlayer::removeSowSeedDesignation(Block& block)
{
	assert(m_blocksNeedingSeedSewn.contains(&block));
	m_blocksNeedingSeedSewn.remove(&block);
	block.m_hasDesignations.remove(m_player, BlockDesignation::SowSeed);
}
void HasFarmFieldsForPlayer::addHarvestDesignation(Plant& plant)
{
	assert(!m_plantsToHarvest.contains(&plant));
	m_plantsToHarvest.insert(&plant);
	plant.m_location.m_hasDesignations.insert(m_player, BlockDesignation::Harvest);
}
void HasFarmFieldsForPlayer::removeHarvestDesignation(Plant& plant)
{
	assert(m_plantsToHarvest.contains(&plant));
	m_plantsToHarvest.remove(&plant);
	plant.m_location.m_hasDesignations.remove(m_player, BlockDesignation::Harvest);
}
void HasFarmFieldsForPlayer::setDayOfYear(uint32_t dayOfYear)
{
	//TODO: Add sow designation when blocking item or plant is removed.
	for(FarmField& farmField : m_farmFields)
		if(farmField.plantSpecies != nullptr)
		{
			if(farmField.plantSpecies.dayOfYearForSowStart == dayOfYear)
			{
				for(Block* block : farmField.blocks)
					if(block->m_hasPlants.canGrowHere(farmField.plantSpecies))
						addSowSeedDesignation(*block);
				farmField.timeToSow = true;
			}
			if(farmField.plantSpecies.dayOfYearForSowEnd == dayOfYear)
			{
				assert(farmField.timeToSow);
				farmField.timeToSow = false;
				for(Block* block : farmField.blocks)
					if(m_blocksNeedingSeedSewn.contains(block))
						removeSowSeedDesignation(*block);
			}
		}
}
FarmField& HasFarmFieldsForPlayer::create(std::unordered_set<Block*>& blocks)
{
	m_farmFields.emplace_back(blocks);
	for(Block* block : blocks)
		block->m_isPartOfFarmField.insert(m_player, m_farmFields.back());
	return &m_farmFields.back();
}
void HasFarmFieldsForPlayer::extend(FarmField& farmField, std:unordered_set<Block*>& blocks)
{
	farmField.blocks.insert(blocks.begin(), blocks.end());
	if(farmField.plantSpecies != nullptr)
		designateBlocks(farmField, blocks);
	for(Block* block : blocks)
		block->m_isPartOfFarmField.insert(m_player, farmField);
}
void HasFarmFieldsForPlayer::setSpecies(FarmField& farmField, const PlantSpecies& plantSpecies)
{
	assert(farmField.plantSpecies == nullptr);
	farmField.plantSpecies = plantSpecies;
	designateBlocks(farmField, farmField.blocks);
}
void HasFarmFieldsForPlayer::designateBlocks(FarmField& farmField, std::unordered_set<Block*>& blocks)
{
	// Designate for sow if the season is right.
	if(farmField.timeToSow)
		for(Block* block : blocks)
			addSowSeedDesignation(*block);
	for(Block* block : blocks)
	{
		// Designate plants already existing for fluid if the species is right and they need fluid.
		if(!block.m_hasPlant.empty())
		{
			Plant& plant = block->m_hasPlant.get();
			if(plant.plantSpecies == farmField.plantSpecies && !plant.m_hasFluid)
				addGivePlantFluidDesignation(plant);
		}
	}
}
void HasFarmFieldsForPlayer::clearSpecies(FarmField& farmField)
{
	assert(farmField.plantSpecies != nullptr);
	farmField.plantSpecies = nullptr;
	undesignateBlocks();
}
void HasFarmFieldsForPlayer::shrink(FarmField& farmField, std::unordered_set<Block*>& blocks)
{
	undesignateBlocks(blocks);
	std::erase_if(farmField.blocks, [&](Block* block){ return blocks.contains(block); });
	if(farmField.blocks.empty())
		m_farmFields.remove(farmField);
	for(Block* block : blocks)
		block->m_isPartOfFarmField.remove(m_player);
}
void HasFarmFieldsForPlayer::remove(FarmField& farmField)
{
	shrink(farmField, farmField.blocks);
}
void HasFarmFieldsForPlayer::undesignateBlocks(std::unordered_set<Block*>& blocks)
{
	for(Block* block : blocks)
	{
		block->m_hasDesignations.removeIfExists(BlockDesignation::GiveFluid);
		block->m_hasDesignations.removeIfExists(BlockDesignation::SowSeed);
		block->m_hasDesignations.removeIfExists(BlockDesignation::Harvest);
	}
}
void HasFarmFields::at(Player& player)
{
	return m_data.at(player);
}
void HasFarmFields::registerPlayer(Player& player)
{
	assert(!m_data.contains(player));
	m_data[&player](player);
}
void HasFarmFields::unregisterPlayer(Player& player)
{
	assert(m_data.contains(player));
	m_data.erase(&player);
}
