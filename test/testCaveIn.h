TEST_CASE("Cave In doesn't happen when block is supported.")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	// Set a supported block to be solid, verify nothing happens.
	Block* block = &area.m_blocks[50][50][1];
	block->setSolid(s_stone);
	area.m_caveInCheck.insert(block);
	area.stepCaveInRead();
	CHECK(area.m_caveInCheck.empty());
	CHECK(area.m_caveInData.empty());
	area.stepCaveInWrite();
	CHECK(area.m_caveInCheck.size() == 0);
}
TEST_CASE("Cave in does happen when block is not supported.")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	// Set a floating block to be solid and add to caveInCheck, verify it falls.
	Block* block = &area.m_blocks[50][50][2];
	block->setSolid(s_stone);
	area.m_caveInCheck.insert(block);
	area.stepCaveInRead();
	CHECK(area.m_caveInData.size() == 1);
	area.stepCaveInWrite();
	CHECK(!area.m_blocks[50][50][2].isSolid());
	CHECK(area.m_blocks[50][50][1].getSolidMaterial() == s_stone);
	CHECK(area.m_caveInCheck.size() == 1);
	area.stepCaveInRead();
	CHECK(area.m_caveInData.size() == 0);
	area.stepCaveInWrite();
	CHECK(area.m_caveInCheck.size() == 0);
}
TEST_CASE("Cave in does happen to multiple unconnected blocks which are unsuported.")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	// Verify multiple seperate blocks fall.
	Block* block = &area.m_blocks[50][50][2];
	block->setSolid(s_stone);
	Block* block2 = &area.m_blocks[40][40][2];
	block2->setSolid(s_stone);
	area.m_caveInCheck.insert(block);
	area.m_caveInCheck.insert(block2);
	area.stepCaveInRead();
	CHECK(area.m_caveInData.size() == 2);
	area.stepCaveInWrite();
	CHECK(!block->isSolid());
	CHECK(!block2->isSolid());
	CHECK(block->m_adjacents[0]->getSolidMaterial() == s_stone);
	CHECK(block2->m_adjacents[0]->getSolidMaterial() == s_stone);
	CHECK(area.m_caveInCheck.size() == 2);
}
TEST_CASE("Cave in connected blocks together.")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	// Verify contiguous groups fall together.
	Block* block = &area.m_blocks[50][50][2];
	block->setSolid(s_stone);
	Block* block2 = &area.m_blocks[50][51][2];
	block2->setSolid(s_stone);
	area.m_caveInCheck.insert(block);
	area.m_caveInCheck.insert(block2);
	area.stepCaveInRead();
	CHECK(area.m_caveInData.size() == 1);
	area.stepCaveInWrite();
	CHECK(!block->isSolid());
	CHECK(!block2->isSolid());
	CHECK(block->m_adjacents[0]->getSolidMaterial() == s_stone);
	CHECK(block2->m_adjacents[0]->getSolidMaterial() == s_stone);
	CHECK(area.m_caveInCheck.size() == 1);
}
TEST_CASE("Blocks on the edge of the area are anchored.")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	// Verify blocks on edges of area don't fall.
	Block* block = &area.m_blocks[0][50][2];
	Block* block2 = &area.m_blocks[1][50][2];
	block->setSolid(s_stone);
	block2->setSolid(s_stone);
	area.m_caveInCheck.insert(block);
	area.m_caveInCheck.insert(block2);
	area.stepCaveInRead();
	CHECK(area.m_caveInData.empty());
	area.stepCaveInWrite();
	CHECK(area.m_caveInCheck.empty());
	CHECK(block->getSolidMaterial() == s_stone);
	CHECK(block2->getSolidMaterial() == s_stone);
	CHECK(!block->m_adjacents[0]->isSolid());
	CHECK(!block2->m_adjacents[0]->isSolid());
	CHECK(area.m_caveInCheck.size() == 0);
}
TEST_CASE("Verify recorded fall distance is the shortest.")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block* block = &area.m_blocks[50][50][2];
	Block* block2 = &area.m_blocks[50][50][3];
	Block* block3 = &area.m_blocks[51][50][3];
	block->setSolid(s_stone);
	block2->setSolid(s_stone);
	block3->setSolid(s_stone);
	area.m_caveInCheck.insert(block);
	area.m_caveInCheck.insert(block2);
	area.m_caveInCheck.insert(block3);
	area.stepCaveInRead();
	CHECK(area.m_caveInData.size() == 1);
	CHECK(std::get<0>(*area.m_caveInData.begin()).size() == 3);
	CHECK(std::get<1>(*area.m_caveInData.begin()) == 1);
	area.stepCaveInWrite();
	CHECK(block->getSolidMaterial() == s_stone);
	CHECK(block->m_adjacents[0]->getSolidMaterial() == s_stone);
	CHECK(!block2->isSolid());
	CHECK(block2->m_adjacents[0]->getSolidMaterial() == s_stone);
	CHECK(!block3->isSolid());
	CHECK(block3->m_adjacents[0]->getSolidMaterial() == s_stone);
	CHECK(area.m_caveInCheck.size() == 1);
}
TEST_CASE("Verify one group falling onto another unanchored group will keep falling in the next step.")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block* block = &area.m_blocks[50][50][2];
	Block* block2 = &area.m_blocks[50][50][4];
	block->setSolid(s_stone);
	block2->setSolid(s_stone);
	area.m_caveInCheck.insert(block);
	area.m_caveInCheck.insert(block2);
	area.stepCaveInRead();
	CHECK(area.m_caveInData.size() == 2);
	area.stepCaveInWrite();
	CHECK(area.m_caveInCheck.size() == 2);
	CHECK(!block->isSolid());
	CHECK(!block2->isSolid());
	CHECK(block->m_adjacents[0]->getSolidMaterial() == s_stone);
	CHECK(block2->m_adjacents[0]->getSolidMaterial() == s_stone);
	area.stepCaveInRead();
	CHECK(area.m_caveInData.size() == 1);
	area.stepCaveInWrite();
	CHECK(block->getSolidMaterial() == s_stone);
	CHECK(block->m_adjacents[0]->getSolidMaterial() == s_stone);
	CHECK(!block2->isSolid());
	CHECK(!block2->m_adjacents[0]->isSolid());
	CHECK(area.m_caveInCheck.size() == 1);
}
