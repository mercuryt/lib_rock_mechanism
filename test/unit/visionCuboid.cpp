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
	SUBCASE("create")
	{
		Cuboid cuboid(area.getBlock(1, 1, 1), area.getBlock(0, 0, 0));
		VisionCuboid visionCuboid(cuboid, 1);
	}
	SUBCASE("can see into")
	{
		Cuboid c1(area.getBlock(0, 1, 1), area.getBlock(0, 0, 0));
		VisionCuboid vc1(c1, 1);
		Cuboid c2(area.getBlock(1, 1, 1), area.getBlock(1, 0, 0));
		VisionCuboid vc2(c2, 2);
		REQUIRE(vc1.m_cuboid.size() == 4);
		REQUIRE(vc2.m_cuboid.size() == 4);
		REQUIRE(vc2.canSeeInto(vc1.m_cuboid));
		REQUIRE(vc1.canSeeInto(vc2.m_cuboid));
	}
	SUBCASE("can combine with")
	{
		Cuboid c1(area.getBlock(0, 1, 1), area.getBlock(0, 0, 0));
		VisionCuboid vc1(c1, 1);
		Cuboid c2(area.getBlock(1, 1, 1), area.getBlock(1, 0, 0));
		VisionCuboid vc2(c2, 2);
		REQUIRE(vc1.m_cuboid.size() == 4);
		REQUIRE(vc2.m_cuboid.size() == 4);
		REQUIRE(vc1.canCombineWith(vc2.m_cuboid));
		Cuboid c3(area.getBlock(1, 1, 0), area.getBlock(1, 0, 0));
		VisionCuboid vc3(c3, 3);
		REQUIRE(!vc3.canCombineWith(vc1.m_cuboid));
		REQUIRE(!vc1.canCombineWith(vc3.m_cuboid));
	}
	SUBCASE("setup area")
	{
		if constexpr(Config::visionCuboidsActive)
		{
			REQUIRE(area.m_hasActors.m_visionCuboids.size() == 1);
			VisionCuboid& visionCuboid = *area.m_hasActors.m_visionCuboids.getVisionCuboidFor(area.getBlock(0, 0, 0));
			REQUIRE(visionCuboid.m_cuboid.size() == 8);
			for(Block& block : visionCuboid.m_cuboid)
				REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(block) == &visionCuboid);
		}
	}
}
TEST_CASE("split at")
{
	if constexpr(Config::visionCuboidsActive)
	{
		Simulation simulation;
		Area& area = simulation.m_hasAreas->createArea(2,2,1);
		Block& b1 = area.getBlock(0, 0, 0);
		Block& b2 = area.getBlock(1, 0, 0);
		Block& b3 = area.getBlock(0, 1, 0);
		Block& b4 = area.getBlock(1, 1, 0);
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
}
TEST_CASE("split below")
{
	if constexpr(Config::visionCuboidsActive)
	{
		Simulation simulation;
		Area& area = simulation.m_hasAreas->createArea(3,3,3);
		Block& middle = area.getBlock(1, 1, 1);
		Block& high = area.getBlock(2, 2, 2);
		Block& low = area.getBlock(0, 0, 0);
		middle.m_hasBlockFeatures.construct(BlockFeatureType::floor, MaterialType::byName("marble"));
		area.m_hasActors.m_visionCuboids.clearDestroyed();
		REQUIRE(area.m_hasActors.m_visionCuboids.size() == 2);
		REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(middle) == area.m_hasActors.m_visionCuboids.getVisionCuboidFor(high));
		REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(middle) != area.m_hasActors.m_visionCuboids.getVisionCuboidFor(low));
		REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(low)->m_cuboid.size() == 9);
		REQUIRE(area.m_hasActors.m_visionCuboids.getVisionCuboidFor(middle)->m_cuboid.size() == 18);
	}
}
