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
	SUBCASE("create")
	{
		Cuboid cuboid(blocks, blocks.getIndex_i(1, 1, 1), blocks.getIndex_i(0, 0, 0));
		VisionCuboid& visionCuboid = area.m_visionCuboids.create(area, cuboid);
		REQUIRE(visionCuboid.m_index == 1);
	}
	SUBCASE("can see into")
	{
		Cuboid c1(blocks, blocks.getIndex_i(0, 1, 1), blocks.getIndex_i(0, 0, 0));
		Cuboid c2(blocks, blocks.getIndex_i(1, 1, 1), blocks.getIndex_i(1, 0, 0));
		REQUIRE(c1.size(blocks) == 4);
		REQUIRE(c2.size(blocks) == 4);
		VisionCuboid& vc1 = area.m_visionCuboids.create(area, c1);
		REQUIRE(vc1.canSeeInto(area, c2));
		VisionCuboid& vc2 = area.m_visionCuboids.create(area, c2);
		REQUIRE(vc2.canSeeInto(area, c1));
	}
	SUBCASE("can combine with")
	{
		Cuboid c1(blocks, blocks.getIndex_i(0, 1, 1), blocks.getIndex_i(0, 0, 0));
		area.m_visionCuboids.create(area, c1);
		Cuboid c2(blocks, blocks.getIndex_i(1, 1, 1), blocks.getIndex_i(1, 0, 0));
		area.m_visionCuboids.create(area, c2);
		Cuboid c3(blocks, blocks.getIndex_i(1, 1, 0), blocks.getIndex_i(1, 0, 0));
		area.m_visionCuboids.create(area, c3);
		const VisionCuboid& vc1 = *area.m_visionCuboids.maybeGetForBlock(c1.m_highest);
		const VisionCuboid& vc2 = *area.m_visionCuboids.maybeGetForBlock(c2.m_highest);
		const VisionCuboid& vc3 = *area.m_visionCuboids.maybeGetForBlock(c3.m_highest);
		REQUIRE(vc1.m_cuboid.size(blocks) == 4);
		REQUIRE(vc2.m_cuboid.size(blocks) == 4);
		REQUIRE(vc1.canCombineWith(area, vc2.m_cuboid));
		REQUIRE(!vc3.canCombineWith(area, vc1.m_cuboid));
		REQUIRE(!vc1.canCombineWith(area, vc3.m_cuboid));
	}
	SUBCASE("setup area")
	{
		REQUIRE(area.m_visionCuboids.size() == 1);
		VisionCuboid& visionCuboid = *area.m_visionCuboids.maybeGetForBlock(blocks.getIndex_i(0, 0, 0));
		REQUIRE(visionCuboid.m_cuboid.size(blocks) == 8);
		for(const BlockIndex& block : visionCuboid.m_cuboid.getView(blocks))
			REQUIRE(area.m_visionCuboids.maybeGetForBlock(block) == &visionCuboid);
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