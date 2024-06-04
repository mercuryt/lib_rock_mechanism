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
	Blocks& blocks = area.getBlocks();
	BlockIndex location = blocks.getIndex({5, 5, 2});
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	simulation.m_step = 0;
	blocks.plant_create(location, wheatGrass, 50);
	Plant& plant = blocks.plant_get(location);
	REQUIRE(plant.m_growthEvent.exists());
	REQUIRE(simulation.m_eventSchedule.m_data.contains(wheatGrass.stepsTillFullyGrown / 2));
	REQUIRE(simulation.m_eventSchedule.m_data.contains(wheatGrass.stepsNeedsFluidFrequency));
	REQUIRE(blocks.isExposedToSky(plant.m_location));
	REQUIRE(!plant.m_temperatureEvent.exists());
	REQUIRE(area.m_hasPlants.getPlantsOnSurface().contains(&plant));
	simulation.fastForward(wheatGrass.stepsNeedsFluidFrequency);
	REQUIRE(plant.m_volumeFluidRequested != 0);
	REQUIRE(!plant.m_growthEvent.exists());
	REQUIRE(plant.m_percentGrown == 50 + ((float)simulation.m_step / (float)wheatGrass.stepsTillFullyGrown) * 100);
	REQUIRE(simulation.m_eventSchedule.m_data.contains(simulation.m_step + wheatGrass.stepsTillDieWithoutFluid - 1));
	area.m_hasRain.start(water, 1, 100);
	REQUIRE(plant.m_volumeFluidRequested == 0);
	REQUIRE(plant.m_growthEvent.exists());
	area.m_hasTemperature.setAmbientSurfaceTemperature(wheatGrass.minimumGrowingTemperature - 1);
	REQUIRE(!plant.m_growthEvent.exists());
	REQUIRE(plant.m_temperatureEvent.exists());
	area.m_hasTemperature.setAmbientSurfaceTemperature(wheatGrass.minimumGrowingTemperature);
	REQUIRE(plant.m_growthEvent.exists());
	REQUIRE(!plant.m_temperatureEvent.exists());
	BlockIndex above = blocks.getBlockAbove(location);
	blocks.solid_set(above, marble, false);
	REQUIRE(!plant.m_growthEvent.exists());
	blocks.solid_setNot(above);
	REQUIRE(plant.m_growthEvent.exists());
	plant.die();
}
TEST_CASE("plantFruits")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	uint16_t dayOfYear = wheatGrass.harvestData->dayOfYearToStart;
	Simulation simulation(L"test", DateTime::toSteps(24, dayOfYear - 1, 1200));
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	BlockIndex location = blocks.getIndex({5, 5, 2});
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	blocks.plant_create(location, wheatGrass, 50);
	Plant& plant = blocks.plant_get(location);
	REQUIRE(plant.m_quantityToHarvest == 0);
	simulation.fastForward(Config::stepsPerHour);
	REQUIRE(plant.m_quantityToHarvest != 0);
}
TEST_CASE("harvestSeasonEnds")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	uint16_t dayOfYear = wheatGrass.harvestData->dayOfYearToStart;
	Step step = DateTime::toSteps(24, dayOfYear - 1, 1200) + wheatGrass.harvestData->stepsDuration;
	Simulation simulation(L"test", step);
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	BlockIndex location = blocks.getIndex({5, 5, 2});
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	blocks.plant_create(location, wheatGrass, 50);
	Plant& plant = blocks.plant_get(location);
	REQUIRE(plant.m_quantityToHarvest != 0);
	simulation.fastForward(Config::stepsPerHour);
	REQUIRE(plant.m_quantityToHarvest == 0);
}
