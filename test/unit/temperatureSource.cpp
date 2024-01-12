#include "../../lib/doctest.h"
#include "../../engine/temperature.h"
#include "../../engine/area.h"
#include "../../engine/materialType.h"
#include "../../engine/simulation.h"
#include "../../engine/block.h"
#include "../../engine/definitions.h"
TEST_CASE("temperature")
{
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	SUBCASE("solid blocks burn")
	{
		Block& origin = area.getBlock(5, 5, 5);
		Block& b1 = area.getBlock(5, 5, 6);
		Block& b2 = area.getBlock(5, 7, 5);
		Block& b3 = area.getBlock(9, 9, 9);
		Block& toBurn = area.getBlock(6, 5, 5);
		Block& toNotBurn = area.getBlock(4, 5, 5);
		auto& wood = MaterialType::byName("poplar wood");
		auto& marble = MaterialType::byName("marble");
		toBurn.setSolid(wood);
		toNotBurn.setSolid(marble);
		uint32_t temperatureBeforeHeatSource = origin.m_blockHasTemperature.get();
		area.m_hasTemperature.addTemperatureSource(origin, 1000);
		area.m_hasTemperature.applyDeltas();
		REQUIRE(origin.m_blockHasTemperature.get() == temperatureBeforeHeatSource + 1000);
		REQUIRE(b1.m_blockHasTemperature.get() == temperatureBeforeHeatSource + 1000);
		REQUIRE(b2.m_blockHasTemperature.get() == 367);
		REQUIRE(b3.m_blockHasTemperature.get() == temperatureBeforeHeatSource);
		REQUIRE(toBurn.m_blockHasTemperature.get() == temperatureBeforeHeatSource + 1000);
		REQUIRE(toNotBurn.m_blockHasTemperature.get() == temperatureBeforeHeatSource + 1000);
		REQUIRE(toBurn.m_fires != nullptr);
		// Fire exists but the new deltas it has created have not been applied
		REQUIRE(toBurn.m_blockHasTemperature.get() == temperatureBeforeHeatSource + 1000);
		REQUIRE(toNotBurn.m_fires == nullptr);
		REQUIRE(toNotBurn.m_blockHasTemperature.get() == temperatureBeforeHeatSource + 1000);
		REQUIRE(!simulation.m_eventSchedule.m_data.empty());
		area.m_hasTemperature.applyDeltas();
		REQUIRE(toBurn.m_blockHasTemperature.get() > temperatureBeforeHeatSource + 1000);
		REQUIRE(toNotBurn.m_blockHasTemperature.get() > temperatureBeforeHeatSource + 1000);
	}
	SUBCASE("burnt to ash")
	{
		Block& origin = area.getBlock(5, 5, 5);
		Block& toBurn = area.getBlock(6, 5, 5);
		auto& wood = MaterialType::byName("poplar wood");
		toBurn.setSolid(wood);
		area.m_hasTemperature.addTemperatureSource(origin, 1000);
		simulation.doStep();
		REQUIRE(toBurn.m_fires != nullptr);
		Fire& fire = toBurn.m_fires->at(&wood);
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
		REQUIRE(toBurn.m_fires == nullptr);
		REQUIRE(!toBurn.isSolid());
	}
}
