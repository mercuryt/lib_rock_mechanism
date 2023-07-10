#include "../../src/temperatureSource.h"
TEST_CASE("temperatureSource")
{
	Area area(10,10,10);
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
	CHECK(origin.m_deltaTemperature == 1000);
	CHECK(b1.m_deltaTemperature == 1000);
	CHECK(b2.m_deltaTemperature == 250);
	CHECK(toBurn.m_deltaTemperature == 1000);
	CHECK(toNotBurn.m_deltaTemperature == 1000);
	CHECK(area.m_blocksWithChangedTemperature.contains(&toBurn));
	CHECK(area.m_blocksWithChangedTemperature.contains(&toNotBurn));
	toBurn.applyTemperatureChange(area.m_blocksWithChangedTemperature.at(&toBurn), toBurn.m_deltaTemperature);
	CHECK(toBurn.m_fire);
	CHECK(toBurn.m_deltaTemperature > 1000);
	CHECK(!toNotBurn.m_fire);
	CHECK(toNotBurn.m_deltaTemperature > 1000);
	CHECK(!eventSchedule::data.empty());
}
