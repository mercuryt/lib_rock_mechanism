TEST_CASE("Create Fluid.")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block = area.m_blocks[50][50][1];
	block.m_solid = nullptr;
	block.addFluid(100, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	fluidGroup->readStep();
	CHECK(fluidGroup->m_stable);
	fluidGroup->writeStep();
	CHECK(!area.m_blocks[50][50][2].m_fluids.contains(s_water));
}
TEST_CASE("Excess volume spawns, negitive excess despawns.")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayers(area, 0, 2, s_stone);
	Block& block = area.m_blocks[50][50][1];
	Block& block2 = area.m_blocks[50][50][2];
	block.m_solid = nullptr;
	block2.m_solid = nullptr;
	block.addFluid(200, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	CHECK(!fluidGroup->m_stable);
	fluidGroup->readStep();
	CHECK(!fluidGroup->m_stable);
	fluidGroup->writeStep();
	CHECK(block2.m_fluids.contains(s_water));
	uint32_t block2WaterVolume = block2.m_fluids[s_water].first;
	CHECK(block2WaterVolume == 200 - MAX_BLOCK_VOLUME);
	FluidGroup* fg = block2.m_fluids[s_water].second;
	CHECK(fluidGroup == fg);
	fluidGroup->readStep();
	CHECK(fluidGroup->m_stable);
	fluidGroup->writeStep();
	CHECK(block2.m_fluids.contains(s_water));
	CHECK(!area.m_blocks[50][50][3].m_fluids.contains(s_water));
	block.removeFluid(MAX_BLOCK_VOLUME, s_water);
	CHECK(!fluidGroup->m_stable);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	CHECK(!block2.m_fluids.contains(s_water));
	CHECK(!fluidGroup->m_stable);
	fluidGroup->readStep();
	CHECK(fluidGroup->m_stable);
}
