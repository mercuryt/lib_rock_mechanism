#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/definitions.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
TEST_CASE("Cave In")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	static const MaterialType& marble = MaterialType::byName("marble");
	SUBCASE("Cave In doesn't happen when block is supported.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		// Set a supported block to be solid, verify nothing happens.
		BlockIndex block = blocks.getIndex({5, 5, 1});
		blocks.solid_set(block, marble, false);
		area.m_caveInCheck.insert(block);
		area.stepCaveInRead();
		CHECK(area.m_caveInCheck.empty());
		CHECK(area.m_caveInData.empty());
		area.stepCaveInWrite();
		CHECK(area.m_caveInCheck.size() == 0);
	}
	SUBCASE("Cave in does happen when block is not supported.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		// Set a floating block to be solid and add to caveInCheck, verify it falls.
		BlockIndex block = blocks.getIndex({5, 5, 2});
		blocks.solid_set(block, marble, false);
		area.m_caveInCheck.insert(block);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 1);
		area.stepCaveInWrite();
		CHECK(!blocks.solid_is(blocks.getIndex({5, 5, 2})));
		CHECK(blocks.solid_get(blocks.getIndex({5, 5, 1})) == marble);
		CHECK(area.m_caveInCheck.size() == 1);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 0);
		area.stepCaveInWrite();
		CHECK(area.m_caveInCheck.size() == 0);
	}
	SUBCASE("Cave in does happen to multiple unconnected blocks which are unsuported.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		// Verify multiple seperate blocks fall.
		BlockIndex block = blocks.getIndex({5, 5, 2});
		blocks.solid_set(block, marble, false);
		BlockIndex block2 = blocks.getIndex({4, 4, 2});
		blocks.solid_set(block2, marble, false);
		area.m_caveInCheck.insert(block);
		area.m_caveInCheck.insert(block2);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 2);
		area.stepCaveInWrite();
		CHECK(!blocks.solid_is(block));
		CHECK(!blocks.solid_is(block2));
		CHECK(blocks.solid_get(blocks.getBlockBelow(block)) == marble);
		CHECK(blocks.solid_get(blocks.getBlockBelow(block2)) == marble);
		CHECK(area.m_caveInCheck.size() == 2);
	}
	SUBCASE("Cave in connected blocks together.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		// Verify contiguous groups fall together.
		BlockIndex block = blocks.getIndex({5, 5, 2});
		blocks.solid_set(block, marble, false);
		BlockIndex block2 = blocks.getIndex({5, 6, 2});
		blocks.solid_set(block2, marble, false);
		area.m_caveInCheck.insert(block);
		area.m_caveInCheck.insert(block2);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 1);
		area.stepCaveInWrite();
		CHECK(!blocks.solid_is(block));
		CHECK(!blocks.solid_is(block2));
		CHECK(blocks.solid_get(blocks.getBlockBelow(block)) == marble);
		CHECK(blocks.solid_get(blocks.getBlockBelow(block2)) == marble);
		CHECK(area.m_caveInCheck.size() == 1);
	}
	SUBCASE("Blocks on the edge of the area are anchored.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		// Verify blocks on edges of area don't fall.
		BlockIndex block = blocks.getIndex({0, 5, 2});
		BlockIndex block2 = blocks.getIndex({1, 5, 2});
		blocks.solid_set(block, marble, false);
		blocks.solid_set(block2, marble, false);
		area.m_caveInCheck.insert(block);
		area.m_caveInCheck.insert(block2);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.empty());
		area.stepCaveInWrite();
		CHECK(area.m_caveInCheck.empty());
		CHECK(blocks.solid_get(block) == marble);
		CHECK(blocks.solid_get(block2) == marble);
		CHECK(!blocks.solid_is(blocks.getBlockBelow(block)));
		CHECK(!blocks.solid_is(blocks.getBlockBelow(block2)));
		CHECK(area.m_caveInCheck.size() == 0);
	}
	SUBCASE("Verify recorded fall distance is the shortest.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block = blocks.getIndex({5, 5, 2});
		BlockIndex block2 = blocks.getIndex({5, 5, 3});
		BlockIndex block3 = blocks.getIndex({6, 5, 3});
		blocks.solid_set(block, marble, false);
		blocks.solid_set(block2, marble, false);
		blocks.solid_set(block3, marble, false);
		area.m_caveInCheck.insert(block);
		area.m_caveInCheck.insert(block2);
		area.m_caveInCheck.insert(block3);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 1);
		CHECK(std::get<0>(*area.m_caveInData.begin()).size() == 3);
		CHECK(std::get<1>(*area.m_caveInData.begin()) == 1);
		area.stepCaveInWrite();
		CHECK(blocks.solid_get(block) == marble);
		CHECK(blocks.solid_get(blocks.getBlockBelow(block)) == marble);
		CHECK(!blocks.solid_is(block2));
		CHECK(blocks.solid_get(blocks.getBlockBelow(block2)) == marble);
		CHECK(!blocks.solid_is(block3));
		CHECK(blocks.solid_get(blocks.getBlockBelow(block3)) == marble);
		CHECK(area.m_caveInCheck.size() == 1);
	}
	SUBCASE("Verify one group falling onto another unanchored group will keep falling in the next step.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block = blocks.getIndex({5, 5, 2});
		BlockIndex block2 = blocks.getIndex({5, 5, 4});
		blocks.solid_set(block, marble, false);
		blocks.solid_set(block2, marble, false);
		area.m_caveInCheck.insert(block);
		area.m_caveInCheck.insert(block2);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 2);
		area.stepCaveInWrite();
		CHECK(area.m_caveInCheck.size() == 2);
		CHECK(!blocks.solid_is(block));
		CHECK(!blocks.solid_is(block2));
		CHECK(blocks.solid_get(blocks.getBlockBelow(block)) == marble);
		CHECK(blocks.solid_get(blocks.getBlockBelow(block2)) == marble);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 1);
		area.stepCaveInWrite();
		CHECK(blocks.solid_get(blocks.getBlockBelow(block)) == marble);
		CHECK(blocks.solid_get(blocks.getBlockBelow(block)) == marble);
		CHECK(!blocks.solid_is(block2));
		CHECK(!blocks.solid_is(blocks.getBlockBelow(block2)));
		CHECK(area.m_caveInCheck.size() == 1);
	}
}
