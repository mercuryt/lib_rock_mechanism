#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/definitions/materialType.h"
#include "../../engine/definitions/definitions.h"
#include "../../engine/space/space.h"
TEST_CASE("temperature")
{
	DateTime now(12, 150, 1200);
	Simulation simulation("", now.toSteps());
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	SUBCASE("solid blocks burn")
	{
		Point3D origin = Point3D::create(5, 5, 5);
		Point3D b1 = Point3D::create(5, 5, 6);
		Point3D b2 = Point3D::create(5, 7, 5);
		Point3D b3 = Point3D::create(9, 9, 9);
		Point3D b4 = Point3D::create(5, 5, 7);
		Point3D toBurn = Point3D::create(6, 5, 5);
		Point3D toNotBurn = Point3D::create(4, 5, 5);
		auto wood = MaterialType::byName("poplar wood");
		auto marble = MaterialType::byName("marble");
		space.solid_set(toBurn, wood, false);
		space.solid_set(toNotBurn, marble, false);
		Temperature temperatureBeforeHeatSource = space.temperature_get(origin);
		[[maybe_unused]] TemperatureSourceId temperatureSourceId = area.m_hasTemperature.m_sources.addTemperatureSource(area, origin.toCuboid(), TemperatureDelta::create(1000));
		area.m_hasTemperature.doStep(area);
		// ToBurn ignites, adding more temperature on top of the 1000 delta at origin.
		CHECK(space.temperature_get(origin) > temperatureBeforeHeatSource + 1000);
		CHECK(area.m_fires.m_fires.queryAny(toBurn));
		CHECK(!area.m_fires.m_fires.queryAny(toNotBurn));
		CHECK(space.temperature_get(b1) > temperatureBeforeHeatSource + 1000);
		CHECK(space.temperature_get(b2) < temperatureBeforeHeatSource + 262);
		CHECK(space.temperature_get(b4) == space.temperature_get(b2));
		CHECK(space.temperature_get(b4) < space.temperature_get(b1));
		CHECK(space.temperature_get(b3) == temperatureBeforeHeatSource + 20);
		CHECK(space.temperature_get(toBurn) > temperatureBeforeHeatSource + 1000);
		CHECK(space.temperature_get(toNotBurn) == temperatureBeforeHeatSource + 1014);
		CHECK(!simulation.m_eventSchedule.m_data.empty());
	}
	SUBCASE("burnt to ash")
	{
		Point3D origin = Point3D::create(5, 5, 5);
		Point3D toBurn = Point3D::create(6, 5, 5);
		auto wood = MaterialType::byName("poplar wood");
		space.solid_set(toBurn, wood, false);
		[[maybe_unused]] TemperatureSourceId temperatureSourceId = area.m_hasTemperature.m_sources.addTemperatureSource(area, origin.toCuboid(), TemperatureDelta::create(1000));
		CHECK(!area.m_fires.m_fires.queryAny(toBurn));
		simulation.doStep();
		CHECK(area.m_fires.m_fires.queryAny(toBurn));
		FireData fire = area.m_fires.m_fires.queryGetFirst(toBurn);
		CHECK(fire.m_stage == FireStage::Smouldering);
		simulation.fastForward(MaterialType::getBurnStageDuration(wood) - 1);
		fire = area.m_fires.m_fires.queryGetFirst(toBurn);
		CHECK(fire.m_stage == FireStage::Burning);
		simulation.fastForward(MaterialType::getBurnStageDuration(wood));
		fire = area.m_fires.m_fires.queryGetFirst(toBurn);
		CHECK(fire.m_stage == FireStage::Flaming);
		simulation.fastForward(MaterialType::getFlameStageDuration(wood));
		fire = area.m_fires.m_fires.queryGetFirst(toBurn);
		CHECK(fire.m_stage == FireStage::Burning);
		CHECK(fire.m_hasPeaked == true);
		simulation.fastForward(MaterialType::getBurnStageDuration(wood) * Config::fireRampDownPhaseDurationFraction);
		fire = area.m_fires.m_fires.queryGetFirst(toBurn);
		CHECK(fire.m_stage == FireStage::Smouldering);
		simulation.fastForward(MaterialType::getBurnStageDuration(wood) * Config::fireRampDownPhaseDurationFraction);
		fire = area.m_fires.m_fires.queryGetFirst(toBurn);
		CHECK(!area.m_fires.m_fires.queryAny(toBurn));
	}
}
