TEST_CASE("Make Area")
{
	Area area(100,100,100);
	CHECK(area.m_sizeX == 100);
	CHECK(area.m_sizeY == 100);
	CHECK(area.m_sizeZ == 100);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	CHECK(s_water->name == "water");
	CHECK(area.m_blocks[50][50][0].getSolidMaterial() == s_stone);
	area.stepCaveInRead();
	area.stepCaveInWrite();
	CHECK(area.m_caveInCheck.empty());
}
