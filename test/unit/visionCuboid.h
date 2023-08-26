#include "../../lib/doctest.h"
#include "../../src/visionCuboid.h"
#include "../../src/area.h"
#include "../../src/simulation.h"
TEST_CASE("vision cuboid basic")
{
	Simulation simulation;
	Area& area = simulation.createArea(2,2,2);
	SUBCASE("create")
	{
		Cuboid cuboid(area.m_blocks[1][1][1], area.m_blocks[0][0][0]);
		VisionCuboid visionCuboid(cuboid);
	}
	SUBCASE("can see into")
	{
		Cuboid c1(area.m_blocks[0][1][1], area.m_blocks[0][0][0]);
		VisionCuboid vc1(c1);
		Cuboid c2(area.m_blocks[1][1][1], area.m_blocks[1][0][0]);
		VisionCuboid vc2(c2);
		CHECK(vc1.m_cuboid.size() == 4);
		CHECK(vc2.m_cuboid.size() == 4);
		CHECK(vc2.canSeeInto(vc1.m_cuboid));
		CHECK(vc1.canSeeInto(vc2.m_cuboid));
	}
	SUBCASE("can combine with")
	{
		Cuboid c1(area.m_blocks[0][1][1], area.m_blocks[0][0][0]);
		VisionCuboid vc1(c1);
		Cuboid c2(area.m_blocks[1][1][1], area.m_blocks[1][0][0]);
		VisionCuboid vc2(c2);
		CHECK(vc1.m_cuboid.size() == 4);
		CHECK(vc2.m_cuboid.size() == 4);
		CHECK(vc1.canCombineWith(vc2.m_cuboid));
		Cuboid c3(area.m_blocks[1][1][0], area.m_blocks[1][0][0]);
		VisionCuboid vc3(c3);
		CHECK(!vc3.canCombineWith(vc1.m_cuboid));
		CHECK(!vc1.canCombineWith(vc3.m_cuboid));
	}
	SUBCASE("setup area")
	{
		VisionCuboid::setup(area);
		CHECK(area.m_visionCuboids.size() == 1);
		VisionCuboid& visionCuboid = *area.m_blocks[0][0][0].m_visionCuboid;
		CHECK(visionCuboid.m_cuboid.size() == 8);
		for(Block& block : visionCuboid.m_cuboid)
			CHECK(block.m_visionCuboid == &visionCuboid);
	}
}
TEST_CASE("split at")
{
	Simulation simulation;
	Area& area = simulation.createArea(2,2,1);
	Block& b1 = area.m_blocks[0][0][0];
	Block& b2 = area.m_blocks[1][0][0];
	Block& b3 = area.m_blocks[0][1][0];
	Block& b4 = area.m_blocks[1][1][0];
	area.visionCuboidsActivate();
	VisionCuboid& vc0 = *b1.m_visionCuboid;
	vc0.splitAt(b4);
	CHECK(vc0.m_destroy);
	VisionCuboid::clearDestroyed(area);
	CHECK(area.m_visionCuboids.size() == 2);
	VisionCuboid& vc1 = *b1.m_visionCuboid;
	VisionCuboid& vc2 = *b2.m_visionCuboid;
	CHECK(&vc1 != &vc2);
	CHECK(b3.m_visionCuboid == &vc1);
	CHECK(vc1.m_cuboid.size() == 2);
	CHECK(vc2.m_cuboid.size() == 1);
	CHECK(area.m_blocks[1][1][0].m_visionCuboid == nullptr);
}
TEST_CASE("split below")
{
	Simulation simulation;
	Area& area = simulation.createArea(3,3,3);
	Block& middle = area.m_blocks[1][1][1];
	Block& high = area.m_blocks[2][2][2];
	Block& low = area.m_blocks[0][0][0];
	area.visionCuboidsActivate();
	middle.m_hasBlockFeatures.construct(BlockFeatureType::floor, MaterialType::byName("marble"));
	VisionCuboid::clearDestroyed(area);
	CHECK(area.m_visionCuboids.size() == 2);
	CHECK(middle.m_visionCuboid == high.m_visionCuboid);
	CHECK(middle.m_visionCuboid != low.m_visionCuboid);
	CHECK(low.m_visionCuboid->m_cuboid.size() == 9);
	CHECK(middle.m_visionCuboid->m_cuboid.size() == 18);
}
