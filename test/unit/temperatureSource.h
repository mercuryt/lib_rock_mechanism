#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../lib/doctest.h"
#include "../../src/temperature.h"
#include "../../src/area.h"
#include "../../src/materialType.h"
#include "../../src/simulation.h"
#include "../../src/block.h"
TEST_CASE("temperature")
{
	Area area(10,10,10, 0);
	simulation::init();
	Block& origin = area.m_blocks[5][5][5];
	Block& b1 = area.m_blocks[5][5][6];
	Block& b2 = area.m_blocks[5][7][5];
	Block& toBurn = area.m_blocks[6][5][5];
	Block& toNotBurn = area.m_blocks[4][5][5];
	auto& wood = MaterialType::byName("wood");
	auto& marble = MaterialType::byName("marble");
	toBurn.setSolid(wood);
	toNotBurn.setSolid(marble);
	TemperatureSource temperatureSource(1000, origin);
	CHECK(origin.m_blockHasTemperature.get() == 1000);
	CHECK(b1.m_blockHasTemperature.get() == 1000);
	CHECK(b2.m_blockHasTemperature.get() == 250);
	CHECK(toBurn.m_blockHasTemperature.get() == 1000);
	CHECK(toNotBurn.m_blockHasTemperature.get() == 1000);
	CHECK(toBurn.m_fire);
	CHECK(toBurn.m_blockHasTemperature.get() > 1000);
	CHECK(!toNotBurn.m_fire);
	CHECK(toNotBurn.m_blockHasTemperature.get() > 1000);
	CHECK(!eventSchedule::data.empty());
}
