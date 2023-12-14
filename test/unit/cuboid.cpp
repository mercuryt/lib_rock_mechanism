#include "../../lib/doctest.h"
#include "../../src/cuboid.h"
#include "../../src/area.h"
#include "../../src/simulation.h"
TEST_CASE("cuboid")
{
	Simulation simulation;
	Area& area = simulation.createArea(2,2,2);
	SUBCASE("create")
	{
		Cuboid cuboid(&area.getBlock(1, 1, 1), &area.getBlock(0, 0, 0));
		CHECK(cuboid.size() == 8);
		cuboid.contains(area.getBlock(1, 1, 0));
	}
	SUBCASE("merge")
	{
		Cuboid c1(&area.getBlock(0, 1, 1), &area.getBlock(0, 0, 0));
		CHECK(c1.size() == 4);
		Cuboid c2(&area.getBlock(1, 1, 1), &area.getBlock(1, 0, 0));
		CHECK(c1.canMerge(c2));
		Cuboid sum = c1.sum(c2);
		CHECK(sum.size() == 8);
		CHECK(c1 != c2);
		c2.merge(c1);
		CHECK(c2.size() == 8);
	}
	SUBCASE("get face")
	{
		Cuboid c1(&area.getBlock(1, 1, 1), &area.getBlock(0, 0, 0));
		Cuboid face = c1.getFace(4);
		CHECK(face.size() == 4);
		CHECK(face.contains(area.getBlock(1, 0, 0)));
		CHECK(!face.contains(area.getBlock(0, 0, 0)));
	}
}
