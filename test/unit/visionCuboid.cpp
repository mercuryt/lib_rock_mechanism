#include "../../lib/doctest.h"
#include "../../engine/vision/visionCuboid.h"
#include "../../engine/area/area.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/config.h"
#include "../../engine/space/space.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"

TEST_CASE("vision cuboid basic")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(2,2,2);
	auto& cuboids = area.m_visionCuboids;
	const Point3D& low = Point3D::create(0, 0, 0);
	const Point3D& high = Point3D::create(1, 1, 1);
	SUBCASE("create")
	{
		CHECK(cuboids.size() == 1);
		CHECK(cuboids.getVisionCuboidIndexForPoint(low).exists());
		CHECK(cuboids.getVisionCuboidIndexForPoint(high) == cuboids.getVisionCuboidIndexForPoint(low));
		VisionCuboidId visionCuboid = cuboids.getVisionCuboidIndexForPoint(low);
		Cuboid cuboid = cuboids.getCuboidByVisionCuboidId(visionCuboid);
		CHECK(cuboid.volume() == 8);
		CHECK(cuboids.getAdjacentsForVisionCuboid(visionCuboid).empty());
	}
	SUBCASE("can merge")
	{
		cuboids.pointIsOpaque(low);
		CHECK(cuboids.size() == 3);
		CHECK(cuboids.maybeGetVisionCuboidIndexForPoint(low).empty());
		VisionCuboidId highCuboid = cuboids.getVisionCuboidIndexForPoint(high);
		CHECK(cuboids.getCuboidByVisionCuboidId(highCuboid).volume() == 4);
		CHECK(cuboids.getAdjacentsForVisionCuboid(highCuboid).size() == 2);
		VisionCuboidId eastCuboid = cuboids.getVisionCuboidIndexForPoint(Point3D::create(0, 1, 0));
		CHECK(cuboids.getCuboidByVisionCuboidId(eastCuboid).volume() == 2);
		CHECK(cuboids.getAdjacentsForVisionCuboid(eastCuboid).size() == 2);
		VisionCuboidId southCuboid = cuboids.getVisionCuboidIndexForPoint(Point3D::create(1, 0, 0));
		CHECK(cuboids.getCuboidByVisionCuboidId(southCuboid).volume() == 1);
		CHECK(cuboids.getAdjacentsForVisionCuboid(southCuboid).size() == 2);
		CHECK(cuboids.getAdjacentsForVisionCuboid(highCuboid).contains(eastCuboid));
		CHECK(cuboids.getAdjacentsForVisionCuboid(highCuboid).contains(southCuboid));
		CHECK(cuboids.getAdjacentsForVisionCuboid(eastCuboid).contains(highCuboid));
		CHECK(cuboids.getAdjacentsForVisionCuboid(eastCuboid).contains(southCuboid));
		CHECK(cuboids.getAdjacentsForVisionCuboid(southCuboid).contains(highCuboid));
		CHECK(cuboids.getAdjacentsForVisionCuboid(southCuboid).contains(eastCuboid));
		cuboids.pointIsTransparent(low);
		CHECK(cuboids.size() == 1);
		CHECK(cuboids.getVisionCuboidIndexForPoint(low).exists());
		VisionCuboidId cuboid = cuboids.getVisionCuboidIndexForPoint(high);
		CHECK(cuboids.getCuboidByVisionCuboidId(cuboid).volume() == 8);
		CHECK(cuboids.getAdjacentsForVisionCuboid(cuboid).empty());
	}
}
TEST_CASE("vision cuboid split")
{
		Simulation simulation;
	SUBCASE("split at")
	{
		Area& area = simulation.m_hasAreas->createArea(2,2,1);
		Point3D b1 = Point3D::create(0, 0, 0);
		Point3D b2 = Point3D::create(1, 0, 0);
		Point3D b3 = Point3D::create(0, 1, 0);
		Point3D b4 = Point3D::create(1, 1, 0);
		CHECK(area.m_visionCuboids.size() == 1);
		area.m_visionCuboids.remove(Cuboid{b4, b4});
		CHECK(area.m_visionCuboids.maybeGetVisionCuboidIndexForPoint(b4).empty());
		CHECK(area.m_visionCuboids.size() == 2);
		VisionCuboidId vc1 = area.m_visionCuboids.getVisionCuboidIndexForPoint(b1);
		VisionCuboidId vc2 = area.m_visionCuboids.getVisionCuboidIndexForPoint(b2);
		VisionCuboidId vc3 = area.m_visionCuboids.getVisionCuboidIndexForPoint(b3);
		if(vc1 == vc2)
		{
			assert(vc2 != vc3);
			CHECK(area.m_visionCuboids.getCuboidByVisionCuboidId(vc1).volume() == 2);
			CHECK(area.m_visionCuboids.getCuboidByVisionCuboidId(vc3).volume() == 1);
		}
		else
		{
			assert(vc1 == vc3);
			CHECK(area.m_visionCuboids.getCuboidByVisionCuboidId(vc1).volume() == 2);
			CHECK(area.m_visionCuboids.getCuboidByVisionCuboidId(vc2).volume() == 1);
		}
		CHECK(area.m_visionCuboids.maybeGetVisionCuboidIndexForPoint(b4).empty());
	}
	SUBCASE("split below")
	{
		Area& area = simulation.m_hasAreas->createArea(3,3,3);
		Space& space = area.getSpace();
		Point3D middle = Point3D::create(1, 1, 1);
		Point3D high = Point3D::create(2, 2, 2);
		Point3D low = Point3D::create(0, 0, 0);
		space.pointFeature_construct(middle, PointFeatureTypeId::Floor, MaterialType::byName("marble"));
		CHECK(area.m_visionCuboids.size() == 2);
		CHECK(area.m_visionCuboids.getVisionCuboidIndexForPoint(middle) == area.m_visionCuboids.getVisionCuboidIndexForPoint(high));
		CHECK(area.m_visionCuboids.getVisionCuboidIndexForPoint(middle) != area.m_visionCuboids.getVisionCuboidIndexForPoint(low));
		CHECK(area.m_visionCuboids.getCuboidForPoint(low).volume() == 9);
		CHECK(area.m_visionCuboids.getCuboidForPoint(middle).volume() == 18);
	}
}