TEST_CAST("locationBuckets")
{
	assert(s_locationBucketSize >= 6);
	Area area(10,10,10);
	Block& block = area.m_blocks[5][5][5];
	CHECK(block->m_locationBucket != nullptr);
	Actor a1(&block, s_oneByOneHalf, s_twoLegs);
	CHECK(block->m_locationBucket.contains(&a1));
	for(Block* adjacent : block->m_adjacentsVector)
		CHECK(block->m_locationBucket == adjacent->m_locationBucket);
	for(Actor* a : area.m_locationBuckets.inRange(block, 10))
		CHECK(a == &a1);
}
TEST_CAST("locationBuckets out of range")
{
	assert(s_locationBucketSize <= 25);
	Area area(30,30,30);
	Block& b1 = area.m_blocks[0][0][1];
	Actor a1(&block, s_oneByOneHalf, s_twoLegs);
	Block b2 = area.m_blocks[15][1][1];
	uint32_t actors = 0;
	for(Actor* actor : area.m_locationBuckets.inRange(b2, 15))
		++actors;
	CHECK(actors == 0);
	Block b3 = area.m_blocks[14][1][1];
	actors = 0;
	for(Actor* actor : area.m_locationBuckets.inRange(b3, 15))
		++actors;
	CHECK(actors == 1);
}
