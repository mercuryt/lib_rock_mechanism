#include "../../lib/doctest.h"
#include "../../engine/vision/visionCuboid.h"
#include "../../engine/area.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/config.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"

TEST_CASE("vision cuboid basic")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(2,2,2);
	Blocks& blocks = area.getBlocks();
	auto& cuboids = area.m_visionCuboids;
	const BlockIndex& low = blocks.getIndex_i(0, 0, 0);
	const BlockIndex& high = blocks.getIndex_i(1, 1, 1);
	SUBCASE("create")
	{
		REQUIRE(cuboids.size() == 1);
		REQUIRE(cuboids.getIndexForBlock(low).exists());
		REQUIRE(cuboids.getIndexForBlock(high) == cuboids.getIndexForBlock(low));
		VisionCuboid& visionCuboid = *cuboids.maybeGetForBlock(low);
		REQUIRE(visionCuboid.m_cuboid.size() == 8);
		REQUIRE(!visionCuboid.toDestory());
		REQUIRE(visionCuboid.m_adjacent.empty());
	}
	SUBCASE("can merge")
	{
		const BlockIndex& low = blocks.getIndex_i(0, 0, 0);
		cuboids.blockIsOpaque(area, low);
		REQUIRE(cuboids.size() == 3);
		REQUIRE(cuboids.getIndexForBlock(low).empty());
		VisionCuboid& highCuboid = *cuboids.maybeGetForBlock(high);
		REQUIRE(highCuboid.m_cuboid.size() == 4);
		REQUIRE(highCuboid.m_adjacent.size() == 2);
		VisionCuboid& eastCuboid = *cuboids.maybeGetForBlock(blocks.getIndex_i(0, 1, 0));
		REQUIRE(eastCuboid.m_cuboid.size() == 2);
		REQUIRE(eastCuboid.m_adjacent.size() == 2);
		VisionCuboid& southCuboid = *cuboids.maybeGetForBlock(blocks.getIndex_i(1, 0, 0));
		REQUIRE(southCuboid.m_cuboid.size() == 1);
		REQUIRE(southCuboid.m_adjacent.size() == 2);
		REQUIRE(highCuboid.m_adjacent.contains(eastCuboid.m_index));
		REQUIRE(highCuboid.m_adjacent.contains(southCuboid.m_index));
		REQUIRE(eastCuboid.m_adjacent.contains(highCuboid.m_index));
		REQUIRE(eastCuboid.m_adjacent.contains(southCuboid.m_index));
		REQUIRE(southCuboid.m_adjacent.contains(highCuboid.m_index));
		REQUIRE(southCuboid.m_adjacent.contains(eastCuboid.m_index));
		cuboids.blockIsTransparent(area, low);
		REQUIRE(cuboids.size() == 1);
		REQUIRE(cuboids.getIndexForBlock(low).exists());
		VisionCuboid& cuboid = *cuboids.maybeGetForBlock(high);
		REQUIRE(cuboid.m_cuboid.size() == 8);
		REQUIRE(cuboid.m_adjacent.empty());
	}
}
TEST_CASE("vision cuboid split")
{
		Simulation simulation;
	SUBCASE("split at")
	{
		Area& area = simulation.m_hasAreas->createArea(2,2,1);
		Blocks& blocks = area.getBlocks();
		BlockIndex b1 = blocks.getIndex_i(0, 0, 0);
		BlockIndex b2 = blocks.getIndex_i(1, 0, 0);
		BlockIndex b3 = blocks.getIndex_i(0, 1, 0);
		BlockIndex b4 = blocks.getIndex_i(1, 1, 0);
		REQUIRE(area.m_visionCuboids.size() == 1);
		area.m_visionCuboids.splitAt(area, *area.m_visionCuboids.maybeGetForBlock(b1), b4);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(b4) == nullptr);
		REQUIRE(area.m_visionCuboids.size() == 3);
		area.m_visionCuboids.clearDestroyed(area);
		REQUIRE(area.m_visionCuboids.size() == 2);
		VisionCuboid& vc1 = *area.m_visionCuboids.maybeGetForBlock(b1);
		VisionCuboid& vc2 = *area.m_visionCuboids.maybeGetForBlock(b2);
		VisionCuboid& vc3 = *area.m_visionCuboids.maybeGetForBlock(b3);
		if(&vc1 == &vc2)
		{
			assert(&vc2 != &vc3);
			REQUIRE(vc1.m_cuboid.size() == 2);
			REQUIRE(vc3.m_cuboid.size() == 1);
		}
		else
		{
			assert(&vc1 == &vc3);
			REQUIRE(vc1.m_cuboid.size() == 2);
			REQUIRE(vc2.m_cuboid.size() == 1);
		}
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(b4) == nullptr);
	}
	SUBCASE("split below")
	{
		Area& area = simulation.m_hasAreas->createArea(3,3,3);
		Blocks& blocks = area.getBlocks();
		BlockIndex middle = blocks.getIndex_i(1, 1, 1);
		BlockIndex high = blocks.getIndex_i(2, 2, 2);
		BlockIndex low = blocks.getIndex_i(0, 0, 0);
		blocks.blockFeature_construct(middle, BlockFeatureType::floor, MaterialType::byName(L"marble"));
		area.m_visionCuboids.clearDestroyed(area);
		REQUIRE(area.m_visionCuboids.size() == 2);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(middle) == area.m_visionCuboids.maybeGetForBlock(high));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(middle) != area.m_visionCuboids.maybeGetForBlock(low));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(low)->m_cuboid.size() == 9);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(middle)->m_cuboid.size() == 18);
	}
}