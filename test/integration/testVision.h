TEST_CASE("See no one when no one is present to be seen")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block = area.m_blocks[5][5][1];
	Actor actor(&block, s_oneByOneFull, s_twoLegs);
	VisionRequest<Block, Actor, Area> visionRequest(actor);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 0);
	block2.getFeatureByType(s_door)->closed = false;
	VisionRequest<Block, Actor, Area> visionRequest2(a1);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 0);
	VisionRequest<Block, Actor, Area> visionRequest2(a1);
	visionRequest2.readStep();
	CHECK(visionRequest2.m_actors.size() == 0);
	block3.getFeatureByType(s_hatch)->closed = false;
	VisionRequest<Block, Actor, Area> visionRequest3(a1);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
	block3.addConstructedFeature(s_floor, s_stone);
	VisionRequest<Block, Actor, Area> visionRequest2(a1);
	visionRequest2.readStep();
	CHECK(visionRequest2.m_actors.size() == 0);
	VisionRequest<Block, Actor, Area> visionRequest3(a2);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
	block3.addConstructedFeature(s_floor, s_glass);
	VisionRequest<Block, Actor, Area> visionRequest2(a1);
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
	VisionRequest<Block, Actor, Area> visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
}
TEST_CASE("VisionCuboid setup")
{
	Area area(2,2,2);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	area.visionCuboidsActivate();
	CHECK(area.m_visionCuboids.size() == 1);
}
TEST_CASE("VisionCuboid divide and join")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	area.visionCuboidsActivate();
	Block& block1 = area.m_blocks[1][1][1];
	Block& block2 = area.m_blocks[5][5][2];
	Block& block3 = area.m_blocks[5][5][5];
	Block& block4 = area.m_blocks[1][1][7];
	Block& block5 = area.m_blocks[9][9][1];
	CHECK(area.m_visionCuboids.size() == 1);
	CHECK(block1.m_visionCuboid->m_cuboid.size() == 900);
	CHECK(block1.m_visionCuboid == block2.m_visionCuboid);
	CHECK(block1.m_visionCuboid == block3.m_visionCuboid);
	CHECK(block1.m_visionCuboid == block4.m_visionCuboid);
	CHECK(block1.m_visionCuboid == block5.m_visionCuboid);
	block3.addConstructedFeature(s_floor, s_stone);
	VisionCuboid<Block, Actor, Area>::clearDestroyed(area);
	CHECK(area.m_visionCuboids.size() == 2);
	CHECK(block1.m_visionCuboid->m_cuboid.size() == 400);
	CHECK(block4.m_visionCuboid->m_cuboid.size() == 500);
	CHECK(block1.m_visionCuboid == block2.m_visionCuboid);
	CHECK(block1.m_visionCuboid != block3.m_visionCuboid);
	CHECK(block1.m_visionCuboid != block4.m_visionCuboid);
	CHECK(block1.m_visionCuboid == block5.m_visionCuboid);
	block2.setSolid(s_stone);
	VisionCuboid<Block, Actor, Area>::clearDestroyed(area);
	CHECK(area.m_visionCuboids.size() == 7);
	CHECK(block2.m_visionCuboid == nullptr);
	CHECK(block1.m_visionCuboid != block3.m_visionCuboid);
	CHECK(block1.m_visionCuboid != block4.m_visionCuboid);
	CHECK(block1.m_visionCuboid != block5.m_visionCuboid);
	block3.removeFeature(s_floor);
	VisionCuboid<Block, Actor, Area>::clearDestroyed(area);
	CHECK(area.m_visionCuboids.size() == 7);
	CHECK(block3.m_visionCuboid == block4.m_visionCuboid);
	CHECK(block4.m_visionCuboid->m_cuboid.size() == 500);
	block2.setNotSolid();
	VisionCuboid<Block, Actor, Area>::clearDestroyed(area);
	CHECK(area.m_visionCuboids.size() == 1);
	CHECK(block2.m_visionCuboid != nullptr);
	CHECK(block1.m_visionCuboid == block2.m_visionCuboid);
	CHECK(block1.m_visionCuboid == block3.m_visionCuboid);
	CHECK(block1.m_visionCuboid == block4.m_visionCuboid);
	CHECK(block1.m_visionCuboid == block5.m_visionCuboid);
}
TEST_CASE("VisionCuboid<Block, Actor, Area> can see")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	area.visionCuboidsActivate();
	Block& block1 = area.m_blocks[3][3][1];
	Block& block2 = area.m_blocks[7][7][1];
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	Actor a2(&block2, s_oneByOneFull, s_twoLegs);
	VisionRequest<Block, Actor, Area> visionRequest(a1);
	visionRequest.readStep();
	CHECK(visionRequest.m_actors.size() == 1);
	CHECK(visionRequest.m_actors.contains(&a2));
}
