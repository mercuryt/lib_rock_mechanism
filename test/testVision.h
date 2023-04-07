TEST_CASE("See no one when no one is present to be seen")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block = area.m_blocks[5][5][1];
	Actor actor(&block, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(actor);
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
	VisionRequest visionRequest(a1);
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
	VisionRequest visionRequest(a1);
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
	VisionRequest visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 0);
}
TEST_CASE("Vision not blocked by wall not directly in the line of sight")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block1 = area.m_blocks[5][3][1];
	Block& block2 = area.m_blocks[5][5][1];
	Block& block3 = area.m_blocks[6][6][1];
	block2.setSolid(s_stone);
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block3, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
}
TEST_CASE("Vision not blocked by one by one wall for two by two shape")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block1 = area.m_blocks[5][3][1];
	Block& block2 = area.m_blocks[5][5][1];
	Block& block3 = area.m_blocks[5][7][1];
	block2.setSolid(s_stone);
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block3, s_twoByTwoFull, s_twoLegs);
	VisionRequest visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
}
TEST_CASE("Vision not blocked by glass wall")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block1 = area.m_blocks[5][3][1];
	Block& block2 = area.m_blocks[5][5][1];
	Block& block3 = area.m_blocks[5][7][1];
	block2.setSolid(s_glass);
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block3, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
}
TEST_CASE("Vision blocked by closed door")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block1 = area.m_blocks[5][3][1];
	Block& block2 = area.m_blocks[5][5][1];
	Block& block3 = area.m_blocks[5][7][1];
	block2.addConstructedFeature(s_door, s_stone);
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block3, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 0);
	block2.getFeatureByType(s_door)->closed = false;
	VisionRequest visionRequest2(a1);
	visionRequest2.readStep();
	CHECK(visionRequest2.m_actors.size() == 1);
}
TEST_CASE("Vision from above and below blocked by closed hatch")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 3, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	Block& block3 = area.m_blocks[5][5][3];
	block1.setNotSolid();
	block2.setNotSolid();
	block3.setNotSolid();
	block3.addConstructedFeature(s_hatch, s_stone);
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block3, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 0);
	VisionRequest visionRequest2(a1);
	visionRequest2.readStep();
	CHECK(visionRequest2.m_actors.size() == 0);
	block3.getFeatureByType(s_hatch)->closed = false;
	VisionRequest visionRequest3(a1);
	visionRequest3.readStep();
	CHECK(visionRequest3.m_actors.size() == 1);
}
TEST_CASE("Vision not blocked by closed hatch on the same z level")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block1 = area.m_blocks[5][3][1];
	Block& block2 = area.m_blocks[5][5][1];
	Block& block3 = area.m_blocks[5][7][1];
	block2.addConstructedFeature(s_hatch, s_stone);
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block3, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
}
TEST_CASE("Vision from above and below blocked by floor")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 3, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	Block& block3 = area.m_blocks[5][5][3];
	block1.setNotSolid();
	block2.setNotSolid();
	block3.setNotSolid();
	block2.addConstructedFeature(s_upStairs, s_stone);
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block3, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
	block3.addConstructedFeature(s_floor, s_stone);
	VisionRequest visionRequest2(a1);
	visionRequest2.readStep();
	CHECK(visionRequest2.m_actors.size() == 0);
	VisionRequest visionRequest3(a2);
	visionRequest3.readStep();
	CHECK(visionRequest3.m_actors.size() == 0);
}
TEST_CASE("Vision from below not blocked by glass floor")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 3, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	Block& block3 = area.m_blocks[5][5][3];
	block1.setNotSolid();
	block2.setNotSolid();
	block3.setNotSolid();
	block2.addConstructedFeature(s_upStairs, s_stone);
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block3, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
	block3.addConstructedFeature(s_floor, s_glass);
	VisionRequest visionRequest2(a1);
	visionRequest2.readStep();
	CHECK(visionRequest2.m_actors.size() == 1);
}
TEST_CASE("Vision not blocked by floor on the same z level")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block1 = area.m_blocks[5][3][1];
	Block& block2 = area.m_blocks[5][5][1];
	Block& block3 = area.m_blocks[5][7][1];
	block2.addConstructedFeature(s_floor, s_stone);
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block3, s_oneByOneFull, s_twoLegs);
	VisionRequest visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
}
