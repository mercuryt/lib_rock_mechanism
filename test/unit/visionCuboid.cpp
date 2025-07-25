#include "../../lib/doctest.h"
#include "../../engine/vision/visionCuboid.h"
#include "../../engine/area/area.h"
#include "../../engine/simulation/simulation.h"
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
	Blocks& blocks = area.getSpace();
	auto& cuboids = area.m_visionCuboids;
	const BlockIndex& low = blocks.getIndex_i(0, 0, 0);
	const BlockIndex& high = blocks.getIndex_i(1, 1, 1);
	SUBCASE("create")
	{
		CHECK(cuboids.size() == 1);
		CHECK(cuboids.getVisionCuboidIndexForPoint(low).exists());
		CHECK(cuboids.getVisionCuboidIndexForPoint(high) == cuboids.getVisionCuboidIndexForPoint(low));
		VisionCuboidId visionCuboid = cuboids.getVisionCuboidIndexForPoint(low);
		Cuboid cuboid = cuboids.getCuboidByVisionCuboidId(visionCuboid);
		CHECK(cuboid.size() == 8);
		CHECK(cuboids.getAdjacentsForVisionCuboid(visionCuboid).empty());
	}
	SUBCASE("can merge")
	{
		const BlockIndex& low = blocks.getIndex_i(0, 0, 0);
		cuboids.blockIsOpaque(low);
		CHECK(cuboids.size() == 3);
		CHECK(cuboids.maybeGetVisionCuboidIndexForPoint(low).empty());
		VisionCuboidId highCuboid = cuboids.getVisionCuboidIndexForPoint(high);
		CHECK(cuboids.getCuboidByVisionCuboidId(highCuboid).size() == 4);
		CHECK(cuboids.getAdjacentsForVisionCuboid(highCuboid).size() == 2);
		VisionCuboidId eastCuboid = cuboids.getVisionCuboidIndexForPoint(blocks.getIndex_i(0, 1, 0));
		CHECK(cuboids.getCuboidByVisionCuboidId(eastCuboid).size() == 2);
		CHECK(cuboids.getAdjacentsForVisionCuboid(eastCuboid).size() == 2);
		VisionCuboidId southCuboid = cuboids.getVisionCuboidIndexForPoint(blocks.getIndex_i(1, 0, 0));
		CHECK(cuboids.getCuboidByVisionCuboidId(southCuboid).size() == 1);
		CHECK(cuboids.getAdjacentsForVisionCuboid(southCuboid).size() == 2);
		CHECK(cuboids.getAdjacentsForVisionCuboid(highCuboid).contains(eastCuboid));
		CHECK(cuboids.getAdjacentsForVisionCuboid(highCuboid).contains(southCuboid));
		CHECK(cuboids.getAdjacentsForVisionCuboid(eastCuboid).contains(highCuboid));
		CHECK(cuboids.getAdjacentsForVisionCuboid(eastCuboid).contains(southCuboid));
		CHECK(cuboids.getAdjacentsForVisionCuboid(southCuboid).contains(highCuboid));
		CHECK(cuboids.getAdjacentsForVisionCuboid(southCuboid).contains(eastCuboid));
		cuboids.blockIsTransparent(low);
		CHECK(cuboids.size() == 1);
		CHECK(cuboids.getVisionCuboidIndexForPoint(low).exists());
		VisionCuboidId cuboid = cuboids.getVisionCuboidIndexForPoint(high);
		CHECK(cuboids.getCuboidByVisionCuboidId(cuboid).size() == 8);
		CHECK(cuboids.getAdjacentsForVisionCuboid(cuboid).empty());
	}
}
TEST_CASE("vision cuboid split")
{
		Simulation simulation;
	SUBCASE("split at")
	{
		Area& area = simulation.m_hasAreas->createArea(2,2,1);
		Blocks& blocks = area.getSpace();
		BlockIndex b1 = blocks.getIndex_i(0, 0, 0);
		BlockIndex b2 = blocks.getIndex_i(1, 0, 0);
		BlockIndex b3 = blocks.getIndex_i(0, 1, 0);
		BlockIndex b4 = blocks.getIndex_i(1, 1, 0);
		CHECK(area.m_visionCuboids.size() == 1);
		area.m_visionCuboids.remove({blocks, b4, b4});
		CHECK(area.m_visionCuboids.maybeGetVisionCuboidIndexForPoint(b4).empty());
		CHECK(area.m_visionCuboids.size() == 2);
		VisionCuboidId vc1 = area.m_visionCuboids.getVisionCuboidIndexForPoint(b1);
		VisionCuboidId vc2 = area.m_visionCuboids.getVisionCuboidIndexForPoint(b2);
		VisionCuboidId vc3 = area.m_visionCuboids.getVisionCuboidIndexForPoint(b3);
		if(vc1 == vc2)
		{
			assert(vc2 != vc3);
			CHECK(area.m_visionCuboids.getCuboidByVisionCuboidId(vc1).size() == 2);
			CHECK(area.m_visionCuboids.getCuboidByVisionCuboidId(vc3).size() == 1);
		}
		else
		{
			assert(vc1 == vc3);
			CHECK(area.m_visionCuboids.getCuboidByVisionCuboidId(vc1).size() == 2);
			CHECK(area.m_visionCuboids.getCuboidByVisionCuboidId(vc2).size() == 1);
		}
		CHECK(area.m_visionCuboids.maybeGetVisionCuboidIndexForPoint(b4).empty());
	}
	SUBCASE("split below")
	{
		Area& area = simulation.m_hasAreas->createArea(3,3,3);
		Blocks& blocks = area.getSpace();
		BlockIndex middle = blocks.getIndex_i(1, 1, 1);
		BlockIndex high = blocks.getIndex_i(2, 2, 2);
		BlockIndex low = blocks.getIndex_i(0, 0, 0);
		blocks.blockFeature_construct(middle, BlockFeatureTypeId::Floor, MaterialType::byName("marble"));
		CHECK(area.m_visionCuboids.size() == 2);
		CHECK(area.m_visionCuboids.getVisionCuboidIndexForPoint(middle) == area.m_visionCuboids.getVisionCuboidIndexForPoint(high));
		CHECK(area.m_visionCuboids.getVisionCuboidIndexForPoint(middle) != area.m_visionCuboids.getVisionCuboidIndexForPoint(low));
		CHECK(area.m_visionCuboids.getCuboidForPoint(low).size() == 9);
		CHECK(area.m_visionCuboids.getCuboidForPoint(middle).size() == 18);
	}
}