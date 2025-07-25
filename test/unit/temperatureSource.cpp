#include "../../lib/doctest.h"
#include "../../engine/temperature.h"
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
		area.m_hasTemperature.addTemperatureSource(origin, TemperatureDelta::create(1000));
		area.m_hasTemperature.applyDeltas();
		CHECK(space.temperature_get(origin) == temperatureBeforeHeatSource + 1000);
		CHECK(space.temperature_get(b1) == temperatureBeforeHeatSource + 1000);
		CHECK(space.temperature_get(b2) == 462);
		CHECK(space.temperature_get(b4) == space.temperature_get(b2));
		CHECK(space.temperature_get(b4) < space.temperature_get(b1));
		CHECK(space.temperature_get(b3) == temperatureBeforeHeatSource);
		CHECK(space.temperature_get(toBurn) == temperatureBeforeHeatSource + 1000);
		CHECK(space.temperature_get(toNotBurn) == temperatureBeforeHeatSource + 1000);
		CHECK(space.fire_exists(toBurn));
		// Fire exists but the new deltas it has created have not been applied
		CHECK(space.temperature_get(toBurn) == temperatureBeforeHeatSource + 1000);
		CHECK(!space.fire_exists(toNotBurn));
		CHECK(space.temperature_get(toNotBurn) == temperatureBeforeHeatSource + 1000);
		CHECK(!simulation.m_eventSchedule.m_data.empty());
		area.m_hasTemperature.applyDeltas();
		CHECK(space.temperature_get(toBurn) > temperatureBeforeHeatSource + 1000);
		CHECK(space.temperature_get(toNotBurn) > temperatureBeforeHeatSource + 1000);
	}
	SUBCASE("burnt to ash")
	{
		Point3D origin = Point3D::create(5, 5, 5);
		Point3D toBurn = Point3D::create(6, 5, 5);
		auto wood = MaterialType::byName("poplar wood");
		space.solid_set(toBurn, wood, false);
		area.m_hasTemperature.addTemperatureSource(origin, TemperatureDelta::create(1000));
		simulation.doStep();
		CHECK(space.fire_exists(toBurn));
		Fire& fire = space.fire_get(toBurn, wood);
		CHECK(area.m_fires.containsFireAt(fire, toBurn));
		CHECK(fire.m_stage == FireStage::Smouldering);
		simulation.fastForward(MaterialType::getBurnStageDuration(wood) - 1);
		CHECK(fire.m_stage == FireStage::Burning);
		simulation.fastForward(MaterialType::getBurnStageDuration(wood));
		CHECK(fire.m_stage == FireStage::Flaming);
		simulation.fastForward(MaterialType::getFlameStageDuration(wood));
		CHECK(fire.m_stage == FireStage::Burning);
		CHECK(fire.m_hasPeaked == true);
		simulation.fastForward(MaterialType::getBurnStageDuration(wood) * Config::fireRampDownPhaseDurationFraction);
		CHECK(fire.m_stage == FireStage::Smouldering);
		simulation.fastForward(MaterialType::getBurnStageDuration(wood) * Config::fireRampDownPhaseDurationFraction);
		CHECK(!space.fire_exists(toBurn));
		CHECK(!space.solid_is(toBurn));
	}
}
