#include "testShared.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../lib/doctest.h"


TEST_CASE("Make Area")
{
	Area area(100,100,100);
	CHECK(area.m_sizeX == 100);
	CHECK(area.m_sizeY == 100);
	CHECK(area.m_sizeZ == 100);
	registerTypes();
	SetSolidLayer(area, 99, s_stone);
	CHECK(area.m_blocks[50][50][99].m_solid == s_stone);
	area.readStep();
	area.writeStep();
	CHECK(area.m_caveInCheck.empty());
}

TEST_CASE("Cave In")
{
	Area area(100,100,100);
	registerTypes();
	SetSolidLayer(area, 99, s_stone);
	// Set a supported block to be solid, verify nothing happens.
	Block* block = &area.m_blocks[50][50][98];
	block->m_solid = s_stone;
	area.m_caveInCheck.insert(block);
	area.readStep();
	CHECK(area.m_caveInCheck.empty());
	CHECK(area.m_caveInData.empty());
	area.writeStep();
	block->m_solid = nullptr;
	// Set a floating block to be solid and add to caveInCheck, verify it falls.
	block = &area.m_blocks[50][50][97];
	block->m_solid = s_stone;
	area.m_caveInCheck.insert(block);
	area.readStep();
	CHECK(area.m_caveInData.size() == 1);
	area.m_caveInCheck.insert(block);
	area.m_caveInCheck.insert(block);
	area.writeStep();
	CHECK(area.m_blocks[50][50][97].m_solid == nullptr);
	CHECK(area.m_blocks[50][50][98].m_solid == s_stone);
	area.m_blocks[50][50][98].m_solid = nullptr;
	area.m_caveInCheck.clear();
	// Verify multiple seperate blocks fall.
	block = &area.m_blocks[50][50][97];
	block->m_solid = s_stone;
	Block* block2 = &area.m_blocks[40][40][97];
	block2->m_solid = s_stone;
	area.m_caveInCheck.insert(block);
	area.m_caveInCheck.insert(block2);
	area.readStep();
	CHECK(area.m_caveInData.size() == 2);
	area.writeStep();
	CHECK(block->m_solid == nullptr);
	CHECK(block2->m_solid == nullptr);
	CHECK(block->m_adjacents[0]->m_solid == s_stone);
	CHECK(block2->m_adjacents[0]->m_solid == s_stone);
	block->m_adjacents[0]->m_solid == nullptr;
	block2->m_adjacents[0]->m_solid == nullptr;
	area.m_caveInCheck.clear();
	// Verify contiguous groups fall together.
	block->m_solid = s_stone;
	block2 = &area.m_blocks[50][51][97];
	block2->m_solid = s_stone;
	area.m_caveInCheck.insert(block);
	area.m_caveInCheck.insert(block2);
	area.readStep();
	CHECK(area.m_caveInData.size() == 1);
	area.writeStep();
	CHECK(block->m_solid == nullptr);
	CHECK(block2->m_solid == nullptr);
	CHECK(block->m_adjacents[0]->m_solid == s_stone);
	CHECK(block2->m_adjacents[0]->m_solid == s_stone);
	block->m_adjacents[0]->m_solid == nullptr;
	block2->m_adjacents[0]->m_solid == nullptr;
	area.m_caveInCheck.clear();
	// Verify blocks on edges of area don't fall.
	block = &area.m_blocks[0][50][97];
	block->m_solid == s_stone;
	area.m_caveInCheck.insert(block);
	area.readStep();
	CHECK(area.m_caveInData.empty());
	area.writeStep();
	CHECK(area.m_caveInCheck.empty());
	CHECK(block->m_solid == s_stone);
	block->m_solid = nullptr;
}
