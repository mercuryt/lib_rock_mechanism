#include "../../lib/doctest.h"
#include "../../engine/geometry/cuboidSet.h"

TEST_CASE("cuboidSet")
{
	CuboidSet cuboidSet;
	REQUIRE(cuboidSet.empty());
	Cuboid firstCuboid(Point3D::create(2, 2, 2), Point3D::create(1, 1, 1));
	cuboidSet.add(firstCuboid);
	REQUIRE(!cuboidSet.empty());
	REQUIRE(cuboidSet.size() == 8);
	REQUIRE(cuboidSet.contains(Point3D::create(1, 2, 1)));
	SUBCASE("merge")
	{
		Cuboid secondCuboid = {Point3D::create(2, 2, 3), Point3D::create(1, 1, 3)};
		REQUIRE(firstCuboid.canMerge(secondCuboid));
		REQUIRE(!firstCuboid.overlapsWith(secondCuboid));
		cuboidSet.add(secondCuboid);
		REQUIRE(cuboidSet.size() == 12);
		REQUIRE(cuboidSet.getCuboids().size() == 1);
	}
	SUBCASE("add")
	{
		Cuboid secondCuboid = {Point3D::create(2, 1, 3), Point3D::create(1, 1, 3)};
		REQUIRE(!firstCuboid.canMerge(secondCuboid));
		REQUIRE(!firstCuboid.overlapsWith(secondCuboid));
		cuboidSet.add(secondCuboid);
		REQUIRE(cuboidSet.size() == 10);
		REQUIRE(cuboidSet.getCuboids().size() == 2);
	}
	SUBCASE("split")
	{
		Cuboid secondCuboid = {Point3D::create(1, 1, 1), Point3D::create(1, 1, 0)};
		REQUIRE(!firstCuboid.canMerge(secondCuboid));
		REQUIRE(firstCuboid.overlapsWith(secondCuboid));
		cuboidSet.add(secondCuboid);
		REQUIRE(cuboidSet.size() == 9);
		REQUIRE(cuboidSet.getCuboids().size() == 4);
	}
}