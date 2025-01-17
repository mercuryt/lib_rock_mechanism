#include "../../lib/doctest.h"
#include "../../engine/cuboid.h"
#include "../../engine/area.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/items/items.h"
#include "../../engine/actors/actors.h"
#include "../../engine/plants.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
TEST_CASE("cuboid")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(2,2,2);
	Blocks& blocks = area.getBlocks();
	SUBCASE("create")
	{
		Cuboid cuboid(blocks, blocks.getIndex_i(1, 1, 1), blocks.getIndex_i(0, 0, 0));
		REQUIRE(cuboid.size(blocks) == 8);
		REQUIRE(cuboid.contains(blocks, blocks.getIndex_i(1, 1, 0)));
	}
	SUBCASE("merge")
	{
		Cuboid c1(blocks, blocks.getIndex_i(0, 1, 1), blocks.getIndex_i(0, 0, 0));
		REQUIRE(c1.size(blocks) == 4);
		Cuboid c2(blocks, blocks.getIndex_i(1, 1, 1), blocks.getIndex_i(1, 0, 0));
		REQUIRE(c1.canMerge(blocks, c2));
		Cuboid sum = c1.sum(blocks, c2);
		REQUIRE(sum.size(blocks) == 8);
		REQUIRE(c1 != c2);
		c2.merge(blocks, c1);
		REQUIRE(c2.size(blocks) == 8);
	}
	SUBCASE("get face")
	{
		Cuboid c1(blocks, blocks.getIndex_i(1, 1, 1), blocks.getIndex_i(0, 0, 0));
		Cuboid face = c1.getFace(blocks, Facing::create(4)); // x + 1, east.
		REQUIRE(face.size(blocks) == 4);
		REQUIRE(face.contains(blocks, blocks.getIndex_i(1, 0, 0)));
		REQUIRE(!face.contains(blocks, blocks.getIndex_i(0, 0, 0)));
	}
}