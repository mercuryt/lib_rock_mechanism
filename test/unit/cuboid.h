TEST_CASE("create")
{
	BaseArea area(2,2,2);
	Cuboid<DerivedBlock, DerivedActor, DerivedArea> cuboid(&area.m_blocks[1][1][1], &area.m_blocks[0][0][0]);
	CHECK(cuboid.size() == 8);
	cuboid.contains(area.m_blocks[1][1][0]);
}
TEST_CASE("merge")
{
	BaseArea area(2,2,2);
	Cuboid<DerivedBlock, DerivedActor, DerivedArea> c1(&area.m_blocks[0][1][1], &area.m_blocks[0][0][0]);
	CHECK(c1.size() == 4);
	Cuboid<DerivedBlock, DerivedActor, DerivedArea> c2(&area.m_blocks[1][1][1], &area.m_blocks[1][0][0]);
	CHECK(c1.canMerge(c2));
	Cuboid<DerivedBlock, DerivedActor, DerivedArea> sum = c1.sum(c2);
	CHECK(sum.size() == 8);
	CHECK(c1 != c2);
	c2.merge(c1);
	CHECK(c2.size() == 8);
}
TEST_CASE("get face")
{
	BaseArea area(2,2,2);
	Cuboid<DerivedBlock, DerivedActor, DerivedArea> c1(&area.m_blocks[1][1][1], &area.m_blocks[0][0][0]);
	Cuboid<DerivedBlock, DerivedActor, DerivedArea> face = c1.getFace(4);
	CHECK(face.size() == 4);
	CHECK(face.contains(area.m_blocks[1][0][0]));
	CHECK(!face.contains(area.m_blocks[0][0][0]));
}
