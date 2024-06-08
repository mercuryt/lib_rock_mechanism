#include "blocks.h"
#include "area.h"
#include "simulation.h"
void Blocks::farm_insert(BlockIndex index, Faction& faction, FarmField& farmField)
{
	if(m_farmFields.contains(index))
		assert(!m_farmFields.at(index).contains(&faction));
	m_farmFields[index][&faction] = &farmField;
}
void Blocks::farm_remove(BlockIndex index, Faction& faction)
{
	assert(m_farmFields.contains(index));
	assert(m_farmFields.at(index).contains(&faction));
	m_farmFields.at(index).erase(&faction);
	auto& designations = m_area.m_blockDesignations.at(faction);
	designations.maybeUnset(index, BlockDesignation::SowSeeds);
	designations.maybeUnset(index, BlockDesignation::GivePlantFluid);
	designations.maybeUnset(index, BlockDesignation::Harvest);
}
void Blocks::farm_designateForHarvestIfPartOfFarmField(BlockIndex index, Plant& plant)
{
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(farmField->plantSpecies == &plant.m_plantSpecies)
				m_area.m_hasFarmFields.at(*faction).addHarvestDesignation(plant);
}
void Blocks::farm_designateForGiveFluidIfPartOfFarmField(BlockIndex index, Plant& plant)
{
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(farmField->plantSpecies == &plant.m_plantSpecies)
				m_area.m_hasFarmFields.at(*faction).addGivePlantFluidDesignation(plant);
}
void Blocks::farm_maybeDesignateForSowingIfPartOfFarmField(BlockIndex index)
{
	if(m_farmFields.contains(index))
	{
		assert(!m_plants.at(index));
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(farm_isSowingSeasonFor(*farmField->plantSpecies))
				m_area.m_hasFarmFields.at(*faction).addSowSeedsDesignation(index);
	}
}
void Blocks::farm_removeAllHarvestDesignations(BlockIndex index)
{
	Plant* plant = m_plants.at(index);	
	if(!plant)
		return;
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(designation_has(index, *faction, BlockDesignation::Harvest))
				m_area.m_hasFarmFields.at(*faction).removeHarvestDesignation(*plant);
}
void Blocks::farm_removeAllGiveFluidDesignations(BlockIndex index)
{
	Plant* plant = m_plants.at(index);	
	if(!plant)
		return;
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(designation_has(index, *faction, BlockDesignation::GivePlantFluid))
				m_area.m_hasFarmFields.at(*faction).removeGivePlantFluidDesignation(*plant);
}
void Blocks::farm_removeAllSowSeedsDesignations(BlockIndex index)
{
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(designation_has(index, *faction, BlockDesignation::SowSeeds))
				m_area.m_hasFarmFields.at(*faction).removeSowSeedsDesignation(index);
}
bool Blocks::farm_isSowingSeasonFor(const PlantSpecies& species) const
{
	uint16_t day = DateTime(m_area.m_simulation.m_step).day;
	return day >= species.dayOfYearForSowStart && day <= species.dayOfYearForSowEnd;
}
FarmField* Blocks::farm_get(BlockIndex index, Faction& faction)
{ 
	if(!m_farmFields.contains(index) || !m_farmFields.at(index).contains(&faction)) 
		return nullptr; 
	return m_farmFields.at(index).at(&faction); 
}
bool Blocks::farm_contains(BlockIndex index, Faction& faction) const
{ 
	return const_cast<Blocks*>(this)->farm_get(index, faction) != nullptr;
}

