#include "../../lib/doctest.h"
#include "../../engine/visionCuboid.h"
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
		REQUIRE(cuboids.maybeGetForBlock(low)->m_cuboid.size(blocks) == 8);
	}
	SUBCASE("can merge")
	{
		const BlockIndex& low = blocks.getIndex_i(0, 0, 0);
		cuboids.blockIsSometimesOpaque(area, low);
		REQUIRE(cuboids.maybeGetForBlock(high)->m_cuboid.size(blocks) == 4);
		REQUIRE(cuboids.size() == 3);
		REQUIRE(cuboids.getIndexForBlock(low).empty());
		cuboids.blockIsNeverOpaque(area, low);
		REQUIRE(cuboids.size() == 1);
		REQUIRE(cuboids.getIndexForBlock(low).exists());
		REQUIRE(cuboids.maybeGetForBlock(low)->m_cuboid.size(blocks) == 8);
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
			REQUIRE(vc1.m_cuboid.size(blocks) == 2);
			REQUIRE(vc3.m_cuboid.size(blocks) == 1);
		}
		else
		{

			assert(&vc1 == &vc3);
			REQUIRE(vc1.m_cuboid.size(blocks) == 2);
			REQUIRE(vc2.m_cuboid.size(blocks) == 1);
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
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(low)->m_cuboid.size(blocks) == 9);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(middle)->m_cuboid.size(blocks) == 18);
	}
}