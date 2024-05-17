#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/areaBuilderUtil.h"
#include "datetime.h"
TEST_CASE("plant")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation(L"test", DateTime::toSteps(12, 100, 1200));
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Block& location = area.getBlock(5, 5, 2);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	simulation.m_step = 0;
	location.m_hasPlant.createPlant(wheatGrass, 50);
	Plant& plant = location.m_hasPlant.get();
	CHECK(plant.m_growthEvent.exists());
	CHECK(simulation.m_eventSchedule.m_data.contains(wheatGrass.stepsTillFullyGrown / 2));
	CHECK(simulation.m_eventSchedule.m_data.contains(wheatGrass.stepsNeedsFluidFrequency));
	CHECK(plant.m_location->m_exposedToSky);
	CHECK(!plant.m_temperatureEvent.exists());
	CHECK(area.m_hasPlants.getPlantsOnSurface().contains(&plant));
	simulation.fastForward(wheatGrass.stepsNeedsFluidFrequency);
	CHECK(plant.m_volumeFluidRequested != 0);
	CHECK(!plant.m_growthEvent.exists());
	CHECK(plant.m_percentGrown == 50 + ((float)simulation.m_step / (float)wheatGrass.stepsTillFullyGrown) * 100);
	CHECK(simulation.m_eventSchedule.m_data.contains(simulation.m_step + wheatGrass.stepsTillDieWithoutFluid - 1));
	area.m_hasRain.start(water, 1, 100);
	CHECK(plant.m_volumeFluidRequested == 0);
	CHECK(plant.m_growthEvent.exists());
	area.m_hasTemperature.setAmbientSurfaceTemperature(wheatGrass.minimumGrowingTemperature - 1);
	CHECK(!plant.m_growthEvent.exists());
	CHECK(plant.m_temperatureEvent.exists());
	area.m_hasTemperature.setAmbientSurfaceTemperature(wheatGrass.minimumGrowingTemperature);
	CHECK(plant.m_growthEvent.exists());
	CHECK(!plant.m_temperatureEvent.exists());
	Block& above = *location.m_adjacents[5];
	above.setSolid(marble);
	CHECK(!plant.m_growthEvent.exists());
	above.setNotSolid();
	CHECK(plant.m_growthEvent.exists());
	plant.die();
}
TEST_CASE("plantFruits")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	uint16_t dayOfYear = wheatGrass.harvestData->dayOfYearToStart;
	Simulation simulation(L"test", DateTime::toSteps(24, dayOfYear - 1, 1200));
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Block& location = area.getBlock(5, 5, 2);
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	location.m_hasPlant.createPlant(wheatGrass, 50);
	Plant& plant = location.m_hasPlant.get();
	CHECK(plant.m_quantityToHarvest == 0);
	simulation.fastForward(Config::stepsPerHour);
	CHECK(plant.m_quantityToHarvest != 0);
}
TEST_CASE("harvestSeasonEnds")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	uint16_t dayOfYear = wheatGrass.harvestData->dayOfYearToStart;
	Step step = DateTime::toSteps(24, dayOfYear - 1, 1200) + wheatGrass.harvestData->stepsDuration;
	Simulation simulation(L"test", step);
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Block& location = area.getBlock(5, 5, 2);
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	location.m_hasPlant.createPlant(wheatGrass, 50);
	Plant& plant = location.m_hasPlant.get();
	CHECK(plant.m_quantityToHarvest != 0);
	simulation.fastForward(Config::stepsPerHour);
	CHECK(plant.m_quantityToHarvest == 0);
}
