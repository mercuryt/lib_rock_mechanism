#include "space.h"
#include "area/area.h"
#include "definitions/plantSpecies.h"
#include "simulation/simulation.h"
#include "../plants.h"
void Space::farm_insert(const Point3D& point, const FactionId& faction, FarmField& farmField)
{
	if(m_farmFields.contains(point))
		assert(!m_farmFields[point].contains(faction));
	m_farmFields.getOrCreate(point).insert(faction, &farmField);
}
void Space::farm_remove(const Point3D& point, const FactionId& faction)
{
	assert(m_farmFields.contains(point));
	assert(m_farmFields[point].contains(faction));
	m_farmFields[point].erase(faction);
	auto& designations = m_area.m_spaceDesignations.getForFaction(faction);
	designations.maybeUnset(point, SpaceDesignation::SowSeeds);
	designations.maybeUnset(point, SpaceDesignation::GivePlantFluid);
	designations.maybeUnset(point, SpaceDesignation::Harvest);
}
void Space::farm_designateForHarvestIfPartOfFarmField(const Point3D& point, const PlantIndex& plant)
{
	if(m_farmFields.contains(point))
	{
		Plants& plants = m_area.getPlants();
		for(auto& [faction, farmField] : m_farmFields[point])
			if(farmField->plantSpecies == plants.getSpecies(plant))
				m_area.m_hasFarmFields.getForFaction(faction).addHarvestDesignation(m_area, plant);
	}
}
void Space::farm_designateForGiveFluidIfPartOfFarmField(const Point3D& point, const PlantIndex& plant)
{
	if(m_farmFields.contains(point))
	{
		Plants& plants = m_area.getPlants();
		for(auto& [faction, farmField] : m_farmFields[point])
			if(farmField->plantSpecies == plants.getSpecies(plant))
				m_area.m_hasFarmFields.getForFaction(faction).addGivePlantFluidDesignation(m_area, point);
	}
}
void Space::farm_maybeDesignateForSowingIfPartOfFarmField(const Point3D& point)
{
	if(m_farmFields.contains(point))
	{
		assert(!m_plants.queryAny(point));
		for(auto& [faction, farmField] : m_farmFields[point])
			if(farm_isSowingSeasonFor(farmField->plantSpecies))
				m_area.m_hasFarmFields.getForFaction(faction).addSowSeedsDesignation(m_area, point);
	}
}
void Space::farm_removeAllHarvestDesignations(const Point3D& point)
{
	PlantIndex plant = m_plants.queryGetOne(point);
	if(plant.empty())
		return;
	if(m_farmFields.contains(point))
		for(auto& [faction, farmField] : m_farmFields[point])
			if(designation_has(point, faction, SpaceDesignation::Harvest))
				m_area.m_hasFarmFields.getForFaction(faction).removeHarvestDesignation(m_area, plant);
}
void Space::farm_removeAllGiveFluidDesignations(const Point3D& point)
{
	PlantIndex plant = m_plants.queryGetOne(point);
	if(plant.empty())
		return;
	if(m_farmFields.contains(point))
		for(auto& [faction, farmField] : m_farmFields[point])
			if(designation_has(point, faction, SpaceDesignation::GivePlantFluid))
				m_area.m_hasFarmFields.getForFaction(faction).removeGivePlantFluidDesignation(m_area, point);
}
void Space::farm_removeAllSowSeedsDesignations(const Point3D& point)
{
	if(m_farmFields.contains(point))
		for(auto& [faction, farmField] : m_farmFields[point])
			if(designation_has(point, faction, SpaceDesignation::SowSeeds))
				m_area.m_hasFarmFields.getForFaction(faction).removeSowSeedsDesignation(m_area, point);
}
bool Space::farm_isSowingSeasonFor(const PlantSpeciesId& species) const
{
	uint16_t day = DateTime(m_area.m_simulation.m_step).day;
	return day >= PlantSpecies::getDayOfYearForSowStart(species) && day <= PlantSpecies::getDayOfYearForSowEnd(species);
}
FarmField* Space::farm_get(const Point3D& point, const FactionId& faction)
{
	if(!m_farmFields.contains(point) || !m_farmFields[point].contains(faction))
		return nullptr;
	return m_farmFields[point][faction];
}
bool Space::farm_contains(const Point3D& point, const FactionId& faction) const
{
	return const_cast<Space*>(this)->farm_get(point, faction) != nullptr;
}

