#include "../../lib/doctest.h"
#include "../../engine/temperature.h"
#include "../../engine/area.h"
#include "../../engine/materialType.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/definitions.h"
TEST_CASE("temperature")
{
	DateTime now(12, 150, 1200);
	Simulation simulation(L"", now.toSteps());
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	SUBCASE("solid blocks burn")
	{
		BlockIndex origin = blocks.getIndex({5, 5, 5});
		BlockIndex b1 = blocks.getIndex({5, 5, 6});
		BlockIndex b2 = blocks.getIndex({5, 7, 5});
		BlockIndex b3 = blocks.getIndex({9, 9, 9});
		BlockIndex toBurn = blocks.getIndex({6, 5, 5});
		BlockIndex toNotBurn = blocks.getIndex({4, 5, 5});
		auto& wood = MaterialType::byName("poplar wood");
		auto& marble = MaterialType::byName("marble");
		blocks.solid_set(toBurn, wood, false);
		blocks.solid_set(toNotBurn, marble, false);
		uint32_t temperatureBeforeHeatSource = blocks.temperature_get(origin);
		area.m_hasTemperature.addTemperatureSource(origin, 1000);
		area.m_hasTemperature.applyDeltas();
		REQUIRE(blocks.temperature_get(origin) == temperatureBeforeHeatSource + 1000);
		REQUIRE(blocks.temperature_get(b1) == temperatureBeforeHeatSource + 1000);
		REQUIRE(blocks.temperature_get(b2) == 367);
		REQUIRE(blocks.temperature_get(b3) == temperatureBeforeHeatSource);
		REQUIRE(blocks.temperature_get(toBurn) == temperatureBeforeHeatSource + 1000);
		REQUIRE(blocks.temperature_get(toNotBurn) == temperatureBeforeHeatSource + 1000);
		REQUIRE(blocks.fire_exists(toBurn));
		// Fire exists but the new deltas it has created have not been applied
		REQUIRE(blocks.temperature_get(toBurn) == temperatureBeforeHeatSource + 1000);
		REQUIRE(!blocks.fire_exists(toNotBurn));
		REQUIRE(blocks.temperature_get(toNotBurn) == temperatureBeforeHeatSource + 1000);
		REQUIRE(!simulation.m_eventSchedule.m_data.empty());
		area.m_hasTemperature.applyDeltas();
		REQUIRE(blocks.temperature_get(toBurn) > temperatureBeforeHeatSource + 1000);
		REQUIRE(blocks.temperature_get(toNotBurn) > temperatureBeforeHeatSource + 1000);
	}
	SUBCASE("burnt to ash")
	{
		BlockIndex origin = blocks.getIndex({5, 5, 5});
		BlockIndex toBurn = blocks.getIndex({6, 5, 5});
		auto& wood = MaterialType::byName("poplar wood");
		blocks.solid_set(toBurn, wood, false);
		area.m_hasTemperature.addTemperatureSource(origin, 1000);
		simulation.doStep();
		REQUIRE(blocks.fire_exists(toBurn));
		Fire& fire = blocks.fire_get(toBurn, wood);
		REQUIRE(area.m_fires.containsFireAt(fire, toBurn));
		REQUIRE(fire.m_stage == FireStage::Smouldering);
		simulation.fastForward(wood.burnData->burnStageDuration - 1);
		REQUIRE(fire.m_stage == FireStage::Burning);
		simulation.fastForward(wood.burnData->burnStageDuration);
		REQUIRE(fire.m_stage == FireStage::Flaming);
		simulation.fastForward(wood.burnData->flameStageDuration);
		REQUIRE(fire.m_stage == FireStage::Burning);
		REQUIRE(fire.m_hasPeaked == true);
		simulation.fastForward(wood.burnData->burnStageDuration * Config::fireRampDownPhaseDurationFraction);
		REQUIRE(fire.m_stage == FireStage::Smouldering);
		simulation.fastForward(wood.burnData->burnStageDuration * Config::fireRampDownPhaseDurationFraction);
		REQUIRE(!blocks.fire_exists(toBurn));
		REQUIRE(!blocks.solid_is(toBurn));
	}
}
