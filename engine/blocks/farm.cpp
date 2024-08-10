#include "blocks.h"
#include "area.h"
#include "plantSpecies.h"
#include "simulation.h"
#include "../plants.h"
void Blocks::farm_insert(BlockIndex index, FactionId faction, FarmField& farmField)
{
	if(m_farmFields.contains(index))
		assert(!m_farmFields.at(index).contains(faction));
	m_farmFields[index][faction] = &farmField;
}
void Blocks::farm_remove(BlockIndex index, FactionId faction)
{
	assert(m_farmFields.contains(index));
	assert(m_farmFields.at(index).contains(faction));
	m_farmFields.at(index).erase(faction);
	auto& designations = m_area.m_blockDesignations.getForFaction(faction);
	designations.maybeUnset(index, BlockDesignation::SowSeeds);
	designations.maybeUnset(index, BlockDesignation::GivePlantFluid);
	designations.maybeUnset(index, BlockDesignation::Harvest);
}
void Blocks::farm_designateForHarvestIfPartOfFarmField(BlockIndex index, PlantIndex plant)
{
	if(m_farmFields.contains(index))
	{
		Plants& plants = m_area.getPlants();
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(farmField->plantSpecies == plants.getSpecies(plant))
				m_area.m_hasFarmFields.getForFaction(faction).addHarvestDesignation(plant);
	}
}
void Blocks::farm_designateForGiveFluidIfPartOfFarmField(BlockIndex index, PlantIndex plant)
{
	if(m_farmFields.contains(index))
	{
		Plants& plants = m_area.getPlants();
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(farmField->plantSpecies == plants.getSpecies(plant))
				m_area.m_hasFarmFields.getForFaction(faction).addGivePlantFluidDesignation(plant);
	}
}
void Blocks::farm_maybeDesignateForSowingIfPartOfFarmField(BlockIndex index)
{
	if(m_farmFields.contains(index))
	{
		assert(m_plants[index].empty());
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(farm_isSowingSeasonFor(farmField->plantSpecies))
				m_area.m_hasFarmFields.getForFaction(faction).addSowSeedsDesignation(index);
	}
}
void Blocks::farm_removeAllHarvestDesignations(BlockIndex index)
{
	PlantIndex plant = m_plants[index];	
	if(plant.empty())
		return;
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(designation_has(index, faction, BlockDesignation::Harvest))
				m_area.m_hasFarmFields.getForFaction(faction).removeHarvestDesignation(plant);
}
void Blocks::farm_removeAllGiveFluidDesignations(BlockIndex index)
{
	PlantIndex plant = m_plants[index];	
	if(plant.empty())
		return;
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(designation_has(index, faction, BlockDesignation::GivePlantFluid))
				m_area.m_hasFarmFields.getForFaction(faction).removeGivePlantFluidDesignation(plant);
}
void Blocks::farm_removeAllSowSeedsDesignations(BlockIndex index)
{
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields.at(index))
			if(designation_has(index, faction, BlockDesignation::SowSeeds))
				m_area.m_hasFarmFields.getForFaction(faction).removeSowSeedsDesignation(index);
}
bool Blocks::farm_isSowingSeasonFor(PlantSpeciesId species) const
{
	uint16_t day = DateTime(m_area.m_simulation.m_step).day;
	return day >= PlantSpecies::getDayOfYearForSowStart(species) && day <= PlantSpecies::getDayOfYearForSowEnd(species);
}
FarmField* Blocks::farm_get(BlockIndex index, FactionId faction)
{ 
	if(!m_farmFields.contains(index) || !m_farmFields.at(index).contains(faction)) 
		return nullptr; 
	return m_farmFields.at(index).at(faction); 
}
bool Blocks::farm_contains(BlockIndex index, FactionId faction) const
{ 
	return const_cast<Blocks*>(this)->farm_get(index, faction) != nullptr;
}

