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
#include "hasShapes.hpp"
#include "actors/actors.h"
#include <algorithm>
bool HasFarmFieldsForFaction::hasGivePlantsFluidDesignations() const
{
	return !m_pointsWithPlantsNeedingFluid.empty();
}
PlantIndex HasFarmFieldsForFaction::getHighestPriorityPlantForGiveFluid(Area& area)
{
	assert(!m_pointsWithPlantsNeedingFluid.empty());
	Plants& plants = area.getPlants();
	Space& space =  area.getSpace();
	m_pointsWithPlantsNeedingFluid.eraseIf([&](const Point3D& point) { return !space.plant_exists(point); });
	if(!m_plantsNeedingFluidIsSorted)
	{
		m_pointsWithPlantsNeedingFluid.sort([&](const Point3D& a, const Point3D& b){
		       	return plants.getStepAtWhichPlantWillDieFromLackOfFluid(space.plant_get(a)) < plants.getStepAtWhichPlantWillDieFromLackOfFluid(space.plant_get(b));
		});
		m_plantsNeedingFluidIsSorted = true;
	}
	PlantIndex output = space.plant_get(m_pointsWithPlantsNeedingFluid.front());
	if(plants.getStepAtWhichPlantWillDieFromLackOfFluid(output) - area.m_simulation.m_step > Config::stepsTillDiePlantPriorityOveride)
		return PlantIndex::null();
	return output;
}
bool HasFarmFieldsForFaction::hasSowSeedsDesignations() const
{
	return !m_pointsNeedingSeedsSewn.empty();
}
bool HasFarmFieldsForFaction::hasHarvestDesignations() const
{
	return !m_pointsWithPlantsToHarvest.empty();
}
PlantSpeciesId HasFarmFieldsForFaction::getPlantSpeciesFor(const Point3D& point) const
{
	for(const FarmField& farmField : m_farmFields)
		if(farmField.points.contains(point))
			return farmField.plantSpecies;
	std::unreachable();
	return PlantSpecies::byName("");
}
void HasFarmFieldsForFaction::addGivePlantFluidDesignation(Area& area, const Point3D& location)
{
	assert(!m_pointsWithPlantsNeedingFluid.contains(location));
	m_plantsNeedingFluidIsSorted = false;
	m_pointsWithPlantsNeedingFluid.insert(location);
	 area.getSpace().designation_set(location, m_faction, SpaceDesignation::GivePlantFluid);
}
void HasFarmFieldsForFaction::removeGivePlantFluidDesignation(Area& area, const Point3D& location)
{
	assert(m_pointsWithPlantsNeedingFluid.contains(location));
	m_pointsWithPlantsNeedingFluid.erase(location);
	 area.getSpace().designation_unset(location, m_faction, SpaceDesignation::GivePlantFluid);
}
void HasFarmFieldsForFaction::addSowSeedsDesignation(Area& area, const Point3D& point)
{
	assert(!m_pointsNeedingSeedsSewn.contains(point));
	m_pointsNeedingSeedsSewn.insert(point);
	 area.getSpace().designation_set(point, m_faction, SpaceDesignation::SowSeeds);
}
void HasFarmFieldsForFaction::removeSowSeedsDesignation(Area& area, const Point3D& point)
{
	assert(m_pointsNeedingSeedsSewn.contains(point));
	m_pointsNeedingSeedsSewn.erase(point);
	 area.getSpace().designation_unset(point, m_faction, SpaceDesignation::SowSeeds);
}
void HasFarmFieldsForFaction::addHarvestDesignation(Area& area, const PlantIndex& plant)
{
	Point3D location = area.getPlants().getLocation(plant);
	assert(!m_pointsWithPlantsToHarvest.contains(location));
	m_pointsWithPlantsToHarvest.insert(location);
	 area.getSpace().designation_set(location, m_faction, SpaceDesignation::Harvest);
}
void HasFarmFieldsForFaction::removeHarvestDesignation(Area& area, const PlantIndex& plant)
{
	Point3D location = area.getPlants().getLocation(plant);
	assert(m_pointsWithPlantsToHarvest.contains(location));
	m_pointsWithPlantsToHarvest.erase(location);
	 area.getSpace().designation_unset(location, m_faction, SpaceDesignation::Harvest);
}
void HasFarmFieldsForFaction::setDayOfYear(Area& area, uint32_t dayOfYear)
{
	//TODO: Add sow designation when blocking item or plant is removed.
	for(FarmField& farmField : m_farmFields)
		if(farmField.plantSpecies.exists())
		{
			if(PlantSpecies::getDayOfYearForSowStart(farmField.plantSpecies) == dayOfYear)
			{
				Space& space =  area.getSpace();
				for(const Point3D& point : farmField.points)
					if(space.plant_canGrowHereAtSomePointToday(point, farmField.plantSpecies))
						addSowSeedsDesignation(area, point);
				farmField.timeToSow = true;
			}
			if(PlantSpecies::getDayOfYearForSowEnd(farmField.plantSpecies) == dayOfYear)
			{
				assert(farmField.timeToSow);
				farmField.timeToSow = false;
				for(const Point3D& point : farmField.points)
					if(m_pointsNeedingSeedsSewn.contains(point))
						removeSowSeedsDesignation(area, point);
			}
		}
}
FarmField& HasFarmFieldsForFaction::create(Area& area, SmallSet<Point3D>&& space)
{
	m_farmFields.emplace(std::move(space));
	auto& field = m_farmFields.back();
	for(const Point3D& point : field.points)
		 area.getSpace().farm_insert(point, m_faction, field);
	return field;
}
FarmField& HasFarmFieldsForFaction::create(Area& area, const SmallSet<Point3D>& space)
{
	SmallSet<Point3D> adapter;
	adapter.maybeInsertAll(space.begin(), space.end());
	return create(area, std::move(adapter));
}
FarmField& HasFarmFieldsForFaction::create(Area& area, const Cuboid& cuboid)
{
	auto set = cuboid.toSet();
	return create(area, std::move(set));
}
FarmField& HasFarmFieldsForFaction::create(Area& area, const CuboidSet& cuboidSet)
{
	auto set = cuboidSet.toPointSet();
	return create(area, std::move(set));
}
void HasFarmFieldsForFaction::extend(Area& area, FarmField& farmField, SmallSet<Point3D>& space)
{
	farmField.points.maybeInsertAll(space.begin(), space.end());
	if(farmField.plantSpecies.exists())
		designatePoints(area, farmField, space);
	for(const Point3D& point : space)
		 area.getSpace().farm_insert(point, m_faction, farmField);
}
void HasFarmFieldsForFaction::setSpecies(Area& area, FarmField& farmField, const PlantSpeciesId& plantSpecies)
{
	assert(farmField.plantSpecies.empty());
	farmField.plantSpecies = plantSpecies;
	int32_t day = DateTime(area.m_simulation.m_step).day;
	if(day >= PlantSpecies::getDayOfYearForSowStart(plantSpecies) && day <= PlantSpecies::getDayOfYearForSowEnd(plantSpecies))
	{
		farmField.timeToSow = true;
		designatePoints(area, farmField, farmField.points);
	}
}
void HasFarmFieldsForFaction::designatePoints(Area& area, FarmField& farmField, SmallSet<Point3D>& designated)
{
	// Designate for sow if the season is right.
	if(farmField.timeToSow)
		for(const Point3D& point : designated)
			addSowSeedsDesignation(area, point);
	Space& space =  area.getSpace();
	Plants& plants = area.getPlants();
	for(const Point3D& point : designated)
		if(space.plant_exists(point))
		{
			const PlantIndex& plant = space.plant_get(point);
			// Designate plants already existing for fluid if the species is right and they need fluid.
			if(plants.getSpecies(plant) == farmField.plantSpecies && plants.getVolumeFluidRequested(plant) != 0)
				addGivePlantFluidDesignation(area, point);
			if(plants.readyToHarvest(plant))
				addHarvestDesignation(area, plant);
		}
}
void HasFarmFieldsForFaction::clearSpecies(Area& area, FarmField& farmField)
{
	assert(farmField.plantSpecies.exists());
	farmField.plantSpecies.clear();
	undesignatePoints(area, farmField.points);
}
void HasFarmFieldsForFaction::shrink(Area& area, FarmField& farmField, SmallSet<Point3D>& undesignated)
{
	undesignatePoints(area, undesignated);
	Space& space =  area.getSpace();
	for(const Point3D& point : undesignated)
		space.farm_remove(point, m_faction);
	farmField.points.eraseIf([&](const Point3D& point){ return undesignated.contains(point); });
	if(farmField.points.empty())
		m_farmFields.erase(farmField);
}
void HasFarmFieldsForFaction::remove(Area& area, FarmField& farmField)
{
	shrink(area, farmField, farmField.points);
}
void HasFarmFieldsForFaction::undesignatePoints(Area& area, SmallSet<Point3D>& undesignated)
{
	Space& space =  area.getSpace();
	for(const Point3D& point : undesignated)
	{
		if(space.plant_exists(point))
		{
			const PlantIndex& plant = space.plant_get(point);
			if(space.designation_has(point, m_faction, SpaceDesignation::GivePlantFluid))
				area.m_hasFarmFields.getForFaction(m_faction).removeGivePlantFluidDesignation(area, point);
			if(space.designation_has(point, m_faction, SpaceDesignation::Harvest))
				area.m_hasFarmFields.getForFaction(m_faction).removeHarvestDesignation(area, plant);
		}
		else if(space.designation_has(point, m_faction, SpaceDesignation::SowSeeds))
				area.m_hasFarmFields.getForFaction(m_faction).removeSowSeedsDesignation(area, point);
	}
}
void HasFarmFieldsForFaction::setPointData(Area& area)
{
	Space& space =  area.getSpace();
	for(FarmField& field : m_farmFields)
		for(const Point3D& point : field.points)
			space.farm_insert(point, m_faction, field);
}
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
	for(auto& [faction, data] : m_data)
		data.setPointData(m_area);
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
PlantIndex AreaHasFarmFields::getHighestPriorityPlantForGiveFluid(Area& area, const FactionId& faction)
{
	if(!m_data.contains(faction))
		return PlantIndex::null();
	return m_data[faction].getHighestPriorityPlantForGiveFluid(area);
}
void AreaHasFarmFields::removeAllSowSeedsDesignations(const Point3D& point)
{
	Space& space = m_area.getSpace();
	for(auto& pair : m_data)
		if(space.designation_has(point, pair.first, SpaceDesignation::SowSeeds))
			pair.second.removeSowSeedsDesignation(m_area, point);
}
void AreaHasFarmFields::setDayOfYear(uint32_t dayOfYear)
{
	for(auto& pair : m_data)
		pair.second.setDayOfYear(m_area, dayOfYear);
}
bool AreaHasFarmFields::hasGivePlantsFluidDesignations(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].hasGivePlantsFluidDesignations();
}
bool AreaHasFarmFields::hasSowSeedsDesignations(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].hasSowSeedsDesignations();
}
bool AreaHasFarmFields::hasHarvestDesignations(const FactionId& faction) const
{
	if(!m_data.contains(faction))
		return false;
	return m_data[faction].hasHarvestDesignations();
}
PlantSpeciesId AreaHasFarmFields::getPlantSpeciesFor(const FactionId& faction, const Point3D& location) const
{
	assert(m_data.contains(faction));
	return m_data[faction].getPlantSpeciesFor(location);
}
