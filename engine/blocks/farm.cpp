#include "blocks.h"
#include "area/area.h"
#include "plantSpecies.h"
#include "simulation/simulation.h"
#include "../plants.h"
void Blocks::farm_insert(const BlockIndex& index, const FactionId& faction, FarmField& farmField)
{
	if(m_farmFields.contains(index))
		assert(!m_farmFields[index].contains(faction));
	m_farmFields.getOrCreate(index).insert(faction, &farmField);
}
void Blocks::farm_remove(const BlockIndex& index, const FactionId& faction)
{
	assert(m_farmFields.contains(index));
	assert(m_farmFields[index].contains(faction));
	m_farmFields[index].erase(faction);
	auto& designations = m_area.m_blockDesignations.getForFaction(faction);
	designations.maybeUnset(index, BlockDesignation::SowSeeds);
	designations.maybeUnset(index, BlockDesignation::GivePlantFluid);
	designations.maybeUnset(index, BlockDesignation::Harvest);
}
void Blocks::farm_designateForHarvestIfPartOfFarmField(const BlockIndex& index, const PlantIndex& plant)
{
	if(m_farmFields.contains(index))
	{
		Plants& plants = m_area.getPlants();
		for(auto& [faction, farmField] : m_farmFields[index])
			if(farmField->plantSpecies == plants.getSpecies(plant))
				m_area.m_hasFarmFields.getForFaction(faction).addHarvestDesignation(m_area, plant);
	}
}
void Blocks::farm_designateForGiveFluidIfPartOfFarmField(const BlockIndex& index, const PlantIndex& plant)
{
	if(m_farmFields.contains(index))
	{
		Plants& plants = m_area.getPlants();
		for(auto& [faction, farmField] : m_farmFields[index])
			if(farmField->plantSpecies == plants.getSpecies(plant))
				m_area.m_hasFarmFields.getForFaction(faction).addGivePlantFluidDesignation(m_area, index);
	}
}
void Blocks::farm_maybeDesignateForSowingIfPartOfFarmField(const BlockIndex& index)
{
	if(m_farmFields.contains(index))
	{
		assert(m_plants[index].empty());
		for(auto& [faction, farmField] : m_farmFields[index])
			if(farm_isSowingSeasonFor(farmField->plantSpecies))
				m_area.m_hasFarmFields.getForFaction(faction).addSowSeedsDesignation(m_area, index);
	}
}
void Blocks::farm_removeAllHarvestDesignations(const BlockIndex& index)
{
	PlantIndex plant = m_plants[index];
	if(plant.empty())
		return;
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields[index])
			if(designation_has(index, faction, BlockDesignation::Harvest))
				m_area.m_hasFarmFields.getForFaction(faction).removeHarvestDesignation(m_area, plant);
}
void Blocks::farm_removeAllGiveFluidDesignations(const BlockIndex& index)
{
	PlantIndex plant = m_plants[index];
	if(plant.empty())
		return;
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields[index])
			if(designation_has(index, faction, BlockDesignation::GivePlantFluid))
				m_area.m_hasFarmFields.getForFaction(faction).removeGivePlantFluidDesignation(m_area, index);
}
void Blocks::farm_removeAllSowSeedsDesignations(const BlockIndex& index)
{
	if(m_farmFields.contains(index))
		for(auto& [faction, farmField] : m_farmFields[index])
			if(designation_has(index, faction, BlockDesignation::SowSeeds))
				m_area.m_hasFarmFields.getForFaction(faction).removeSowSeedsDesignation(m_area, index);
}
bool Blocks::farm_isSowingSeasonFor(const PlantSpeciesId& species) const
{
	uint16_t day = DateTime(m_area.m_simulation.m_step).day;
	return day >= PlantSpecies::getDayOfYearForSowStart(species) && day <= PlantSpecies::getDayOfYearForSowEnd(species);
}
FarmField* Blocks::farm_get(const BlockIndex& index, const FactionId& faction)
{
	if(!m_farmFields.contains(index) || !m_farmFields[index].contains(faction))
		return nullptr;
	return m_farmFields[index][faction];
}
bool Blocks::farm_contains(const BlockIndex& index, const FactionId& faction) const
{
	return const_cast<Blocks*>(this)->farm_get(index, faction) != nullptr;
}

