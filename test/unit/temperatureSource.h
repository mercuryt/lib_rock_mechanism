#include "../../lib/doctest.h"
#include "../../src/temperature.h"
#include "../../src/area.h"
#include "../../src/materialType.h"
#include "../../src/simulation.h"
#include "../../src/block.h"
#include "../../src/definitions.h"
TEST_CASE("temperature")
{
	simulation::init();
	Area area(10,10,10,0);
	SUBCASE("solid blocks burn")
	{
		Block& origin = area.m_blocks[5][5][5];
		Block& b1 = area.m_blocks[5][5][6];
		Block& b2 = area.m_blocks[5][7][5];
		Block& b3 = area.m_blocks[9][9][9];
		Block& toBurn = area.m_blocks[6][5][5];
		Block& toNotBurn = area.m_blocks[4][5][5];
		auto& wood = MaterialType::byName("poplar wood");
		auto& marble = MaterialType::byName("marble");
		toBurn.setSolid(wood);
		toNotBurn.setSolid(marble);
		area.m_areaHasTemperature.addTemperatureSource(origin, 1000);
		area.m_areaHasTemperature.applyDeltas();
		CHECK(origin.m_blockHasTemperature.get() == 1000);
		CHECK(b1.m_blockHasTemperature.get() == 1000);
		CHECK(b2.m_blockHasTemperature.get() == 88);
		CHECK(b3.m_blockHasTemperature.get() == 0);
		CHECK(toBurn.m_blockHasTemperature.get() == 1000);
		CHECK(toNotBurn.m_blockHasTemperature.get() == 1000);
		CHECK(toBurn.m_fire);
		// Fire exists but the new deltas it has created have not been applied
		CHECK(toBurn.m_blockHasTemperature.get() == 1000);
		CHECK(!toNotBurn.m_fire);
		CHECK(toNotBurn.m_blockHasTemperature.get() == 1000);
		CHECK(!eventSchedule::data.empty());
		area.m_areaHasTemperature.applyDeltas();
		CHECK(toBurn.m_blockHasTemperature.get() > 1000);
		CHECK(toNotBurn.m_blockHasTemperature.get() > 1000);
	}
}
