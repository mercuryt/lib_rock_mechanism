#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area.h"
#include "../../engine/blocks/blocks.h"
#include "types.h"
TEST_CASE("block")
{
	Simulation simulation(L"", 0);
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	BlockIndex lowest = 0;
	REQUIRE(blocks.getBlockBelow(lowest) == BLOCK_INDEX_MAX);
	REQUIRE(blocks.getBlockSouth(lowest) == BLOCK_INDEX_MAX);
	REQUIRE(blocks.getBlockWest(lowest) == BLOCK_INDEX_MAX);
	REQUIRE(blocks.getBlockAbove(lowest) == 100);
	REQUIRE(blocks.getBlockNorth(lowest) == 10);
	REQUIRE(blocks.getBlockEast(lowest) == 1);
	REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(0));
}

