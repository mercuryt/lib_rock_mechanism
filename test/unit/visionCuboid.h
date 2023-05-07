TEST_CASE("create")
{
	Area area(2,2,2);
	BaseCuboid<Block, Actor, Area> cuboid(area.m_blocks[1][1][1], area.m_blocks[0][0][0]);
	VisionCuboid<Block, Actor, Area> visionCuboid(cuboid);
}
TEST_CASE("can see into")
{
	Area area(2,2,2);
	BaseCuboid<Block, Actor, Area> c1(area.m_blocks[0][1][1], area.m_blocks[0][0][0]);
	VisionCuboid<Block, Actor, Area> vc1(c1);
	BaseCuboid<Block, Actor, Area> c2(area.m_blocks[1][1][1], area.m_blocks[1][0][0]);
	VisionCuboid<Block, Actor, Area> vc2(c2);
	CHECK(vc1.m_cuboid.size() == 4);
	CHECK(vc2.m_cuboid.size() == 4);
	CHECK(vc2.canSeeInto(vc1.m_cuboid));
	CHECK(vc1.canSeeInto(vc2.m_cuboid));
}
TEST_CASE("can combine with")
{
	Area area(2,2,2);
	BaseCuboid<Block, Actor, Area> c1(area.m_blocks[0][1][1], area.m_blocks[0][0][0]);
	VisionCuboid<Block, Actor, Area> vc1(c1);
	BaseCuboid<Block, Actor, Area> c2(area.m_blocks[1][1][1], area.m_blocks[1][0][0]);
	VisionCuboid<Block, Actor, Area> vc2(c2);
	CHECK(vc1.m_cuboid.size() == 4);
	CHECK(vc2.m_cuboid.size() == 4);
	CHECK(vc1.canCombineWith(vc2.m_cuboid));
	BaseCuboid<Block, Actor, Area> c3(area.m_blocks[1][1][0], area.m_blocks[1][0][0]);
	VisionCuboid<Block, Actor, Area> vc3(c3);
	CHECK(!vc3.canCombineWith(vc1.m_cuboid));
	CHECK(!vc1.canCombineWith(vc3.m_cuboid));
}
TEST_CASE("setup area")
{
	Area area(2,2,2);
	VisionCuboid<Block, Actor, Area>::setup(area);
	CHECK(area.m_visionCuboids.size() == 1);
	VisionCuboid<Block, Actor, Area>& visionCuboid = *area.m_blocks[0][0][0].m_visionCuboid;
	CHECK(visionCuboid.m_cuboid.size() == 8);
	for(Block& block : visionCuboid.m_cuboid)
		CHECK(block.m_visionCuboid == &visionCuboid);
}
TEST_CASE("split at")
{
	Area area(2,2,1);
	Block& b1 = area.m_blocks[0][0][0];
	Block& b2 = area.m_blocks[1][0][0];
	Block& b3 = area.m_blocks[0][1][0];
	Block& b4 = area.m_blocks[1][1][0];
	area.visionCuboidsActivate();
	VisionCuboid<Block, Actor, Area>& vc0 = *b1.m_visionCuboid;
	vc0.splitAt(b4);
	CHECK(vc0.m_destroy);
	VisionCuboid<Block, Actor, Area>::clearDestroyed(area);
	CHECK(area.m_visionCuboids.size() == 2);
	VisionCuboid<Block, Actor, Area>& vc1 = *b1.m_visionCuboid;
	VisionCuboid<Block, Actor, Area>& vc2 = *b2.m_visionCuboid;
	CHECK(&vc1 != &vc2);
	CHECK(b3.m_visionCuboid == &vc1);
	CHECK(vc1.m_cuboid.size() == 2);
	CHECK(vc2.m_cuboid.size() == 1);
	CHECK(area.m_blocks[1][1][0].m_visionCuboid == nullptr);
}
TEST_CASE("split below")
{
	Area area(3,3,3);
	Block& middle = area.m_blocks[1][1][1];
	Block& high = area.m_blocks[2][2][2];
	Block& low = area.m_blocks[0][0][0];
	area.visionCuboidsActivate();
	registerTypes();
	middle.addConstructedFeature(s_floor, s_stone);
	VisionCuboid<Block, Actor, Area>::clearDestroyed(area);
	CHECK(area.m_visionCuboids.size() == 2);
	CHECK(middle.m_visionCuboid == high.m_visionCuboid);
	CHECK(middle.m_visionCuboid != low.m_visionCuboid);
	CHECK(low.m_visionCuboid->m_cuboid.size() == 9);
	CHECK(middle.m_visionCuboid->m_cuboid.size() == 18);
}
