#include "../../lib/doctest.h"
#include "../../engine/space/nthAdjacentOffsets.h"
#include "../../engine/geometry/point3D.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/space/space.h"
#include <algorithm>
TEST_CASE("getNthAdjacentOffsets")
{
	auto result = getNthAdjacentOffsets(1);
	CHECK(result.size() == 6);
	CHECK(std::ranges::find(result, Offset3D(0,0,1)) != result.end());
	CHECK(std::ranges::find(result, Offset3D(0,0,-1)) != result.end());
	CHECK(std::ranges::find(result, Offset3D(0,1,0)) != result.end());
	CHECK(std::ranges::find(result, Offset3D(0,-1,0)) != result.end());
	CHECK(std::ranges::find(result, Offset3D(1,0,0)) != result.end());
	CHECK(std::ranges::find(result, Offset3D(-1,0,0)) != result.end());
	CHECK(std::ranges::find(result, Offset3D(-2,0,0)) == result.end());
	CHECK(std::ranges::find(result, Offset3D(0,0,0)) == result.end());
	CHECK(std::ranges::find(result, Offset3D(0,0,2)) == result.end());
	CHECK(std::ranges::find(result, Offset3D(0,2,0)) == result.end());
	result = getNthAdjacentOffsets(3);
	CHECK(result.size() == 38);
	result = getNthAdjacentOffsets(4);
	CHECK(result.size() == 66);
	result = getNthAdjacentOffsets(5);
	CHECK(result.size() == 102);
	result = getNthAdjacentOffsets(6);
	CHECK(result.size() == 146);
}
TEST_CASE("getNthAdjacentBlocks")
{
	Simulation simulation;
	Area& area = simulation.getAreas().createArea(8, 8, 8);
	Space& space = area.getSpace();
	const Point3D center = Point3D::create(4,4,4);
	const Point3D above1 = Point3D::create(4,4,5);
	const Point3D above2 = Point3D::create(4,4,6);
	const Point3D above3 = Point3D::create(4,4,7);
	const Point3D edge = Point3D::create(0,4,4);
	const Point3D nextToEdge = Point3D::create(0,3,4);
	const Point3D nextToEdge2 = Point3D::create(0,2,4);
	const Point3D corner = Point3D::create(0,0,4);
	auto result = space.getNthAdjacent(center, Distance::create(1));
	CHECK(result.size() == 6);
	CHECK(result.contains(above1));
	CHECK(!result.contains(above2));
	CHECK(!result.contains(above3));
	CHECK(!result.contains(center));
	result = space.getNthAdjacent(center, Distance::create(2));
	CHECK(result.size() == 18);
	CHECK(!result.contains(above1));
	CHECK(result.contains(above2));
	CHECK(!result.contains(above3));
	CHECK(!result.contains(center));
	result = space.getNthAdjacent(center, Distance::create(3));
	CHECK(result.size() == 38);
	CHECK(!result.contains(above1));
	CHECK(!result.contains(above2));
	CHECK(result.contains(above3));
	CHECK(!result.contains(center));
	CHECK(!result.contains(edge));
	result = space.getNthAdjacent(center, Distance::create(4));
	CHECK(result.size() == 63);
	CHECK(result.contains(edge));
	CHECK(!result.contains(nextToEdge));
	CHECK(!result.contains(corner));
	result = space.getNthAdjacent(center, Distance::create(5));
	CHECK(result.size() == 84);
	CHECK(!result.contains(corner));
	CHECK(!result.contains(edge));
	CHECK(result.contains(nextToEdge));
	CHECK(!result.contains(nextToEdge2));
	result = space.getNthAdjacent(center, Distance::create(6));
	CHECK(result.size() == 92);
	CHECK(!result.contains(corner));
	CHECK(!result.contains(edge));
	CHECK(!result.contains(nextToEdge));
	CHECK(result.contains(nextToEdge2));
}