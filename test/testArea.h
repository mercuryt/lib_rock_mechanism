#include "testShared.h"
#define DOCTEST_CONFIG_IMPLIMENT_WITH_MAIN
#include "../lib/doctest.h"

void testCreate()
{
	Area area = Area(200,200,200);
	TEST_CASE("Create"){
		REQUIRE(area.xSize == 200);
		REQUIRE(area.ySize == 200);
		REQUIRE(area.zSize == 200);
	}
}
