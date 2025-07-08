#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/numericTypes/types.h"
TEST_CASE("block")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getSpace();
	BlockIndex lowest = BlockIndex::create(0);
	CHECK(blocks.getBlockBelow(lowest).empty());
	CHECK(blocks.getBlockNorth(lowest).empty());
	CHECK(blocks.getBlockWest(lowest).empty());
	CHECK(blocks.getBlockAbove(lowest) == 100);
	CHECK(blocks.getBlockSouth(lowest) == 10);
	CHECK(blocks.getBlockEast(lowest) == 1);
	CHECK(area.m_visionCuboids.getVisionCuboidIndexForPoint(lowest).exists());
}

