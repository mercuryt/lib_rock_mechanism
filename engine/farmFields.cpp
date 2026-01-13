#include "farmFields.h"
#include "area/area.h"
#include "datetime.h"
#include "deserializeDishonorCallbacks.h"
#include "designations.h"
#include "numericTypes/index.h"
#include "definitions/plantSpecies.h"
#include "simulation/simulation.h"
#include "plants.h"
#include "space/space.h"
#include "hasShapes.h"
#include "actors/actors.h"
#include <algorithm>
bool HasFarmFieldsForFaction::hasGivePlantsFluidDesignations() const
{
	return !m_plantsNeedingFluid.empty();
}
bool HasFarmFieldsForFaction::hasHarvestDesignations(Area& area) const
{
	return area.getSpace().designation_anyForFaction(m_faction, SpaceDesignation::Harvest);
}
bool HasFarmFieldsForFaction::hasSowSeedsDesignations(Area& area) const
{
	return area.getSpace().designation_anyForFaction(m_faction, SpaceDesignation::SowSeeds);
}
PlantIndex HasFarmFieldsForFaction::getHighestPriorityPlantForGiveFluid()
{
	assert(!m_plantsNeedingFluid.empty());
	if(!m_plantsNeedingFluidIsSorted)
	{
		std::ranges::sort(m_plantsNeedingFluid.m_data, std::less{}, &std::pair<Step, PlantIndex>::first);
		m_plantsNeedingFluidIsSorted = true;
	}
	return m_plantsNeedingFluid.front().second;
}
PlantSpeciesId HasFarmFieldsForFaction::getPlantSpeciesFor(const Point3D& point) const
{
	for(const FarmField& farmField : m_farmFields)
		if(farmField.m_cuboids.contains(point))
			return farmField.m_plantSpecies;
	std::unreachable();
	return PlantSpecies::byName("");
}
void HasFarmFieldsForFaction::addHarvestDesignation(Area& area, const PlantIndex& plant)
{
	Point3D location = area.getPlants().getLocation(plant);
	area.getSpace().designation_set(location, m_faction, SpaceDesignation::Harvest);
}
void HasFarmFieldsForFaction::removeHarvestDesignation(Area& area, const PlantIndex& plant)
{
	Point3D location = area.getPlants().getLocation(plant);
	area.getSpace().designation_unset(location, m_faction, SpaceDesignation::Harvest);
}
void HasFarmFieldsForFaction::setDayOfYear(Area& area, int16_t dayOfYear)
{
	//TODO: Add sow designation when blocking item or plant is removed.
	for(FarmField& farmField : m_farmFields)
		if(farmField.m_plantSpecies.exists())
		{
			if(PlantSpecies::getDayOfYearForSowStart(farmField.m_plantSpecies) == dayOfYear)
			{
				Space& space = area.getSpace();
				if(space.plant_canGrowHereAtSomePointToday(farmField.m_cuboids.front().m_high, farmField.m_plantSpecies))
					addSowSeedsDesignation(area, farmField.m_cuboids);
				farmField.m_timeToSow = true;
			}
			if(PlantSpecies::getDayOfYearForSowEnd(farmField.m_plantSpecies) == dayOfYear)
			{
				assert(farmField.m_timeToSow);
				farmField.m_timeToSow = false;
				maybeRemoveSowSeedsDesignation(area, farmField.m_cuboids);
			}
		}
}
FarmField& HasFarmFieldsForFaction::create(Area& area, CuboidSet&& space)
{
	m_farmFields.emplace(std::move(space));
	auto& field = m_farmFields.back();
	area.getSpace().farm_insert(field.m_cuboids, m_faction, field);
	return field;
}
FarmField& HasFarmFieldsForFaction::create(Area& area, const CuboidSet& newCuboids)
{
	CuboidSet adapter;
	adapter.addSet(newCuboids);
	return create(area, std::move(adapter));
}
FarmField& HasFarmFieldsForFaction::create(Area& area, const Cuboid& cuboid) { return create(area, CuboidSet::create(cuboid)); }
void HasFarmFieldsForFaction::extend(Area& area, FarmField& farmField, CuboidSet& cuboids)
{
	farmField.m_cuboids.addSet(cuboids);
	if(farmField.m_plantSpecies.exists())
		designatePoints(area, farmField, cuboids);
	area.getSpace().farm_insert(cuboids, m_faction, farmField);
}
void HasFarmFieldsForFaction::setSpecies(Area& area, FarmField& farmField, const PlantSpeciesId& plantSpecies)
{
	assert(farmField.m_plantSpecies.empty());
	farmField.m_plantSpecies = plantSpecies;
	int16_t day = DateTime(area.m_simulation.m_step).day;
	if(day >= PlantSpecies::getDayOfYearForSowStart(plantSpecies) && day <= PlantSpecies::getDayOfYearForSowEnd(plantSpecies))
	{
		farmField.m_timeToSow = true;
		designatePoints(area, farmField, farmField.m_cuboids);
	}
}
void HasFarmFieldsForFaction::designatePoints(Area& area, FarmField& farmField, CuboidSet& designated)
{
	// Designate for sow if the season is right.
	if(farmField.m_timeToSow)
		addSowSeedsDesignation(area, designated);
	Space& space = area.getSpace();
	Plants& plants = area.getPlants();
	for(const PlantIndex& plant : space.plant_getAll(designated))
	{
		if(plants.getSpecies(plant) == farmField.m_plantSpecies && plants.getVolumeFluidRequested(plant) != 0)
			addGivePlantFluidDesignation(area, plants.getLocation(plant));
		if(plants.readyToHarvest(plant))
			addHarvestDesignation(area, plant);
	}
}
void HasFarmFieldsForFaction::clearSpecies(Area& area, FarmField& farmField)
{
	assert(farmField.m_plantSpecies.exists());
	farmField.m_plantSpecies.clear();
	undesignatePoints(area, farmField.m_cuboids);
}
void HasFarmFieldsForFaction::shrink(Area& area, FarmField& farmField, CuboidSet& undesignated)
{
	undesignatePoints(area, undesignated);
	Space& space = area.getSpace();
	space.farm_remove(undesignated, m_faction);
	farmField.m_cuboids.maybeRemoveAll(undesignated);
	if(farmField.m_cuboids.empty())
		m_farmFields.erase(farmField);
}
void HasFarmFieldsForFaction::remove(Area& area, FarmField& farmField)
{
	shrink(area, farmField, farmField.m_cuboids);
}
void HasFarmFieldsForFaction::undesignatePoints(Area& area, CuboidSet& undesignated)
{
	area.m_hasFarmFields.getForFaction(m_faction).maybeRemoveSowSeedsDesignation(area, undesignated);
	area.m_hasFarmFields.getForFaction(m_faction).maybeRemoveHarvestDesignation(area, undesignated);
	area.m_hasFarmFields.getForFaction(m_faction).maybeRemoveGivePlantFluidDesignation(area, undesignated);
}
void HasFarmFieldsForFaction::setPointData(Area& area)
{
	Space& space = area.getSpace();
	for(FarmField& field : m_farmFields)
		space.farm_insert(field.m_cuboids, m_faction, field);
}
template<typename ShapeT>
void HasFarmFieldsForFaction::addGivePlantFluidDesignation(Area& area, const ShapeT& shape)
{
	m_plantsNeedingFluidIsSorted = false;
	const Plants& plants = area.getPlants();
	for(const PlantIndex& plant : area.getSpace().plant_getAll(shape))
		m_plantsNeedingFluid.insert({plants.getStepAtWhichPlantWillDieFromLackOfFluid(plant), plant});
	area.getSpace().designation_set(shape, m_faction, SpaceDesignation::GivePlantFluid);
}
template void HasFarmFieldsForFaction::addGivePlantFluidDesignation<Point3D>(Area& area, const Point3D& shape);
template void HasFarmFieldsForFaction::addGivePlantFluidDesignation<Cuboid>(Area& area, const Cuboid& shape);
template void HasFarmFieldsForFaction::addGivePlantFluidDesignation<CuboidSet>(Area& area, const CuboidSet& shape);
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
	for(auto& [faction, fieldData] : m_data)
		fieldData.setPointData(m_area);
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
PlantIndex AreaHasFarmFields::getHighestPriorityPlantForGiveFluid(const FactionId& faction)
{
	if(!m_data.contains(faction))
		return PlantIndex::null();
	return m_data[faction].getHighestPriorityPlantForGiveFluid();
}
void AreaHasFarmFields::removeAllSowSeedsDesignations(const Point3D& point)
{
	Space& space = m_area.getSpace();
	for(auto& pair : m_data)
		if(space.designation_has(point, pair.first, SpaceDesignation::SowSeeds))
			pair.second.removeSowSeedsDesignation(m_area, point);
}
void AreaHasFarmFields::setDayOfYear(int16_t dayOfYear)
{
	for(auto& pair : m_data)
		pair.second.setDayOfYear(m_area, dayOfYear);
}
bool AreaHasFarmFields::hasGivePlantsFluidDesignations(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_area.m_spaceDesignations.getForFaction(faction).any(SpaceDesignation::GivePlantFluid);
}
bool AreaHasFarmFields::hasSowSeedsDesignations(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_area.m_spaceDesignations.getForFaction(faction).any(SpaceDesignation::SowSeeds);
}
bool AreaHasFarmFields::hasHarvestDesignations(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_area.m_spaceDesignations.getForFaction(faction).any(SpaceDesignation::Harvest);
}
PlantSpeciesId AreaHasFarmFields::getPlantSpeciesFor(const FactionId& faction, const Point3D& location) const
{
	assert(m_data.contains(faction));
	return m_data[faction].getPlantSpeciesFor(location);
}