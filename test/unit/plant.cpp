#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/datetime.h"
#include "../../engine/plants.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/definitions/plantSpecies.h"
TEST_CASE("plant")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static PlantSpeciesId wheatGrass = PlantSpecies::byName("wheat grass");
	static FluidTypeId water = FluidType::byName("water");
	Simulation simulation("test", DateTime::toSteps(12, 100, 1200));
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	Plants& plants = area.getPlants();
	Point3D location = Point3D::create(5, 5, 2);
	CHECK(space.isExposedToSky(location));
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	simulation.m_step = Step::create(0);
	space.plant_create(location, wheatGrass, Percent::create(50));
	PlantIndex plant = space.plant_get(location);
	CHECK(plants.isGrowing(plant));
	CHECK(area.m_eventSchedule.m_data.contains(PlantSpecies::getStepsTillFullyGrown(wheatGrass) / 2));
	CHECK(area.m_eventSchedule.m_data.contains(PlantSpecies::getStepsNeedsFluidFrequency(wheatGrass)));
	CHECK(space.isExposedToSky(plants.getLocation(plant)));
	CHECK(!plants.temperatureEventExists(plant));
	CHECK(plants.isOnSurface(plant));
	simulation.fastForward(PlantSpecies::getStepsNeedsFluidFrequency(wheatGrass));
	CHECK(plants.getVolumeFluidRequested(plant) != 0);
	CHECK(!plants.isGrowing(plant));
	CHECK(plants.getPercentGrown(plant) == 50 + ((float)simulation.m_step.get() / (float)PlantSpecies::getStepsTillFullyGrown(wheatGrass).get()) * 100);
	CHECK(area.m_eventSchedule.m_data.contains(simulation.m_step + PlantSpecies::getStepsTillDieWithoutFluid(wheatGrass) - 1));
	area.m_hasRain.start(water, Percent::create(1), Step::create(100));
	CHECK(plants.getVolumeFluidRequested(plant) == 0);
	CHECK(plants.isGrowing(plant));
	area.m_hasTemperature.setAmbientSurfaceTemperature(PlantSpecies::getMinimumGrowingTemperature(wheatGrass) - 1);
	CHECK(!plants.isGrowing(plant));
	CHECK(plants.temperatureEventExists(plant));
	area.m_hasTemperature.setAmbientSurfaceTemperature(PlantSpecies::getMinimumGrowingTemperature(wheatGrass));
	CHECK(plants.isGrowing(plant));
	CHECK(!plants.temperatureEventExists(plant));
	Point3D above = location.above();
	space.solid_set(above, marble, false);
	CHECK(!plants.isGrowing(plant));
	space.solid_setNot(above);
	CHECK(plants.isGrowing(plant));
	plants.die(plant);
}
TEST_CASE("plantFruits")
{
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static PlantSpeciesId wheatGrass = PlantSpecies::byName("wheat grass");
	int16_t dayOfYear = PlantSpecies::getDayOfYearToStartHarvest(wheatGrass);
	Simulation simulation("test", DateTime::toSteps(24, dayOfYear - 1, 1200));
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Plants& plants = area.getPlants();
	Space& space = area.getSpace();
	Point3D location = Point3D::create(5, 5, 2);
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	space.plant_create(location, wheatGrass, Percent::create(50));
	PlantIndex plant = space.plant_get(location);
	CHECK(plants.getQuantityToHarvest(plant) == 0);
	simulation.fastForward(Config::stepsPerHour);
	CHECK(plants.getQuantityToHarvest(plant) != 0);
}
TEST_CASE("harvestSeasonEnds")
{
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static PlantSpeciesId wheatGrass = PlantSpecies::byName("wheat grass");
	int16_t dayOfYear = PlantSpecies::getDayOfYearToStartHarvest(wheatGrass);
	Step step = DateTime::toSteps(24, dayOfYear - 1, 1200) + PlantSpecies::getStepsDurationHarvest(wheatGrass);
	Simulation simulation("test", step);
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	Plants& plants = area.getPlants();
	Point3D location = Point3D::create(5, 5, 2);
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	space.plant_create(location, wheatGrass, Percent::create(50));
	PlantIndex plant = space.plant_get(location);
	CHECK(plants.getQuantityToHarvest(plant) != 0);
	simulation.fastForward(Config::stepsPerHour);
	CHECK(plants.getQuantityToHarvest(plant) == 0);
}
