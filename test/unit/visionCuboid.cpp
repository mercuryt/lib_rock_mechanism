#include "../../lib/doctest.h"
#include "../../engine/visionCuboid.h"
#include "../../engine/area.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/config.h"
TEST_CASE("vision cuboid basic")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(2,2,2);
	Blocks& blocks = area.getBlocks();
	SUBCASE("create")
	{
		Cuboid cuboid(blocks, blocks.getIndex({1, 1, 1}), blocks.getIndex({0, 0, 0}));
		VisionCuboid visionCuboid(area, cuboid, 1);
	}
	SUBCASE("can see into")
	{
		Cuboid c1(blocks, blocks.getIndex({0, 1, 1}), blocks.getIndex({0, 0, 0}));
		VisionCuboid vc1(area, c1, 1);
		Cuboid c2(blocks, blocks.getIndex({1, 1, 1}), blocks.getIndex({1, 0, 0}));
		VisionCuboid vc2(area, c2, 2);
		REQUIRE(vc1.m_cuboid.size() == 4);
		REQUIRE(vc2.m_cuboid.size() == 4);
		REQUIRE(vc2.canSeeInto(vc1.m_cuboid));
		REQUIRE(vc1.canSeeInto(vc2.m_cuboid));
	}
	SUBCASE("can combine with")
	{
		Cuboid c1(blocks, blocks.getIndex({0, 1, 1}), blocks.getIndex({0, 0, 0}));
		VisionCuboid vc1(area, c1, 1);
		Cuboid c2(blocks, blocks.getIndex({1, 1, 1}), blocks.getIndex({1, 0, 0}));
		VisionCuboid vc2(area, c2, 2);
		REQUIRE(vc1.m_cuboid.size() == 4);
		REQUIRE(vc2.m_cuboid.size() == 4);
		REQUIRE(vc1.canCombineWith(vc2.m_cuboid));
		Cuboid c3(blocks, blocks.getIndex({1, 1, 0}), blocks.getIndex({1, 0, 0}));
		VisionCuboid vc3(area, c3, 3);
		REQUIRE(!vc3.canCombineWith(vc1.m_cuboid));
		REQUIRE(!vc1.canCombineWith(vc3.m_cuboid));
	}
	SUBCASE("setup area")
	{
		REQUIRE(area.m_hasActors.m_visionCuboids.size() == 1);
		VisionCuboid& visionCuboid = *area.m_hasActors.m_visionCuboids.getVisionCuboidFor(blocks.getIndex({0, 0, 0}));
		REQUIRE(visionCuboid.m_cuboid.size() == 8);
		for(BlockIndex block : visionCuboid.m_cuboid)
			REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(block) == &visionCuboid);
	}
}
TEST_CASE("split at")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(2,2,1);
	Blocks& blocks = area.getBlocks();
	BlockIndex b1 = blocks.getIndex({0, 0, 0});
	BlockIndex b2 = blocks.getIndex({1, 0, 0});
	BlockIndex b3 = blocks.getIndex({0, 1, 0});
	BlockIndex b4 = blocks.getIndex({1, 1, 0});
	VisionCuboid& vc0 = *area.m_hasActors.m_visionCuboids.getVisionCuboidFor(b1);
	vc0.splitAt(b4);
	REQUIRE(vc0.m_destroy);
	area.m_hasActors.m_visionCuboids.clearDestroyed();
	REQUIRE(area.m_hasActors.m_visionCuboids.size() == 2);
	VisionCuboid& vc1 = *area.m_hasActors.m_visionCuboids.getVisionCuboidFor(b1);
	VisionCuboid& vc2 = *area.m_hasActors.m_visionCuboids.getVisionCuboidFor(b2);
	REQUIRE(&vc1 != &vc2);
	REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(b3) == &vc1);
	REQUIRE(vc1.m_cuboid.size() == 2);
	REQUIRE(vc2.m_cuboid.size() == 1);
	REQUIRE(!area.m_hasActors.m_visionCuboids.getVisionCuboidFor(b4));
}
TEST_CASE("split below")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(3,3,3);
	Blocks& blocks = area.getBlocks();
	BlockIndex middle = blocks.getIndex({1, 1, 1});
	BlockIndex high = blocks.getIndex({2, 2, 2});
	BlockIndex low = blocks.getIndex({0, 0, 0});
	blocks.blockFeature_construct(middle, BlockFeatureType::floor, MaterialType::byName("marble"));
	area.m_hasActors.m_visionCuboids.clearDestroyed();
	REQUIRE(area.m_hasActors.m_visionCuboids.size() == 2);
	REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(middle) == area.m_hasActors.m_visionCuboids.getVisionCuboidFor(high));
	REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(middle) != area.m_hasActors.m_visionCuboids.getVisionCuboidFor(low));
	REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(low)->m_cuboid.size() == 9);
	REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(middle)->m_cuboid.size() == 18);
}
