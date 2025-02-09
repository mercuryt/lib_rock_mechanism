#include "../../lib/doctest.h"
#include "../../engine/geometry/cuboidSet.h"

TEST_CASE("cuboidSet")
{
	CuboidSet cuboidSet;
	CHECK(cuboidSet.empty());
	Cuboid firstCuboid(Point3D::create(2, 2, 2), Point3D::create(1, 1, 1));
	cuboidSet.add(firstCuboid);
	CHECK(!cuboidSet.empty());
	CHECK(cuboidSet.size() == 8);
	CHECK(cuboidSet.contains(Point3D::create(1, 2, 1)));
	SUBCASE("merge")
	{
		Cuboid secondCuboid = {Point3D::create(2, 2, 3), Point3D::create(1, 1, 3)};
		CHECK(firstCuboid.canMerge(secondCuboid));
		CHECK(!firstCuboid.intersects(secondCuboid));
		cuboidSet.add(secondCuboid);
		CHECK(cuboidSet.size() == 12);
		CHECK(cuboidSet.getCuboids().size() == 1);
	}
	SUBCASE("add")
	{
		Cuboid secondCuboid = {Point3D::create(2, 1, 3), Point3D::create(1, 1, 3)};
		CHECK(!firstCuboid.canMerge(secondCuboid));
		CHECK(!firstCuboid.intersects(secondCuboid));
		cuboidSet.add(secondCuboid);
		CHECK(cuboidSet.size() == 10);
		CHECK(cuboidSet.getCuboids().size() == 2);
	}
	SUBCASE("split")
	{
		Cuboid secondCuboid = {Point3D::create(1, 1, 1), Point3D::create(1, 1, 0)};
		CHECK(!firstCuboid.canMerge(secondCuboid));
		CHECK(firstCuboid.intersects(secondCuboid));
		cuboidSet.add(secondCuboid);
		CHECK(cuboidSet.size() == 9);
		CHECK(cuboidSet.getCuboids().size() == 4);
	}
}