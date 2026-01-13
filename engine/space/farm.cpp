#include "space.h"
#include "../area/area.h"
#include "../definitions/plantSpecies.h"
#include "../simulation/simulation.h"
#include "../plants.h"
void Space::farm_designateForHarvestIfPartOfFarmField(const Point3D& point, const PlantIndex& plant)
{
	Plants& plants = m_area.getPlants();
	const PlantSpeciesId& species = plants.getSpecies(plant);
	for(const auto& [faction, fields] : m_farmFields)
	{
		const FarmField* farm = fields.queryGetOne(point).get();
		if(farm != nullptr && farm->m_plantSpecies == species)
				m_area.m_hasFarmFields.getForFaction(faction).addHarvestDesignation(m_area, plant);
	}
}
void Space::farm_designateForGiveFluidIfPartOfFarmField(const Point3D& point, const PlantIndex& plant)
{
	Plants& plants = m_area.getPlants();
	const PlantSpeciesId& species = plants.getSpecies(plant);
	const Point3D& location = plants.getLocation(plant);
	for(const auto& [faction, fields] : m_farmFields)
	{
		const FarmField* farm = fields.queryGetOne(point).get();
		if(farm != nullptr && farm->m_plantSpecies == species)
			m_area.m_hasFarmFields.getForFaction(faction).addGivePlantFluidDesignation(m_area, location);
	}
}
void Space::farm_maybeDesignateForSowingIfPartOfFarmField(const Point3D& point)
{
	for(const auto& [faction, fields] : m_farmFields)
	{
		const FarmField* farm = fields.queryGetOne(point).get();
		if(farm != nullptr && !designation_has(point, faction, SpaceDesignation::SowSeeds) && !plant_exists(point))
			m_area.m_hasFarmFields.getForFaction(faction).addSowSeedsDesignation(m_area, point);
	}
}
void Space::farm_removeAllHarvestDesignations(const Point3D& point)
{
	const PlantIndex plant = m_plants.queryGetOne(point);
	for(const auto& [faction, fields] : m_farmFields)
	{
		const FarmField* farm = fields.queryGetOne(point).get();
		if(farm != nullptr && designation_has(point, faction, SpaceDesignation::Harvest))
				m_area.m_hasFarmFields.getForFaction(faction).removeHarvestDesignation(m_area, plant);
	}
}
void Space::farm_removeAllGiveFluidDesignations(const Point3D& point)
{
	for(const auto& [faction, fields] : m_farmFields)
	{
		const FarmField* farm = fields.queryGetOne(point).get();
		if(farm != nullptr && designation_has(point, faction, SpaceDesignation::GivePlantFluid))
				m_area.m_hasFarmFields.getForFaction(faction).removeGivePlantFluidDesignation(m_area, point);
	}
}
void Space::farm_removeAllSowSeedsDesignations(const Point3D& point)
{
	for(const auto& [faction, fields] : m_farmFields)
	{
		const FarmField* farm = fields.queryGetOne(point).get();
		if(farm != nullptr && designation_has(point, faction, SpaceDesignation::SowSeeds))
				m_area.m_hasFarmFields.getForFaction(faction).removeGivePlantFluidDesignation(m_area, point);
	}
}
bool Space::farm_isSowingSeasonFor(const PlantSpeciesId& species) const
{
	int day = DateTime(m_area.m_simulation.m_step).day;
	return day >= PlantSpecies::getDayOfYearForSowStart(species) && day <= PlantSpecies::getDayOfYearForSowEnd(species);
}
bool Space::farm_contains(const Point3D& point, const FactionId& faction) const
{
	return const_cast<Space*>(this)->farm_get(point, faction) != nullptr;
}
template<typename ShapeT>
void Space::farm_remove(const ShapeT& shape, const FactionId& faction)
{
	m_farmFields[faction].removeAll(shape);
	auto& designations = m_area.m_spaceDesignations.getForFaction(faction);
	designations.maybeUnset(shape, SpaceDesignation::SowSeeds);
	designations.maybeUnset(shape, SpaceDesignation::GivePlantFluid);
	designations.maybeUnset(shape, SpaceDesignation::Harvest);
}
template void Space::farm_remove<Point3D>(const Point3D&, const FactionId&);
template void Space::farm_remove<Cuboid>(const Cuboid&, const FactionId&);
template void Space::farm_remove<CuboidSet>(const CuboidSet&, const FactionId&);