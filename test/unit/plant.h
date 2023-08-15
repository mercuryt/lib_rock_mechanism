#include "../../lib/doctest.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"
TEST_CASE("plant")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const FluidType& water = FluidType::byName("water");
	simulation::init();
	Area area(10,10,10, 280);
	eventSchedule::clear();
	area.setHour(12, 100);
	Block& location = area.m_blocks[5][5][2];
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	areaBuilderUtil::setSolidLayer(area, 1, dirt);
	simulation::step = 0;
	location.m_hasPlant.addPlant(wheatGrass, 50);
	Plant& plant = location.m_hasPlant.get();
	CHECK(plant.m_growthEvent.exists());
	CHECK(eventSchedule::data.contains(wheatGrass.stepsTillFullyGrown / 2));
	CHECK(eventSchedule::data.contains(wheatGrass.stepsNeedsFluidFrequency));
	CHECK(plant.m_location.m_exposedToSky);
	CHECK(!plant.m_temperatureEvent.exists());
	CHECK(area.m_hasPlants.getPlantsOnSurface().contains(&plant));
	simulation::step = wheatGrass.stepsNeedsFluidFrequency;
	eventSchedule::execute(simulation::step);
	CHECK(plant.m_volumeFluidRequested != 0);
	CHECK(!plant.m_growthEvent.exists());
	CHECK(plant.m_percentGrown == 50 + ((float)simulation::step / (float)wheatGrass.stepsTillFullyGrown) * 100);
	CHECK(eventSchedule::data.contains(simulation::step + wheatGrass.stepsTillDieWithoutFluid));
	area.m_hasRain.start(water, 1, 100);
	CHECK(plant.m_volumeFluidRequested == 0);
	CHECK(plant.m_growthEvent.exists());
	area.m_areaHasTemperature.setAmbientSurfaceTemperature(wheatGrass.minimumGrowingTemperature - 1);
	CHECK(!plant.m_growthEvent.exists());
	CHECK(plant.m_temperatureEvent.exists());
	area.m_areaHasTemperature.setAmbientSurfaceTemperature(wheatGrass.minimumGrowingTemperature);
	CHECK(plant.m_growthEvent.exists());
	CHECK(!plant.m_temperatureEvent.exists());
	Block& above = *location.m_adjacents[5];
	above.setSolid(marble);
	CHECK(!plant.m_growthEvent.exists());
	above.setNotSolid();
	CHECK(plant.m_growthEvent.exists());
	uint32_t dayOfYear = wheatGrass.harvestData->dayOfYearToStart;
	area.setDayOfYear(dayOfYear);
	CHECK(plant.m_quantityToHarvest != 0);
	simulation::step = simulation::step + plant.m_plantSpecies.harvestData->stepsDuration;
	eventSchedule::execute(simulation::step);
	CHECK(plant.m_quantityToHarvest == 0);
}
