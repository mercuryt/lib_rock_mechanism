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
		VisionCuboid visionCuboid(area, cuboid, VisionCuboidId::create(1));
	}
	SUBCASE("can see into")
	{
		Cuboid c1(blocks, blocks.getIndex_i(0, 1, 1), blocks.getIndex_i(0, 0, 0));
		VisionCuboid vc1(area, c1, VisionCuboidId::create(1));
		Cuboid c2(blocks, blocks.getIndex_i(1, 1, 1), blocks.getIndex_i(1, 0, 0));
		VisionCuboid vc2(area, c2, VisionCuboidId::create(2));
		REQUIRE(vc1.m_cuboid.size(blocks) == 4);
		REQUIRE(vc2.m_cuboid.size(blocks) == 4);
		REQUIRE(vc2.canSeeInto(vc1.m_cuboid));
		REQUIRE(vc1.canSeeInto(vc2.m_cuboid));
	}
	SUBCASE("can combine with")
	{
		Cuboid c1(blocks, blocks.getIndex_i(0, 1, 1), blocks.getIndex_i(0, 0, 0));
		VisionCuboid vc1(area, c1, VisionCuboidId::create(1));
		Cuboid c2(blocks, blocks.getIndex_i(1, 1, 1), blocks.getIndex_i(1, 0, 0));
		VisionCuboid vc2(area, c2, VisionCuboidId::create(2));
		REQUIRE(vc1.m_cuboid.size(blocks) == 4);
		REQUIRE(vc2.m_cuboid.size(blocks) == 4);
		REQUIRE(vc1.canCombineWith(vc2.m_cuboid));
		Cuboid c3(blocks, blocks.getIndex_i(1, 1, 0), blocks.getIndex_i(1, 0, 0));
		VisionCuboid vc3(area, c3, VisionCuboidId::create(3));
		REQUIRE(!vc3.canCombineWith(vc1.m_cuboid));
		REQUIRE(!vc1.canCombineWith(vc3.m_cuboid));
	}
	SUBCASE("setup area")
	{
		REQUIRE(area.m_visionCuboids.size() == 1);
		VisionCuboid& visionCuboid = *area.m_visionCuboids.getVisionCuboidFor(blocks.getIndex_i(0, 0, 0));
		REQUIRE(visionCuboid.m_cuboid.size(blocks) == 8);
		for(const BlockIndex& block : visionCuboid.m_cuboid.getView(blocks))
			REQUIRE(area.m_visionCuboids.getVisionCuboidFor(block) == &visionCuboid);
	}
}
TEST_CASE("split at")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(2,2,1);
	Blocks& blocks = area.getBlocks();
	BlockIndex b1 = blocks.getIndex_i(0, 0, 0);
	BlockIndex b2 = blocks.getIndex_i(1, 0, 0);
	BlockIndex b3 = blocks.getIndex_i(0, 1, 0);
	BlockIndex b4 = blocks.getIndex_i(1, 1, 0);
	VisionCuboid& vc0 = *area.m_visionCuboids.getVisionCuboidFor(b1);
	vc0.splitAt(b4);
	REQUIRE(vc0.m_destroy);
	area.m_visionCuboids.clearDestroyed();
	REQUIRE(area.m_visionCuboids.size() == 2);
	VisionCuboid& vc1 = *area.m_visionCuboids.getVisionCuboidFor(b1);
	VisionCuboid& vc2 = *area.m_visionCuboids.getVisionCuboidFor(b2);
	REQUIRE(&vc1 != &vc2);
	REQUIRE(area.m_visionCuboids.getVisionCuboidFor(b3) == &vc1);
	REQUIRE(vc1.m_cuboid.size(blocks) == 2);
	REQUIRE(vc2.m_cuboid.size(blocks) == 1);
	REQUIRE(!area.m_visionCuboids.getVisionCuboidFor(b4));
}
TEST_CASE("split below")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(3,3,3);
	Blocks& blocks = area.getBlocks();
	BlockIndex middle = blocks.getIndex_i(1, 1, 1);
	BlockIndex high = blocks.getIndex_i(2, 2, 2);
	BlockIndex low = blocks.getIndex_i(0, 0, 0);
	blocks.blockFeature_construct(middle, BlockFeatureType::floor, MaterialType::byName(L"marble"));
	area.m_visionCuboids.clearDestroyed();
	REQUIRE(area.m_visionCuboids.size() == 2);
	REQUIRE(area.m_visionCuboids.getVisionCuboidFor(middle) == area.m_visionCuboids.getVisionCuboidFor(high));
	REQUIRE(area.m_visionCuboids.getVisionCuboidFor(middle) != area.m_visionCuboids.getVisionCuboidFor(low));
	REQUIRE(area.m_visionCuboids.getVisionCuboidFor(low)->m_cuboid.size(blocks) == 9);
	REQUIRE(area.m_visionCuboids.getVisionCuboidFor(middle)->m_cuboid.size(blocks) == 18);
}
