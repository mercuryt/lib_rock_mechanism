TEST_CASE("FluidContents with groups and blocks")
	Area area(10,10,10);
	Block& b1 = area.m_blocks[1][1][1];
	b1.addFluid(s_maxBlockVolume, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	FluidContents& fc = b1.m_fluidContents;
	CHECK(fc.m_totalVolume == 0);
	CHECK(!fc.isOverfull);
	CHECK(!fc.contains(s_water));
	CHECK(fc.canEnter(s_water));
	CHECK(fc.volumeCanEnter(s_water) == s_maxBlockVolume);
	CHECK(fluidGroup == &fc.getGroup(s_water));
	CHECK(fc.m_totalVolume == s_maxBlockVolume);
	CHECK(fc.getVolume(s_water) == s_maxBlockVolume);
	CHECK(!fc.isOverfull);
	CHECK(fc.contains(s_water));
	CHECK(!fc.canEnter(s_water));
	b1.addFluid(s_maxBlockVolume, s_mercury);
	CHECK(fc.isOverfull);
	CHECK(fc.m_totalVolume == 2 * s_maxBlockVolume);
	CHECK(fc.getVolume(s_mercury) == s_maxBlockVolume);
	CHECK(fc.getGroup(s_mercury).m_fluidType == s_mercury);
	fc.resolveOverfull();
	CHECK(fc.m_totalVolume == s_maxBlockVolume);
	CHECK(!fc.canEnter(s_water));
	CHECK(!fc.canEnter(s_mercury));
	CHECK(fc.canEnter(s_lava));
}
