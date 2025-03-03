#include "../../lib/doctest.h"
#include "../../engine/blocks/nthAdjacentOffsets.h"
#include "../../engine/geometry/point3D.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/blocks/blocks.h"
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
	Blocks& blocks = area.getBlocks();
	const BlockIndex center = blocks.getIndex_i(4,4,4);
	const BlockIndex above1 = blocks.getIndex_i(4,4,5);
	const BlockIndex above2 = blocks.getIndex_i(4,4,6);
	const BlockIndex above3 = blocks.getIndex_i(4,4,7);
	const BlockIndex edge = blocks.getIndex_i(0,4,4);
	const BlockIndex nextToEdge = blocks.getIndex_i(0,3,4);
	const BlockIndex nextToEdge2 = blocks.getIndex_i(0,2,4);
	const BlockIndex corner = blocks.getIndex_i(0,0,4);
	auto result = blocks.getNthAdjacent(center, DistanceInBlocks::create(1));
	CHECK(result.size() == 6);
	CHECK(result.contains(above1));
	CHECK(!result.contains(above2));
	CHECK(!result.contains(above3));
	CHECK(!result.contains(center));
	result = blocks.getNthAdjacent(center, DistanceInBlocks::create(2));
	CHECK(result.size() == 18);
	CHECK(!result.contains(above1));
	CHECK(result.contains(above2));
	CHECK(!result.contains(above3));
	CHECK(!result.contains(center));
	result = blocks.getNthAdjacent(center, DistanceInBlocks::create(3));
	CHECK(result.size() == 38);
	CHECK(!result.contains(above1));
	CHECK(!result.contains(above2));
	CHECK(result.contains(above3));
	CHECK(!result.contains(center));
	CHECK(!result.contains(edge));
	result = blocks.getNthAdjacent(center, DistanceInBlocks::create(4));
	CHECK(result.size() == 63);
	CHECK(result.contains(edge));
	CHECK(!result.contains(nextToEdge));
	CHECK(!result.contains(corner));
	result = blocks.getNthAdjacent(center, DistanceInBlocks::create(5));
	CHECK(result.size() == 84);
	CHECK(!result.contains(corner));
	CHECK(!result.contains(edge));
	CHECK(result.contains(nextToEdge));
	CHECK(!result.contains(nextToEdge2));
	result = blocks.getNthAdjacent(center, DistanceInBlocks::create(6));
	CHECK(result.size() == 92);
	CHECK(!result.contains(corner));
	CHECK(!result.contains(edge));
	CHECK(!result.contains(nextToEdge));
	CHECK(result.contains(nextToEdge2));
}