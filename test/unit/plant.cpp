#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/datetime.h"
#include "../../engine/plants.h"
TEST_CASE("plant")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation(L"test", DateTime::toSteps(12, 100, 1200));
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	BlockIndex location = blocks.getIndex({5, 5, 2});
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	simulation.m_step = 0;
	blocks.plant_create(location, wheatGrass, 50);
	PlantIndex plant = blocks.plant_get(location);
	REQUIRE(plants.isGrowing(plant));
	REQUIRE(simulation.m_eventSchedule.m_data.contains(wheatGrass.stepsTillFullyGrown / 2));
	REQUIRE(simulation.m_eventSchedule.m_data.contains(wheatGrass.stepsNeedsFluidFrequency));
	REQUIRE(blocks.isExposedToSky(plants.getLocation(plant)));
	REQUIRE(!plants.temperatureEventExists());
	REQUIRE(area.getPlants().getOnSurface().contains(plant));
	simulation.fastForward(wheatGrass.stepsNeedsFluidFrequency);
	REQUIRE(plants.getVolumeFluidRequested(plant) != 0);
	REQUIRE(!plants.isGrowing(plant));
	REQUIRE(plants.getPercentGrown(plant) == 50 + ((float)simulation.m_step / (float)wheatGrass.stepsTillFullyGrown) * 100);
	REQUIRE(simulation.m_eventSchedule.m_data.contains(simulation.m_step + wheatGrass.stepsTillDieWithoutFluid - 1));
	area.m_hasRain.start(water, 1, 100);
	REQUIRE(plants.getVolumeFluidRequested(plant) == 0);
	REQUIRE(plants.isGrowing(plant));
	area.m_hasTemperature.setAmbientSurfaceTemperature(wheatGrass.minimumGrowingTemperature - 1);
	REQUIRE(!plants.isGrowing(plant));
	REQUIRE(plants.temperatureEventExists(plant));
	area.m_hasTemperature.setAmbientSurfaceTemperature(wheatGrass.minimumGrowingTemperature);
	REQUIRE(plants.isGrowing(plant));
	REQUIRE(!plants.temperatureEventExists(plant));
	BlockIndex above = blocks.getBlockAbove(location);
	blocks.solid_set(above, marble, false);
	REQUIRE(!plants.isGrowing(plant));
	blocks.solid_setNot(above);
	REQUIRE(plants.isGrowing(plant));
	plants.die(plant);
}
TEST_CASE("plantFruits")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	uint16_t dayOfYear = wheatGrass.harvestData->dayOfYearToStart;
	Simulation simulation(L"test", DateTime::toSteps(24, dayOfYear - 1, 1200));
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Plants& plants = area.getPlants();
	Blocks& blocks = area.getBlocks();
	BlockIndex location = blocks.getIndex({5, 5, 2});
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	blocks.plant_create(location, wheatGrass, 50);
	PlantIndex plant = blocks.plant_get(location);
	REQUIRE(plants.getQuantityToHarvest(plant) == 0);
	simulation.fastForward(Config::stepsPerHour);
	REQUIRE(plants.getQuantityToHarvest(plant) != 0);
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
	Plants& plants = area.getPlants();
	BlockIndex location = blocks.getIndex({5, 5, 2});
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	blocks.plant_create(location, wheatGrass, 50);
	PlantIndex plant = blocks.plant_get(location);
	REQUIRE(plants.getQuantityToHarvest(plant) != 0);
	simulation.fastForward(Config::stepsPerHour);
	REQUIRE(plants.getQuantityToHarvest(plant) == 0);
}
