#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/definitions/definitions.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/space/space.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
TEST_CASE("caveIn")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Space& space = area.getSpace();
	Support& support = space.getSupport();
	static MaterialTypeId marble = MaterialType::byName("marble");
	SUBCASE("Cave In doesn't happen when block is supported.")
	{
		areaBuilderUtil::setSolidLayer(area, Distance::create(0), marble);
		// Set a supported block to be solid, verify nothing happens.
		Point3D block = Point3D::create(5, 5, 1);
		space.solid_set(block, marble, false);
		support.maybeFall({block, block});
		support.doStep(area);
		CHECK(support.maybeFallVolume() == 0);
	}
	SUBCASE("Cave in does happen when block is not supported.")
	{
		areaBuilderUtil::setSolidLayer(area, Distance::create(0), marble);
		// Set a floating block to be solid and add to caveInCheck, verify it falls.
		Point3D block = Point3D::create(5, 5, 2);
		space.solid_set(block, marble, false);
		support.maybeFall({block, block});
		support.doStep(area);
		CHECK(!space.solid_isAny(Point3D::create(5, 5, 2)));
		CHECK(space.solid_get(Point3D::create(5, 5, 1)) == marble);
		CHECK(support.maybeFallVolume() == 0);
	}
	SUBCASE("Cave in does happen to multiple unconnected blocks which are unsuported.")
	{
		areaBuilderUtil::setSolidLayer(area, Distance::create(0), marble);
		// Verify multiple seperate blocks fall.
		Point3D block = Point3D::create(5, 5, 2);
		space.solid_set(block, marble, false);
		Point3D block2 = Point3D::create(4, 4, 2);
		space.solid_set(block2, marble, false);
		support.maybeFall({block, block});
		support.maybeFall({block2, block2});
		support.doStep(area);
		CHECK(!space.solid_isAny(block));
		CHECK(!space.solid_isAny(block2));
		CHECK(space.solid_get(block.below()) == marble);
		CHECK(space.solid_get(block2.below()) == marble);
		CHECK(support.maybeFallVolume() == 0);
	}
	SUBCASE("Cave in connected blocks together.")
	{
		areaBuilderUtil::setSolidLayer(area, Distance::create(0), marble);
		// Verify contiguous groups fall together.
		Point3D block = Point3D::create(5, 5, 2);
		space.solid_set(block, marble, false);
		Point3D block2 = Point3D::create(5, 6, 2);
		space.solid_set(block2, marble, false);
		support.maybeFall({block, block});
		support.maybeFall({block2, block2});
		support.doStep(area);
		CHECK(!space.solid_isAny(block));
		CHECK(!space.solid_isAny(block2));
		CHECK(space.solid_get(block.below()) == marble);
		CHECK(space.solid_get(block2.below()) == marble);
		CHECK(support.maybeFallVolume() == 0);
	}
	SUBCASE("Blocks on the edge of the area are anchored.")
	{
		areaBuilderUtil::setSolidLayer(area, Distance::create(0), marble);
		// Verify blocks on edges of area don't fall.
		Point3D block = Point3D::create(0, 5, 2);
		Point3D block2 = Point3D::create(1, 5, 2);
		space.solid_set(block, marble, false);
		space.solid_set(block2, marble, false);
		support.maybeFall({block, block});
		support.maybeFall({block2, block2});
		support.doStep(area);
		CHECK(space.solid_get(block) == marble);
		CHECK(space.solid_get(block2) == marble);
		CHECK(!space.solid_isAny(block.below()));
		CHECK(!space.solid_isAny(block2.below()));
		CHECK(support.maybeFallVolume() == 0);
	}
	SUBCASE("Verify recorded fall distance is the shortest.")
	{
		areaBuilderUtil::setSolidLayer(area, Distance::create(0), marble);
		Point3D block = Point3D::create(5, 5, 2);
		Point3D block2 = Point3D::create(5, 5, 3);
		Point3D block3 = Point3D::create(6, 5, 3);
		space.solid_set(block, marble, false);
		space.solid_set(block2, marble, false);
		space.solid_set(block3, marble, false);
		support.maybeFall({block, block});
		support.maybeFall({block2, block2});
		support.maybeFall({block2, block2});
		support.doStep(area);
		CHECK(space.solid_get(block) == marble);
		CHECK(space.solid_get(block.below()) == marble);
		CHECK(!space.solid_isAny(block2));
		CHECK(space.solid_get(block2.below()) == marble);
		CHECK(!space.solid_isAny(block3));
		CHECK(space.solid_get(block3.below()) == marble);
		CHECK(support.maybeFallVolume() == 0);
	}
	SUBCASE("Verify one group falling onto another unanchored group will keep falling.")
	{
		areaBuilderUtil::setSolidLayer(area, Distance::create(0), marble);
		Point3D block = Point3D::create(5, 5, 2);
		Point3D block2 = Point3D::create(5, 5, 4);
		space.solid_set(block, marble, false);
		space.solid_set(block2, marble, false);
		support.maybeFall({block, block});
		support.maybeFall({block2, block2});
		support.doStep(area);
		CHECK(space.solid_get(block.below()) == marble);
		CHECK(!space.solid_isAny(block2));
		CHECK(!space.solid_isAny(block2.below()));
		CHECK(space.solid_isAny(block));
		CHECK(space.solid_isAny(block.below()));
		CHECK(support.maybeFallVolume() == 0);
	}
}
