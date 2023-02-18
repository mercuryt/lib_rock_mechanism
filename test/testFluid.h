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
TEST_CASE("Excess volume spawns and negitive excess despawns.")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayers(area, 0, 2, s_stone);
	Block& block = area.m_blocks[50][50][1];
	Block& block2 = area.m_blocks[50][50][2];
	block.m_solid = nullptr;
	block2.m_solid = nullptr;
	block.addFluid(MAX_BLOCK_VOLUME * 2, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	CHECK(!fluidGroup->m_stable);
	fluidGroup->readStep();
	CHECK(!fluidGroup->m_stable);
	fluidGroup->writeStep();
	CHECK(block2.m_fluids.contains(s_water));
	uint32_t block2WaterVolume = block2.m_fluids[s_water].first;
	CHECK(block2WaterVolume == MAX_BLOCK_VOLUME);
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
	CHECK(fluidGroup->m_stable);
	fluidGroup->writeStep();
	CHECK(block.m_fluids.contains(s_water));
	uint32_t blockWaterVolume = block.m_fluids[s_water].first;
	CHECK(blockWaterVolume == MAX_BLOCK_VOLUME);
	CHECK(!block2.m_fluids.contains(s_water));
}
TEST_CASE("Remove volume can destroy FluidGroups.")
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
	block.removeFluid(100, s_water);
	CHECK(fluidGroup->m_destroy == false);
	fluidGroup->readStep();
	CHECK(fluidGroup->m_futureBlocks.size() == 0);
	CHECK(fluidGroup->m_destroy == true);
	area.writeStep();
	CHECK(area.m_unstableFluidGroups.empty());
	CHECK(area.m_fluidGroups.empty());
}
TEST_CASE("Flow into adjacent hole")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayers(area, 0, 2, s_stone);
	Block& block = area.m_blocks[50][50][1];
	Block& block2 = area.m_blocks[50][50][2];
	Block& block3 = area.m_blocks[50][51][2];
	block.m_solid = nullptr;
	block2.m_solid = nullptr;
	block3.m_solid = nullptr;
	block3.addFluid(MAX_BLOCK_VOLUME, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	CHECK(!fluidGroup->m_stable);
	fluidGroup->readStep();
	CHECK(fluidGroup->m_destroy == false);
	fluidGroup->writeStep();
	CHECK(fluidGroup->m_blocks.size() == 2);
	CHECK(!block.m_fluids.contains(s_water));
	CHECK(block2.m_fluids.contains(s_water));
	CHECK(block3.m_fluids.contains(s_water));
	CHECK(block3.m_fluids[s_water].first == MAX_BLOCK_VOLUME / 2);
	CHECK(block2.m_fluids[s_water].first == MAX_BLOCK_VOLUME / 2);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	CHECK(block.m_fluids.contains(s_water));
	CHECK(!block2.m_fluids.contains(s_water));
	CHECK(!block3.m_fluids.contains(s_water));
	CHECK(block.m_fluids[s_water].first == MAX_BLOCK_VOLUME);
	CHECK(fluidGroup->m_blocks.size() == 1);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	CHECK(fluidGroup->m_stable);
}
TEST_CASE("Flow across flat area")
{
	Area area(100,100,100);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block = area.m_blocks[50][50][1];
	block.addFluid(MAX_BLOCK_VOLUME, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	fluidGroup->readStep();
	fluidGroup->writeStep();
	CHECK(block.volumeOfFluidTypeContains(s_water) == MAX_BLOCK_VOLUME / 5);
	for(Block* adjacent : block.m_adjacents)
		if(adjacent->m_z == 1)
			CHECK(adjacent->volumeOfFluidTypeContains(s_water) == MAX_BLOCK_VOLUME / 5);
	CHECK(!fluidGroup->m_stable);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	CHECK(block.volumeOfFluidTypeContains(s_water) == MAX_BLOCK_VOLUME / 13);
	Block& block2 = area.m_blocks[50][52][1];
	CHECK(block2.volumeOfFluidTypeContains(s_water) == MAX_BLOCK_VOLUME / 13);
	Block& block3 = area.m_blocks[51][51][1];
	CHECK(block3.volumeOfFluidTypeContains(s_water) == MAX_BLOCK_VOLUME / 13);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	CHECK(block.volumeOfFluidTypeContains(s_water) == MAX_BLOCK_VOLUME / 25);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == MAX_BLOCK_VOLUME / 25);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == MAX_BLOCK_VOLUME / 25);
	Block& block4 = area.m_blocks[50][53][1];
	CHECK(block4.volumeOfFluidTypeContains(s_water) == MAX_BLOCK_VOLUME / 25);
	CHECK(!fluidGroup->m_stable);
}
