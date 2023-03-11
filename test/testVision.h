TEST_CASE("See no one when no one is present to be seen")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block = area.m_blocks[5][5][1];
	Actor actor(&block, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(&actor);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.empty());
}
TEST_CASE("See someone nearby")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block1 = area.m_blocks[3][3][1];
	Block& block2 = area.m_blocks[7][7][1];
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block2, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(&a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
	CHECK(visionRequest.m_actors.contains(&a2));
}
TEST_CASE("Too far to see")
{
	Area area(20,20,20);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block1 = area.m_blocks[0][0][1];
	Block& block2 = area.m_blocks[19][19][1];
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block2, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(&a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 0);
}
TEST_CASE("Vision blocked by wall")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block1 = area.m_blocks[5][3][1];
	Block& block2 = area.m_blocks[5][5][1];
	Block& block3 = area.m_blocks[5][7][1];
	block2.setSolid(s_stone);
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block3, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(&a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 0);
}
