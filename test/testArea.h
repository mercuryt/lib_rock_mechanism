TEST_CASE("Make Area")
{
	Area area(10,10,10);
	CHECK(area.m_sizeX == 10);
	CHECK(area.m_sizeY == 10);
	CHECK(area.m_sizeZ == 10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	CHECK(s_water->name == "water");
	CHECK(area.m_blocks[5][5][0].getSolidMaterial() == s_stone);
	area.stepCaveInRead();
	area.stepCaveInWrite();
	CHECK(area.m_caveInCheck.empty());
}
