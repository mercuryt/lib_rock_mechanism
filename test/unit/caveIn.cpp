#include "../../lib/doctest.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/definitions.h"
#include "../../src/simulation.h"
TEST_CASE("Cave In")
{
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	static const MaterialType& marble = MaterialType::byName("marble");
	SUBCASE("Cave In doesn't happen when block is supported.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		// Set a supported block to be solid, verify nothing happens.
		Block* block = &area.getBlock(5, 5, 1);
		block->setSolid(marble);
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
		Block* block = &area.getBlock(5, 5, 2);
		block->setSolid(marble);
		area.m_caveInCheck.insert(block);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 1);
		area.stepCaveInWrite();
		CHECK(!area.getBlock(5, 5, 2).isSolid());
		CHECK(area.getBlock(5, 5, 1).getSolidMaterial() == marble);
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
		Block* block = &area.getBlock(5, 5, 2);
		block->setSolid(marble);
		Block* block2 = &area.getBlock(4, 4, 2);
		block2->setSolid(marble);
		area.m_caveInCheck.insert(block);
		area.m_caveInCheck.insert(block2);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 2);
		area.stepCaveInWrite();
		CHECK(!block->isSolid());
		CHECK(!block2->isSolid());
		CHECK(block->m_adjacents[0]->getSolidMaterial() == marble);
		CHECK(block2->m_adjacents[0]->getSolidMaterial() == marble);
		CHECK(area.m_caveInCheck.size() == 2);
	}
	SUBCASE("Cave in connected blocks together.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		// Verify contiguous groups fall together.
		Block* block = &area.getBlock(5, 5, 2);
		block->setSolid(marble);
		Block* block2 = &area.getBlock(5, 6, 2);
		block2->setSolid(marble);
		area.m_caveInCheck.insert(block);
		area.m_caveInCheck.insert(block2);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 1);
		area.stepCaveInWrite();
		CHECK(!block->isSolid());
		CHECK(!block2->isSolid());
		CHECK(block->m_adjacents[0]->getSolidMaterial() == marble);
		CHECK(block2->m_adjacents[0]->getSolidMaterial() == marble);
		CHECK(area.m_caveInCheck.size() == 1);
	}
	SUBCASE("Blocks on the edge of the area are anchored.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		// Verify blocks on edges of area don't fall.
		Block* block = &area.getBlock(0, 5, 2);
		Block* block2 = &area.getBlock(1, 5, 2);
		block->setSolid(marble);
		block2->setSolid(marble);
		area.m_caveInCheck.insert(block);
		area.m_caveInCheck.insert(block2);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.empty());
		area.stepCaveInWrite();
		CHECK(area.m_caveInCheck.empty());
		CHECK(block->getSolidMaterial() == marble);
		CHECK(block2->getSolidMaterial() == marble);
		CHECK(!block->m_adjacents[0]->isSolid());
		CHECK(!block2->m_adjacents[0]->isSolid());
		CHECK(area.m_caveInCheck.size() == 0);
	}
	SUBCASE("Verify recorded fall distance is the shortest.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block* block = &area.getBlock(5, 5, 2);
		Block* block2 = &area.getBlock(5, 5, 3);
		Block* block3 = &area.getBlock(6, 5, 3);
		block->setSolid(marble);
		block2->setSolid(marble);
		block3->setSolid(marble);
		area.m_caveInCheck.insert(block);
		area.m_caveInCheck.insert(block2);
		area.m_caveInCheck.insert(block3);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 1);
		CHECK(std::get<0>(*area.m_caveInData.begin()).size() == 3);
		CHECK(std::get<1>(*area.m_caveInData.begin()) == 1);
		area.stepCaveInWrite();
		CHECK(block->getSolidMaterial() == marble);
		CHECK(block->m_adjacents[0]->getSolidMaterial() == marble);
		CHECK(!block2->isSolid());
		CHECK(block2->m_adjacents[0]->getSolidMaterial() == marble);
		CHECK(!block3->isSolid());
		CHECK(block3->m_adjacents[0]->getSolidMaterial() == marble);
		CHECK(area.m_caveInCheck.size() == 1);
	}
	SUBCASE("Verify one group falling onto another unanchored group will keep falling in the next step.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block* block = &area.getBlock(5, 5, 2);
		Block* block2 = &area.getBlock(5, 5, 4);
		block->setSolid(marble);
		block2->setSolid(marble);
		area.m_caveInCheck.insert(block);
		area.m_caveInCheck.insert(block2);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 2);
		area.stepCaveInWrite();
		CHECK(area.m_caveInCheck.size() == 2);
		CHECK(!block->isSolid());
		CHECK(!block2->isSolid());
		CHECK(block->m_adjacents[0]->getSolidMaterial() == marble);
		CHECK(block2->m_adjacents[0]->getSolidMaterial() == marble);
		area.stepCaveInRead();
		CHECK(area.m_caveInData.size() == 1);
		area.stepCaveInWrite();
		CHECK(block->getSolidMaterial() == marble);
		CHECK(block->m_adjacents[0]->getSolidMaterial() == marble);
		CHECK(!block2->isSolid());
		CHECK(!block2->m_adjacents[0]->isSolid());
		CHECK(area.m_caveInCheck.size() == 1);
	}
}
