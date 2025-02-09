#include "../../lib/doctest.h"
#include "../../engine/temperature.h"
#include "../../engine/area.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/materialType.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/definitions.h"
#include "../../engine/blocks/blocks.h"
TEST_CASE("temperature")
{
	DateTime now(12, 150, 1200);
	Simulation simulation(L"", now.toSteps());
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	SUBCASE("solid blocks burn")
	{
		BlockIndex origin = blocks.getIndex_i(5, 5, 5);
		BlockIndex b1 = blocks.getIndex_i(5, 5, 6);
		BlockIndex b2 = blocks.getIndex_i(5, 7, 5);
		BlockIndex b3 = blocks.getIndex_i(9, 9, 9);
		BlockIndex toBurn = blocks.getIndex_i(6, 5, 5);
		BlockIndex toNotBurn = blocks.getIndex_i(4, 5, 5);
		auto wood = MaterialType::byName(L"poplar wood");
		auto marble = MaterialType::byName(L"marble");
		blocks.solid_set(toBurn, wood, false);
		blocks.solid_set(toNotBurn, marble, false);
		Temperature temperatureBeforeHeatSource = blocks.temperature_get(origin);
		area.m_hasTemperature.addTemperatureSource(origin, TemperatureDelta::create(1000));
		area.m_hasTemperature.applyDeltas();
		CHECK(blocks.temperature_get(origin) == temperatureBeforeHeatSource + 1000);
		CHECK(blocks.temperature_get(b1) == temperatureBeforeHeatSource + 1000);
		CHECK(blocks.temperature_get(b2) == 386);
		CHECK(blocks.temperature_get(b3) == temperatureBeforeHeatSource);
		CHECK(blocks.temperature_get(toBurn) == temperatureBeforeHeatSource + 1000);
		CHECK(blocks.temperature_get(toNotBurn) == temperatureBeforeHeatSource + 1000);
		CHECK(blocks.fire_exists(toBurn));
		// Fire exists but the new deltas it has created have not been applied
		CHECK(blocks.temperature_get(toBurn) == temperatureBeforeHeatSource + 1000);
		CHECK(!blocks.fire_exists(toNotBurn));
		CHECK(blocks.temperature_get(toNotBurn) == temperatureBeforeHeatSource + 1000);
		CHECK(!simulation.m_eventSchedule.m_data.empty());
		area.m_hasTemperature.applyDeltas();
		CHECK(blocks.temperature_get(toBurn) > temperatureBeforeHeatSource + 1000);
		CHECK(blocks.temperature_get(toNotBurn) > temperatureBeforeHeatSource + 1000);
	}
	SUBCASE("burnt to ash")
	{
		BlockIndex origin = blocks.getIndex_i(5, 5, 5);
		BlockIndex toBurn = blocks.getIndex_i(6, 5, 5);
		auto wood = MaterialType::byName(L"poplar wood");
		blocks.solid_set(toBurn, wood, false);
		area.m_hasTemperature.addTemperatureSource(origin, TemperatureDelta::create(1000));
		simulation.doStep();
		CHECK(blocks.fire_exists(toBurn));
		Fire& fire = blocks.fire_get(toBurn, wood);
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
		CHECK(!blocks.fire_exists(toBurn));
		CHECK(!blocks.solid_is(toBurn));
	}
}
