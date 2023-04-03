TEST_CASE("Create Fluid.")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block = area.m_blocks[5][5][1];
	block.setNotSolid();
	block.addFluid(100, s_water);
	CHECK(area.m_blocks[5][5][1].m_fluids.contains(s_water));
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	CHECK(fluidGroup->m_fillQueue.m_set.size() == 1);
	fluidGroup->readStep();
	CHECK(fluidGroup->m_stable);
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->mergeStep();
	fluidGroup->splitStep();
	CHECK(!area.m_blocks[5][5][2].m_fluids.contains(s_water));
	CHECK(area.m_blocks[5][5][1].m_fluids.contains(s_water));
	CHECK(area.m_fluidGroups.size() == 1);
}
TEST_CASE("Excess volume spawns and negitive excess despawns.")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 2, s_stone);
	Block& block = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	Block& block3 = area.m_blocks[5][5][3];
	block.setNotSolid();
	block2.setNotSolid();
	block.addFluid(s_maxBlockVolume * 2, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	CHECK(!fluidGroup->m_stable);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 1);
	CHECK(fluidGroup->m_fillQueue.m_set.size() == 1);
	CHECK(fluidGroup->m_fillQueue.m_queue[0].block == &block2);
	// Step 1.
	fluidGroup->readStep();
	CHECK(!fluidGroup->m_stable);
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 2);
	CHECK(fluidGroup->m_fillQueue.m_set.size() == 1);
	CHECK(fluidGroup->m_fillQueue.m_queue[0].block == &block3);
	CHECK(block2.m_fluids.contains(s_water));
	CHECK(block2.m_fluids[s_water].first == s_maxBlockVolume);
	CHECK(fluidGroup == block2.m_fluids[s_water].second);
	// Step 2.
	fluidGroup->readStep();
	CHECK(fluidGroup->m_stable);
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(block2.m_fluids.contains(s_water));
	CHECK(!area.m_blocks[5][5][3].m_fluids.contains(s_water));
	block.removeFluid(s_maxBlockVolume, s_water);
	CHECK(!fluidGroup->m_stable);
	// Step 3.
	fluidGroup->readStep();
	CHECK(fluidGroup->m_stable);
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(block.m_fluids.contains(s_water));
	CHECK(block.m_fluids[s_water].first == s_maxBlockVolume);
	CHECK(!block2.m_fluids.contains(s_water));
}
TEST_CASE("Remove volume can destroy FluidGroups.")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block = area.m_blocks[5][5][1];
	block.setNotSolid();
	block.addFluid(100, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	fluidGroup->readStep();
	CHECK(fluidGroup->m_stable);
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	block.removeFluid(100, s_water);
	CHECK(fluidGroup->m_destroy == false);
	// Step 1.
	fluidGroup->readStep();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_futureEmpty.size() == 1);
	CHECK(fluidGroup->m_destroy == true);
}
TEST_CASE("Flow into adjacent hole")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 2, s_stone);
	Block& destination = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	Block& origin = area.m_blocks[5][6][2];
	Block& block4 = area.m_blocks[5][5][3];
	Block& block5 = area.m_blocks[5][6][3];
	destination.setNotSolid();
	block2.setNotSolid();
	origin.setNotSolid();
	origin.addFluid(s_maxBlockVolume, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	CHECK(!fluidGroup->m_stable);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 1);
	CHECK(fluidGroup->m_fillQueue.m_set.size() == 2);
	// Step 1.
	fluidGroup->readStep();
	CHECK(fluidGroup->m_destroy == false);
	CHECK(fluidGroup->m_fillQueue.m_set.size() == 2);
	CHECK(fluidGroup->m_fillQueue.m_set.contains(&block2));
	CHECK(fluidGroup->m_fillQueue.m_set.contains(&block5));
	CHECK(fluidGroup->m_futureRemoveFromFillQueue.empty());
	CHECK(fluidGroup->m_futureAddToFillQueue.size() == 3);
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 2);
	CHECK(!destination.m_fluids.contains(s_water));
	CHECK(block2.m_fluids.contains(s_water));
	CHECK(origin.m_fluids.contains(s_water));
	CHECK(origin.m_fluids[s_water].first == s_maxBlockVolume / 2);
	CHECK(block2.m_fluids[s_water].first == s_maxBlockVolume / 2);
	CHECK(fluidGroup->m_fillQueue.m_set.size() == 5);
	CHECK(fluidGroup->m_fillQueue.m_set.contains(&destination));
	CHECK(fluidGroup->m_fillQueue.m_set.contains(&block2));
	CHECK(fluidGroup->m_fillQueue.m_set.contains(&origin));
	CHECK(fluidGroup->m_fillQueue.m_set.contains(&block4));
	CHECK(fluidGroup->m_fillQueue.m_set.contains(&block5));
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 2);
	CHECK(fluidGroup->m_drainQueue.m_set.contains(&block2));
	CHECK(fluidGroup->m_drainQueue.m_set.contains(&origin));
	// Step 2.
	fluidGroup->readStep();
	CHECK(fluidGroup->m_futureRemoveFromFillQueue.size() == 4);
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(destination.m_fluids.contains(s_water));
	CHECK(!block2.m_fluids.contains(s_water));
	CHECK(!origin.m_fluids.contains(s_water));
	CHECK(destination.m_fluids[s_water].first == s_maxBlockVolume);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 1);
	// If the group is stable at this point depends on the viscosity of water, do one more step to make sure.
	CHECK(fluidGroup->m_fillQueue.m_set.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 1);
	// Step 3.
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fluidGroup->m_stable);
}
TEST_CASE("Flow across flat area")
{
	s_step = 0;
	Area area(20,20,20);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block = area.m_blocks[10][10][1];
	Block& block2 = area.m_blocks[10][12][1];
	Block& block3 = area.m_blocks[11][11][1];
	Block& block4 = area.m_blocks[10][13][1];
	Block& block5 = area.m_blocks[10][14][1];
	Block& block6 = area.m_blocks[10][15][1];
	Block& block7 = area.m_blocks[16][10][1];
	Block& block8 = area.m_blocks[17][10][1];
	block.addFluid(s_maxBlockVolume, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	//Step 1.
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 5);
	CHECK(block.volumeOfFluidTypeContains(s_water) == 20);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 0);
	for(Block* adjacent : block.m_adjacents)
		if(adjacent->m_z == 1)
			CHECK(adjacent->volumeOfFluidTypeContains(s_water) == 20);
	CHECK(!fluidGroup->m_stable);
	CHECK(fluidGroup->m_excessVolume == 0);
	s_step++;
	//Step 2.
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 13);
	CHECK(block.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fluidGroup->m_excessVolume == 9);
	s_step++;
	//Step 3.
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 25);
	CHECK(block.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(!fluidGroup->m_stable);
	CHECK(fluidGroup->m_excessVolume == 25);
	s_step++;
	//Step 4.
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 41);
	CHECK(block.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block6.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fluidGroup->m_excessVolume == 18);
	s_step++;
	//Step 5.
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 61);
	CHECK(block.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block6.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block8.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fluidGroup->m_excessVolume == 39);
	s_step++;
	//Step 6.
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 85);
	CHECK(block.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block6.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block7.volumeOfFluidTypeContains(s_water) == 1);
	CHECK(block8.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fluidGroup->m_excessVolume == 15);
	s_step++;
	//Step 7.
	fluidGroup->readStep();
	CHECK(fluidGroup->m_stable);
}
TEST_CASE("Flow across flat area double stack")
{
	Area area(20,20,20);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& origin1 = area.m_blocks[10][10][1];
	Block& origin2 = area.m_blocks[10][10][2];
	Block& block1 = area.m_blocks[10][11][1];
	Block& block2 = area.m_blocks[11][11][1];
	Block& block3 = area.m_blocks[10][12][1];
	Block& block4 = area.m_blocks[10][13][1];
	Block& block5 = area.m_blocks[10][14][1];
	Block& block6 = area.m_blocks[15][10][1];
	Block& block7 = area.m_blocks[16][10][1];
	origin1.addFluid(100, s_water);
	origin2.addFluid(100, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 2);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 5);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 40);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 40);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(!fluidGroup->m_stable);
	CHECK(fluidGroup->m_excessVolume == 0);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 13);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 15);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 15);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 15);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 15);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fluidGroup->m_excessVolume == 5);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 25);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(!fluidGroup->m_stable);
	CHECK(fluidGroup->m_excessVolume == 25);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 41);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 4);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 4);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 4);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 4);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 4);
	CHECK(block6.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fluidGroup->m_excessVolume == 36);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 61);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block6.volumeOfFluidTypeContains(s_water) == 3);
	CHECK(block7.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fluidGroup->m_excessVolume == 17);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 85);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block6.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(block7.volumeOfFluidTypeContains(s_water) == 2);
	CHECK(fluidGroup->m_excessVolume == 30);
	CHECK(!fluidGroup->m_stable);
}
TEST_CASE("Flow across area and then fill hole")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block = area.m_blocks[5][5][2];
	Block& block2a = area.m_blocks[5][6][2];
	Block& block2b = area.m_blocks[6][5][2];
	Block& block2c = area.m_blocks[5][4][2];
	Block& block2d = area.m_blocks[4][5][2];
	Block& block3 = area.m_blocks[6][6][2];
	Block& block4 = area.m_blocks[5][7][2];
	Block& block5 = area.m_blocks[5][7][1];
	Block& block6 = area.m_blocks[5][8][2];
	block.addFluid(s_maxBlockVolume, s_water);
	block5.setNotSolid();
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 5);
	CHECK(block.volumeOfFluidTypeContains(s_water) == 20);
	CHECK(block2a.volumeOfFluidTypeContains(s_water) == 20);
	CHECK(block2b.volumeOfFluidTypeContains(s_water) == 20);
	CHECK(block2c.volumeOfFluidTypeContains(s_water) == 20);
	CHECK(block2d.volumeOfFluidTypeContains(s_water) == 20);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(!fluidGroup->m_stable);
	CHECK(fluidGroup->m_excessVolume == 0);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 13);
	CHECK(block.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block2a.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block2b.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block2c.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block2d.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 7);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block6.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fluidGroup->m_excessVolume == 9);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 1);
	CHECK(block.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block2a.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(block6.volumeOfFluidTypeContains(s_water) == 0);
	// If the group is stable at this point depends on the viscosity of water, do one more step to make sure.
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(fluidGroup->m_stable);
}
TEST_CASE("FluidGroups are able to split into parts")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 2, s_stone);
	Block& destination1 = area.m_blocks[5][4][1];
	Block& destination2 = area.m_blocks[5][6][1];
	Block& origin1 = area.m_blocks[5][5][2];
	Block& origin2 = area.m_blocks[5][5][3];
	destination1.setNotSolid();
	destination2.setNotSolid();
	destination1.m_adjacents[5]->setNotSolid();
	destination2.m_adjacents[5]->setNotSolid();
	origin1.setNotSolid();
	origin1.addFluid(100, s_water);
	origin2.addFluid(100, s_water);
	CHECK(origin1.getFluidGroup(s_water) == origin2.getFluidGroup(s_water));
	CHECK(area.m_fluidGroups.size() == 1);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 2);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 66);
	CHECK(destination1.m_adjacents[5]->volumeOfFluidTypeContains(s_water) == 66);
	CHECK(destination2.m_adjacents[5]->volumeOfFluidTypeContains(s_water) == 66);
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(destination1.m_adjacents[5]->volumeOfFluidTypeContains(s_water) == 0);
	CHECK(destination2.m_adjacents[5]->volumeOfFluidTypeContains(s_water) == 0);
	CHECK(destination1.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(destination2.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(area.m_fluidGroups.size() == 2);
	CHECK(destination1.getFluidGroup(s_water) != destination2.getFluidGroup(s_water));
}
TEST_CASE("Fluid Groups merge")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& origin1 = area.m_blocks[5][4][1];
	Block& block1 = area.m_blocks[5][5][1];
	Block& origin2 = area.m_blocks[5][6][1];
	origin1.setNotSolid();
	block1.setNotSolid();
	origin2.setNotSolid();
	origin1.addFluid(100, s_water);
	origin2.addFluid(100, s_water);
	CHECK(area.m_fluidGroups.size() == 2);
	FluidGroup* fg1 = origin1.getFluidGroup(s_water);
	FluidGroup* fg2 = origin2.getFluidGroup(s_water);
	CHECK(fg1 != fg2);
	// Step 1.
	fg1->readStep();
	fg2->readStep();
	fg1->writeStep();
	fg2->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	fg1->mergeStep();
	fg2->mergeStep();
	CHECK(fg2->m_merged);
	CHECK(fg1->m_drainQueue.m_set.size() == 3);
	CHECK(fg1->m_excessVolume == 0);
	CHECK(fg1->m_fillQueue.m_set.size() == 6);
	CHECK(fg1->m_drainQueue.m_set.size() == 3);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(fg1 == origin1.getFluidGroup(s_water));
	CHECK(fg1 == block1.getFluidGroup(s_water));
	CHECK(fg1 == origin2.getFluidGroup(s_water));
	// Step 2.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	// Step 3.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 66);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 66);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 66);
	CHECK(fg1->m_excessVolume == 2);
	CHECK(fg1->m_stable);
}
TEST_CASE("Fluid Groups merge four blocks")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block1 = area.m_blocks[5][4][1];
	Block& block2 = area.m_blocks[5][5][1];
	Block& block3 = area.m_blocks[5][6][1];
	Block& block4 = area.m_blocks[5][7][1];
	block1.setNotSolid();
	block2.setNotSolid();
	block3.setNotSolid();
	block4.setNotSolid();
	block1.addFluid(100, s_water);
	block4.addFluid(100, s_water);
	CHECK(area.m_fluidGroups.size() == 2);
	FluidGroup* fg1 = block1.getFluidGroup(s_water);
	FluidGroup* fg2 = block4.getFluidGroup(s_water);
	CHECK(fg1 != fg2);
	// Step 1.
	fg1->readStep();
	fg2->readStep();
	fg1->writeStep();
	fg2->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	fg1->mergeStep();
	CHECK(fg2->m_merged);
	fg2->mergeStep();
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 50);
	// Step 2.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->mergeStep();
	fg1->splitStep();
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(fg1->m_stable);
}
TEST_CASE("Denser fluids sink")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 2, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	Block& block3 = area.m_blocks[5][5][3];
	block1.setNotSolid();
	block2.setNotSolid();
	block1.addFluid(100, s_water);
	block2.addFluid(100, s_mercury);
	CHECK(area.m_fluidGroups.size() == 2);
	FluidGroup* fgWater = block1.getFluidGroup(s_water);
	FluidGroup* fgMercury = block2.getFluidGroup(s_mercury);
	CHECK(fgWater != nullptr);
	CHECK(fgMercury != nullptr);
	CHECK(fgWater->m_fluidType == s_water);
	CHECK(fgMercury->m_fluidType == s_mercury);
	CHECK(fgWater->m_fillQueue.m_set.size() == 1);
	// Step 1.
	fgWater->readStep();
	CHECK(fgWater->m_fillQueue.m_set.size() == 1);
	fgMercury->readStep();
	fgWater->writeStep();
	CHECK(fgWater->m_drainQueue.m_set.size() == 1);
	fgMercury->writeStep();
	fgWater->afterWriteStep();
	fgMercury->afterWriteStep();
	fgWater->splitStep();
	fgMercury->splitStep();
	CHECK(fgWater->m_fillQueue.m_set.size() == 2);
	CHECK(fgWater->m_fillQueue.m_set.contains(&block1));
	CHECK(fgWater->m_fillQueue.m_set.contains(&block2));
	fgWater->mergeStep();
	fgMercury->mergeStep();
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block1.volumeOfFluidTypeContains(s_mercury) == 50);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block2.volumeOfFluidTypeContains(s_mercury) == 50);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fgWater->m_excessVolume == 50);
	CHECK(fgMercury->m_excessVolume == 0);
	CHECK(!fgWater->m_stable);
	CHECK(!fgMercury->m_stable);
	CHECK(fgWater->m_fillQueue.m_set.size() == 2);
	CHECK(fgWater->m_fillQueue.m_set.contains(&block1));
	CHECK(fgWater->m_fillQueue.m_set.contains(&block2));
	CHECK(fgMercury->m_fillQueue.m_set.size() == 3);
	// Step 2.
	fgWater->readStep();
	fgMercury->readStep();
	fgWater->writeStep();
	fgMercury->writeStep();
	fgWater->afterWriteStep();
	fgMercury->afterWriteStep();
	fgWater->splitStep();
	fgMercury->splitStep();
	fgWater->mergeStep();
	fgMercury->mergeStep();
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block1.volumeOfFluidTypeContains(s_mercury) == 100);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block2.volumeOfFluidTypeContains(s_mercury) == 0);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fgWater->m_excessVolume == 50);
	CHECK(fgMercury->m_excessVolume == 0);
	CHECK(!fgWater->m_stable);
	CHECK(!fgMercury->m_stable);
	CHECK(fgWater->m_fillQueue.m_set.size() == 3);
	CHECK(fgWater->m_fillQueue.m_set.contains(&block1));
	CHECK(fgWater->m_fillQueue.m_set.contains(&block2));
	CHECK(fgWater->m_fillQueue.m_set.contains(&block3));
	CHECK(fgWater->m_drainQueue.m_set.size() == 1);
	CHECK(fgWater->m_drainQueue.m_set.contains(&block2));
	// Step 3.
	fgWater->readStep();
	CHECK(fgWater->m_futureGroups.size() == 0);
	fgMercury->readStep();
	fgWater->writeStep();
	fgMercury->writeStep();
	fgWater->afterWriteStep();
	fgMercury->afterWriteStep();
	fgWater->splitStep();
	fgMercury->splitStep();
	fgWater->mergeStep();
	fgMercury->mergeStep();
	CHECK(fgWater->m_drainQueue.m_set.size() == 1);
	CHECK(fgWater->m_fillQueue.m_set.size() == 2);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block1.volumeOfFluidTypeContains(s_mercury) == 100);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(block2.volumeOfFluidTypeContains(s_mercury) == 0);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(fgWater->m_stable);
	CHECK(fgWater->m_excessVolume == 0);
	CHECK(fgMercury->m_excessVolume == 0);
	CHECK(fgMercury->m_stable);
}
TEST_CASE("Merge 3 groups at two block distance")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block1 = area.m_blocks[5][2][1];
	Block& block2 = area.m_blocks[5][3][1];
	Block& block3 = area.m_blocks[5][4][1];
	Block& block4 = area.m_blocks[5][5][1];
	Block& block5 = area.m_blocks[5][6][1];
	Block& block6 = area.m_blocks[5][7][1];
	Block& block7 = area.m_blocks[5][8][1];
	block1.setNotSolid();
	block2.setNotSolid();
	block3.setNotSolid();
	block4.setNotSolid();
	block5.setNotSolid();
	block6.setNotSolid();
	block7.setNotSolid();
	block1.addFluid(100, s_water);
	block4.addFluid(100, s_water);
	block7.addFluid(100, s_water);
	CHECK(area.m_fluidGroups.size() == 3);
	FluidGroup* fg1 = block1.getFluidGroup(s_water);
	FluidGroup* fg2 = block4.getFluidGroup(s_water);
	FluidGroup* fg3 = block7.getFluidGroup(s_water);
	CHECK(fg1 != nullptr);
	CHECK(fg2 != nullptr);
	CHECK(fg3 != nullptr);
	CHECK(fg1->m_fluidType == s_water);
	CHECK(fg2->m_fluidType == s_water);
	CHECK(fg3->m_fluidType == s_water);
	// Step 1.
	fg1->readStep();
	fg2->readStep();
	fg3->readStep();
	fg1->writeStep();
	fg2->writeStep();
	fg3->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg3->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	fg3->splitStep();
	fg1->mergeStep();
	fg2->mergeStep();
	fg3->mergeStep();
	CHECK(fg2->m_drainQueue.m_set.size() == 7);
	CHECK(fg1->m_merged);
	CHECK(fg3->m_merged);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 33);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 33);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 33);
	CHECK(block6.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block7.volumeOfFluidTypeContains(s_water) == 50);
	// Step 2.
	fg2->readStep();
	fg2->writeStep();
	fg2->splitStep();
	fg2->mergeStep();
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 42);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 42);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 42);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 42);
	CHECK(block5.volumeOfFluidTypeContains(s_water) == 42);
	CHECK(block6.volumeOfFluidTypeContains(s_water) == 42);
	CHECK(block7.volumeOfFluidTypeContains(s_water) == 42);
	fg2->readStep();
	CHECK(fg2->m_stable);
}
TEST_CASE("Split test 2")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 4, s_stone);
	Block& block1 = area.m_blocks[5][4][1];
	Block& block2 = area.m_blocks[5][5][1];
	Block& block3 = area.m_blocks[5][5][2];
	Block& origin1 = area.m_blocks[5][5][3];
	Block& origin2 = area.m_blocks[5][6][3];
	Block& origin3 = area.m_blocks[5][7][3];
	Block& block4 = area.m_blocks[5][7][2];
	origin1.setNotSolid();
	origin2.setNotSolid();
	origin3.setNotSolid();
	block1.setNotSolid();
	block2.setNotSolid();
	block3.setNotSolid();
	block4.setNotSolid();
	origin1.addFluid(20, s_water);
	FluidGroup* fg1 = origin1.getFluidGroup(s_water);
	origin2.addFluid(20, s_water);
	origin3.addFluid(20, s_water);
	CHECK(area.m_fluidGroups.size() == 1);
	CHECK(fg1->m_drainQueue.m_set.size() == 3);
	// Step 1.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	CHECK(fg1->m_excessVolume == 0);
	CHECK(fg1->m_drainQueue.m_set.size() == 1);
	CHECK(fg1->m_drainQueue.m_set.contains(&block4));
	fg1 = block3.getFluidGroup(s_water);
	FluidGroup* fg2 = block4.getFluidGroup(s_water);
	CHECK(fg2->m_drainQueue.m_set.size() == 1);
	CHECK(fg1 != fg2);
	CHECK(fg1->m_fillQueue.m_set.size() == 3);
	CHECK(fg2->m_fillQueue.m_set.size() == 2);
	CHECK(area.m_fluidGroups.size() == 2);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin3.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 30);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 30);
	// Step 2.
	fg1->readStep();
	fg2->readStep();
	fg1->writeStep();
	fg2->writeStep();
	fg2->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	fg1->mergeStep();
	fg2->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin3.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 30);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 30);
	CHECK(fg2->m_stable);
	// Step 3.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 15);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 15);
	// Step 4.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	CHECK(fg1->m_stable);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 15);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 15);
}
TEST_CASE("Merge with group as it splits")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 4, s_stone);
	Block& origin1 = area.m_blocks[5][4][1];
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	Block& origin2 = area.m_blocks[5][5][3];
	Block& origin3 = area.m_blocks[5][6][3];
	Block& origin4 = area.m_blocks[5][7][3];
	Block& block3 = area.m_blocks[5][7][2];
	origin1.setNotSolid();
	origin2.setNotSolid();
	origin3.setNotSolid();
	origin4.setNotSolid();
	block1.setNotSolid();
	block2.setNotSolid();
	block3.setNotSolid();
	origin1.addFluid(100, s_water);
	FluidGroup* fg1 = origin1.getFluidGroup(s_water);
	origin2.addFluid(20, s_water);
	FluidGroup* fg2 = origin2.getFluidGroup(s_water);
	origin3.addFluid(20, s_water);
	origin4.addFluid(20, s_water);
	CHECK(area.m_fluidGroups.size() == 2);
	CHECK(fg1->m_drainQueue.m_set.size() == 1);
	CHECK(fg2->m_drainQueue.m_set.size() == 3);
	CHECK(fg1 != fg2);
	// Step 1.
	fg1->readStep();
	fg2->readStep();
	fg1->writeStep();
	CHECK(fg1->m_excessVolume == 0);
	fg2->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	fg1->mergeStep();
	fg2->mergeStep();
	CHECK(fg1->m_drainQueue.m_set.size() == 3);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin3.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin4.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 30);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 30);
	// Step 2.
	fg1->readStep();
	fg2->readStep();
	CHECK(fg1->m_excessVolume == 0);
	fg1->writeStep();
	CHECK(fg1->m_drainQueue.m_set.size() == 2);
	fg2->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	fg1->mergeStep();
	CHECK(fg1->m_excessVolume == 0);
	fg2->mergeStep();
	CHECK(!fg2->m_merged);
	CHECK(fg2->m_stable);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 65);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 65);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 30);
	CHECK(fg1->m_stable);
}
TEST_CASE("Merge with two groups while spliting")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 4, s_stone);
	Block& origin1 = area.m_blocks[5][4][1];
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	Block& origin2 = area.m_blocks[5][5][3];
	Block& origin3 = area.m_blocks[5][6][3];
	Block& origin4 = area.m_blocks[5][7][3];
	Block& block3 = area.m_blocks[5][7][2];
	Block& block4 = area.m_blocks[5][7][1];
	Block& origin5 = area.m_blocks[5][8][1];
	origin1.setNotSolid();
	origin2.setNotSolid();
	origin3.setNotSolid();
	origin4.setNotSolid();
	origin5.setNotSolid();
	block1.setNotSolid();
	block2.setNotSolid();
	block3.setNotSolid();
	block4.setNotSolid();
	origin1.addFluid(100, s_water);
	FluidGroup* fg1 = origin1.getFluidGroup(s_water);
	origin2.addFluid(20, s_water);
	FluidGroup* fg2 = origin2.getFluidGroup(s_water);
	origin3.addFluid(20, s_water);
	origin4.addFluid(20, s_water);
	origin5.addFluid(100, s_water);
	FluidGroup* fg3 = origin5.getFluidGroup(s_water);
	CHECK(area.m_fluidGroups.size() == 3);
	CHECK(fg1 != fg2);
	CHECK(fg1 != fg3);
	CHECK(fg2 != fg3);
	// Step 1.
	fg1->readStep();
	fg2->readStep();
	fg3->readStep();
	CHECK(fg3->m_futureGroups.empty());
	fg1->writeStep();
	CHECK(fg1->m_excessVolume == 0);
	fg2->writeStep();
	fg3->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg3->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	FluidGroup* fg4 = block2.getFluidGroup(s_water);
	CHECK(!fg3->m_merged);
	fg3->splitStep();
	CHECK(fg1->m_drainQueue.m_set.size() == 2);
	CHECK(fg2->m_drainQueue.m_set.size() == 1);
	fg1->mergeStep();
	CHECK(fg1->m_drainQueue.m_set.size() == 3);
	fg2->mergeStep();
	fg4->mergeStep();
	fg3->mergeStep();
	CHECK(fg4->m_merged);
	CHECK(fg2->m_merged);
	CHECK(fg1->m_drainQueue.m_set.size() == 3);
	CHECK(fg4->m_drainQueue.m_set.size() == 1);
	CHECK(fg1 != fg4);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin3.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin4.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 30);
	CHECK(block3.volumeOfFluidTypeContains(s_water) == 30);
	CHECK(origin5.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 50);
	// Step 2.
	fg1->readStep();
	fg3->readStep();
	CHECK(fg1->m_excessVolume == 0);
	fg1->writeStep();
	CHECK(fg1->m_drainQueue.m_set.size() == 2);
	fg3->writeStep();
	fg1->afterWriteStep();
	fg3->afterWriteStep();
	fg1->splitStep();
	fg3->splitStep();
	fg1->mergeStep();
	CHECK(fg1->m_excessVolume == 0);
	fg3->mergeStep();
	CHECK(fg3->m_stable);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 65);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 65);
	CHECK(origin5.volumeOfFluidTypeContains(s_water) == 65);
	CHECK(block4.volumeOfFluidTypeContains(s_water) == 65);
	CHECK(fg1->m_stable);
}
TEST_CASE("Bubbles")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 4, s_stone);
	Block& origin1 = area.m_blocks[5][5][1];
	Block& origin2 = area.m_blocks[5][5][2];
	Block& origin3 = area.m_blocks[5][5][3];
	Block& block1 = area.m_blocks[5][5][4];
	origin1.setNotSolid();
	origin2.setNotSolid();
	origin3.setNotSolid();
	block1.setNotSolid();
	origin1.addFluid(100, s_CO2);
	origin2.addFluid(100, s_water);
	origin3.addFluid(100, s_water);
	FluidGroup* fg1 = origin1.getFluidGroup(s_CO2);
	FluidGroup* fg2 = origin2.getFluidGroup(s_water);
	// Step 1.
	fg1->readStep();
	fg2->readStep();
	fg1->writeStep();
	fg2->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	CHECK(fg1->m_excessVolume == 100);
	CHECK(fg1->m_disolved);
	fg2->splitStep();
	fg2->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_CO2) == 0);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(origin2.volumeOfFluidTypeContains(s_CO2) == 0);
	CHECK(origin3.volumeOfFluidTypeContains(s_CO2) == 100);
	CHECK(fg1 == origin3.getFluidGroup(s_CO2));
	// Step 2.
	fg1->readStep();
	fg2->readStep();
	fg1->writeStep();
	fg2->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	fg1->mergeStep();
	fg2->mergeStep();
	CHECK(fg1->m_stable);
	fg2->removeFluid(100);
	// Step 3.
	fg2->readStep();
	fg2->writeStep();
	CHECK(!fg1->m_stable);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 0);
	fg2->splitStep();
	fg2->mergeStep();
	// Step 4.
	fg1->readStep();
	CHECK(fg2->m_stable);
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(origin2.volumeOfFluidTypeContains(s_CO2) == 100);
	// Step 5.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	CHECK(fg1->m_stable);
	CHECK(fg2->m_stable);
}
TEST_CASE("Three liquids")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 4, s_stone);
	Block& origin1 = area.m_blocks[5][5][1];
	Block& origin2 = area.m_blocks[5][5][2];
	Block& origin3 = area.m_blocks[5][5][3];
	Block& block1 = area.m_blocks[5][5][4];
	origin1.setNotSolid();
	origin2.setNotSolid();
	origin3.setNotSolid();
	block1.setNotSolid();
	origin1.addFluid(100, s_CO2);
	origin2.addFluid(100, s_water);
	origin3.addFluid(100, s_mercury);
	FluidGroup* fg1 = origin1.getFluidGroup(s_CO2);
	FluidGroup* fg2 = origin2.getFluidGroup(s_water);
	FluidGroup* fg3 = origin3.getFluidGroup(s_mercury);
	// Step 1.
	fg1->readStep();
	fg2->readStep();
	fg3->readStep();
	fg1->writeStep();
	fg2->writeStep();
	fg3->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg3->afterWriteStep();
	CHECK(fg1->m_excessVolume == 100);
	CHECK(fg1->m_disolved);
	fg2->splitStep();
	CHECK(fg1->m_excessVolume == 50);
	fg3->splitStep();
	fg2->mergeStep();
	fg3->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_CO2) == 0);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin2.volumeOfFluidTypeContains(s_CO2) == 50);
	CHECK(origin2.volumeOfFluidTypeContains(s_mercury) == 50);
	CHECK(origin3.volumeOfFluidTypeContains(s_mercury) == 50);
	CHECK(fg1 == origin2.getFluidGroup(s_CO2));
	CHECK(fg1->m_excessVolume == 50);
	// Step 2.
	fg1->readStep();
	fg2->readStep();
	fg3->readStep();
	CHECK(!fg3->m_stable);
	fg1->writeStep();
	fg2->writeStep();
	fg3->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg3->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	fg3->splitStep();
	fg1->mergeStep();
	fg2->mergeStep();
	fg3->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(origin1.volumeOfFluidTypeContains(s_mercury) == 50);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 0);
	CHECK(origin2.volumeOfFluidTypeContains(s_mercury) == 50);
	CHECK(origin2.volumeOfFluidTypeContains(s_CO2) == 50);
	CHECK(origin3.volumeOfFluidTypeContains(s_CO2) == 50);
	CHECK(origin3.volumeOfFluidTypeContains(s_mercury) == 0);
	CHECK(block1.m_totalFluidVolume == 0);
	CHECK(fg2->m_excessVolume == 50);
	CHECK(fg1->m_excessVolume == 0);
	// Step 3.
	fg1->readStep();
	fg2->readStep();
	fg3->readStep();
	fg1->writeStep();
	fg2->writeStep();
	fg3->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg3->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	fg3->splitStep();
	fg1->mergeStep();
	fg2->mergeStep();
	fg3->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_mercury) == 100);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(origin2.volumeOfFluidTypeContains(s_mercury) == 0);
	CHECK(origin2.volumeOfFluidTypeContains(s_CO2) == 50);
	CHECK(origin3.volumeOfFluidTypeContains(s_CO2) == 50);
	CHECK(block1.m_totalFluidVolume == 0);
	CHECK(fg1->m_excessVolume == 0);
	CHECK(fg2->m_excessVolume == 50);
	// Step 4.
	fg1->readStep();
	fg2->readStep();
	fg3->readStep();
	fg1->writeStep();
	fg2->writeStep();
	fg3->writeStep();
	fg1->afterWriteStep();
	fg2->afterWriteStep();
	fg3->afterWriteStep();
	fg1->splitStep();
	fg2->splitStep();
	fg3->splitStep();
	fg1->mergeStep();
	fg2->mergeStep();
	fg3->mergeStep();
	CHECK(fg2->m_stable);
	CHECK(origin1.volumeOfFluidTypeContains(s_mercury) == 100);
	CHECK(origin2.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(origin3.volumeOfFluidTypeContains(s_CO2) == 50);
	CHECK(block1.m_totalFluidVolume == 0);
	CHECK(fg1->m_excessVolume == 50);
	CHECK(fg2->m_excessVolume == 0);
	CHECK(fg2->m_stable);
	CHECK(fg3->m_stable);
	// Step 5.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	CHECK(fg1->m_stable);
	CHECK(fg3->m_stable);
}
TEST_CASE("Set not solid")
{
	Area area(10, 10, 10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& origin1 = area.m_blocks[5][5][1];
	Block& block1 = area.m_blocks[5][6][1];
	Block& block2 = area.m_blocks[5][7][1];
	origin1.setNotSolid();
	block2.setNotSolid();
	origin1.addFluid(100, s_water);
	FluidGroup* fg1 = origin1.getFluidGroup(s_water);
	CHECK(fg1 != nullptr);
	// Step 1.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 100);
	CHECK(fg1->m_stable);
	// Step 2.
	block1.setNotSolid();
	CHECK(!fg1->m_stable);
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 50);
	// Step .
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 33);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 33);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 33);
	CHECK(!fg1->m_stable);
	CHECK(fg1->m_excessVolume == 1);
}
TEST_CASE("Set solid")
{
	Area area(10, 10, 10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& origin1 = area.m_blocks[5][5][1];
	Block& block1 = area.m_blocks[5][6][1];
	Block& block2 = area.m_blocks[5][7][1];
	origin1.setNotSolid();
	block1.setNotSolid();
	block2.setNotSolid();
	origin1.addFluid(100, s_water);
	FluidGroup* fg1 = origin1.getFluidGroup(s_water);
	// Step 1.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(fg1->m_excessVolume == 0);
	block1.setSolid(s_stone);
}
TEST_CASE("Set solid and split")
{
	Area area(10, 10, 10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& origin1 = area.m_blocks[5][6][1];
	Block& block2 = area.m_blocks[5][7][1];
	block1.setNotSolid();
	origin1.setNotSolid();
	block2.setNotSolid();
	origin1.addFluid(100, s_water);
	FluidGroup* fg1 = origin1.getFluidGroup(s_water);
	// Step 1.
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 33);
	CHECK(origin1.volumeOfFluidTypeContains(s_water) == 33);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 33);
	CHECK(fg1->m_excessVolume == 1);
	// Step 2.
	origin1.setSolid(s_stone);
	CHECK(origin1.isSolid());
	CHECK(fg1->m_potentiallySplitFromSyncronusStep.size() == 2);
	CHECK(fg1->m_potentiallySplitFromSyncronusStep.contains(&block1));
	CHECK(fg1->m_potentiallySplitFromSyncronusStep.contains(&block2));
	CHECK(fg1->m_excessVolume == 34);
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 50);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 50);
	fg1->splitStep();
	CHECK(area.m_fluidGroups.size() == 2);
	FluidGroup* fg2 = &area.m_fluidGroups.back();
	fg1->mergeStep();
	fg2->mergeStep();
	//Step 3.
	fg1->readStep();
	fg2->readStep();
	CHECK(fg2->m_stable);
	CHECK(fg1->m_stable);
}
TEST_CASE("Cave in falls in fluid and pistons it up")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	block1.setNotSolid();
	block1.addFluid(100, s_water);
	block2.setSolid(s_stone);
area.m_caveInCheck.insert(&block2);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
area.stepCaveInRead();
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
area.stepCaveInWrite();
	CHECK(area.m_unstableFluidGroups.size() == 1);
	CHECK(block1.m_totalFluidVolume == 0);
	CHECK(block1.getSolidMaterial() == s_stone);
	CHECK(!block2.isSolid());
	CHECK(fluidGroup->m_excessVolume == 100);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 0);
	CHECK(fluidGroup->m_fillQueue.m_set.size() == 1);
	CHECK(fluidGroup->m_fillQueue.m_set.contains(&block2));
	CHECK(block2.fluidCanEnterEver());
	fluidGroup->readStep();
area.stepCaveInRead();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
area.stepCaveInWrite();
	CHECK(block2.m_totalFluidVolume == 100);
	CHECK(fluidGroup->m_excessVolume == 0);
	CHECK(fluidGroup->m_stable == false);
	CHECK(area.m_fluidGroups.size() == 1);
}
TEST_CASE("Test diagonal seep")
{
	if constexpr(s_fluidsSeepDiagonalModifier == 0)
		return;
	s_step = 1;
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[6][6][1];
	block1.setNotSolid();
	block2.setNotSolid();
	block1.addFluid(10, s_water);
	FluidGroup* fg1 = *area.m_unstableFluidGroups.begin();
	fg1->readStep();
	fg1->writeStep();
	fg1->afterWriteStep();
	fg1->splitStep();
	fg1->mergeStep();
	s_step++;
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 1);
	FluidGroup* fg2 = block2.getFluidGroup(s_water);
	CHECK(fg1 != fg2);
	CHECK(fg1->m_excessVolume == -1);
	CHECK(!fg1->m_stable);
	for(int i = 0; i < 5; ++i)
	{
		fg1->readStep();
		fg2->readStep();
		fg1->writeStep();
		fg2->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg1->mergeStep();
		fg2->mergeStep();
		s_step++;
	}
	CHECK(block1.volumeOfFluidTypeContains(s_water) == 5);
	CHECK(block2.volumeOfFluidTypeContains(s_water) == 5);
	CHECK(fg1->m_excessVolume == 0);
	CHECK(!fg1->m_stable);
}
TEST_CASE("Test mist")
{
	s_step = 1;
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 2, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	Block& block3 = area.m_blocks[5][5][3];
	Block& block4 = area.m_blocks[5][5][4];
	Block& block5 = area.m_blocks[5][6][3];
	block1.setNotSolid();
	block2.setNotSolid();
	block3.addFluid(100, s_water);
	block4.addFluid(100, s_water);
	FluidGroup* fluidGroup = *area.m_unstableFluidGroups.begin();
	// Step 1.
	fluidGroup->readStep();
	fluidGroup->writeStep();
	fluidGroup->afterWriteStep();
	fluidGroup->splitStep();
	fluidGroup->mergeStep();
	CHECK(block5.m_mist == s_water);
	// Several steps.
	while(s_step < 11)
	{
		if(!fluidGroup->m_stable)
		{
			fluidGroup->readStep();
			fluidGroup->writeStep();
			fluidGroup->afterWriteStep();
			fluidGroup->splitStep();
			fluidGroup->mergeStep();
		}
		s_step++;
	}
	CHECK(area.m_scheduledEvents.at(11).size() == 8);
	area.executeScheduledEvents(11);
	CHECK(area.m_scheduledEvents.empty());
	CHECK(block5.m_mist == nullptr);
}
void trenchTest2Fluids(uint32_t scaleL, uint32_t scaleW, uint32_t steps)
{
	uint32_t maxX = scaleL + 2;
	uint32_t maxY = scaleW + 2;
	uint32_t maxZ = scaleW + 1;
	uint32_t halfMaxX = maxX / 2;
	Area area(maxX, maxY, maxZ);
	s_step = 0;
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, maxZ - 1, s_stone);
	// Water
	Block* water1 = &area.m_blocks[1][1][1];
	Block* water2 = &area.m_blocks[halfMaxX - 1][maxY - 2][maxZ - 1];		
	setFullFluidCuboid(area, water1, water2, s_water);
	// CO2
	Block* CO2_1 = &area.m_blocks[halfMaxX][1][1];
	Block* CO2_2 = &area.m_blocks[maxX - 2][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, CO2_1, CO2_2, s_CO2);
	CHECK(area.m_fluidGroups.size() == 2);
	FluidGroup* fgWater = water1->getFluidGroup(s_water);
	FluidGroup* fgCO2 = CO2_1->getFluidGroup(s_CO2);
	CHECK(!fgWater->m_merged);
	CHECK(!fgCO2->m_merged);
	uint32_t totalVolume = fgWater->totalVolume();
	s_step = 1;
	while(s_step < steps)
	{
		for(FluidGroup* fluidGroup : area.m_unstableFluidGroups)
			fluidGroup->readStep();
		area.writeStep();
		s_step++;
	}
	uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
	uint32_t expectedHeight = 1 + (maxZ - 1) / 2;
	uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
	CHECK(fgWater->m_stable);
	CHECK(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgWater->totalVolume() == totalVolume);
	CHECK(fgCO2->m_stable);
	CHECK(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgCO2->totalVolume() == totalVolume);
	CHECK(area.m_blocks[1][1][1].m_fluids.contains(s_water));
	CHECK(area.m_blocks[maxX - 2][1][1].m_fluids.contains(s_water));
	CHECK(area.m_blocks[1][1][maxZ - 1].m_fluids.contains(s_CO2));
	CHECK(area.m_blocks[maxX - 2][1][maxZ - 1].m_fluids.contains(s_CO2));
}
TEST_CASE("trench test 2 fluids scale 2-1")
{
	trenchTest2Fluids(2, 1, 8);
}
TEST_CASE("trench test 2 fluids scale 4-1")
{
	trenchTest2Fluids(4, 1, 12);
}
TEST_CASE("trench test 2 fluids scale 40-1")
{
	trenchTest2Fluids(40, 1, 30);
}
TEST_CASE("trench test 2 fluids scale 20-5")
{
	trenchTest2Fluids(20, 5, 20);
}
void trenchTest3Fluids(uint32_t scaleL, uint32_t scaleW, uint32_t steps)
{
	uint32_t maxX = scaleL + 2;
	uint32_t maxY = scaleW + 2;
	uint32_t maxZ = scaleW + 1;
	uint32_t thirdMaxX = maxX / 3;
	Area area(maxX, maxY, maxZ);
	s_step = 0;
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, maxZ - 1, s_stone);
	// Water
	Block* water1 = &area.m_blocks[1][1][1];
	Block* water2 = &area.m_blocks[thirdMaxX][maxY - 2][maxZ - 1];		
	setFullFluidCuboid(area, water1, water2, s_water);
	// CO2
	Block* CO2_1 = &area.m_blocks[thirdMaxX + 1][1][1];
	Block* CO2_2 = &area.m_blocks[(thirdMaxX * 2)][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, CO2_1, CO2_2, s_CO2);
	// Lava
	Block* lava1 = &area.m_blocks[(thirdMaxX * 2) + 1][1][1];
	Block* lava2 = &area.m_blocks[maxX - 2][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, lava1, lava2, s_lava);
	CHECK(area.m_fluidGroups.size() == 3);
	FluidGroup* fgWater = water1->getFluidGroup(s_water);
	FluidGroup* fgCO2 = CO2_1->getFluidGroup(s_CO2);
	FluidGroup* fgLava = lava1->getFluidGroup(s_lava);
	s_step = 1;
	uint32_t totalVolume = fgWater->totalVolume();
	while(s_step < steps)
	{
		for(FluidGroup* fluidGroup : area.m_unstableFluidGroups)
			fluidGroup->readStep();
		area.writeStep();
		s_step++;
	}
	uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
	uint32_t expectedHeight = std::max(1u, maxZ / 3);
	uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
	CHECK(fgWater->m_stable);
	CHECK(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgWater->totalVolume() == totalVolume);
	CHECK(fgCO2->m_stable);
	CHECK(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgCO2->totalVolume() == totalVolume);
	CHECK(fgLava->m_stable);
	CHECK(fgLava->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgLava->totalVolume() == totalVolume);
	CHECK(area.m_blocks[1][1][1].m_fluids.contains(s_lava));
	CHECK(area.m_blocks[maxX - 2][1][1].m_fluids.contains(s_lava));
	CHECK(area.m_blocks[1][1][maxZ - 1].m_fluids.contains(s_CO2));
	CHECK(area.m_blocks[maxX - 2][1][maxZ - 1].m_fluids.contains(s_CO2));
}
TEST_CASE("trench test 3 fluids scale 3-1")
{
	trenchTest3Fluids(3, 1, 10);
}
TEST_CASE("trench test 3 fluids scale 9-1")
{
	trenchTest3Fluids(9, 1, 20);
}
TEST_CASE("trench test 3 fluids scale 3-3")
{
	trenchTest3Fluids(3, 3, 10);
}
TEST_CASE("trench test 3 fluids scale 18-3")
{
	trenchTest3Fluids(18, 3, 30);
}
void trenchTest4Fluids(uint32_t scaleL, uint32_t scaleW, uint32_t steps)
{
	uint32_t maxX = scaleL + 2;
	uint32_t maxY = scaleW + 2;
	uint32_t maxZ = scaleW + 1;
	uint32_t quarterMaxX = maxX / 4;
	Area area(maxX, maxY, maxZ);
	s_step = 0;
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, maxZ - 1, s_stone);
	// Water
	Block* water1 = &area.m_blocks[1][1][1];
	Block* water2 = &area.m_blocks[quarterMaxX][maxY - 2][maxZ - 1];		
	setFullFluidCuboid(area, water1, water2, s_water);
	// CO2
	Block* CO2_1 = &area.m_blocks[quarterMaxX + 1][1][1];
	Block* CO2_2 = &area.m_blocks[(quarterMaxX * 2)][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, CO2_1, CO2_2, s_CO2);
	// Lava
	Block* lava1 = &area.m_blocks[(quarterMaxX * 2) + 1][1][1];
	Block* lava2 = &area.m_blocks[quarterMaxX * 3][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, lava1, lava2, s_lava);
	// Mercury
	Block* mercury1 = &area.m_blocks[(quarterMaxX * 3) + 1][1][1];
	Block* mercury2 = &area.m_blocks[maxX - 2][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, mercury1, mercury2, s_mercury);
	CHECK(area.m_fluidGroups.size() == 4);
	FluidGroup* fgWater = water1->getFluidGroup(s_water);
	FluidGroup* fgCO2 = CO2_1->getFluidGroup(s_CO2);
	FluidGroup* fgLava = lava1->getFluidGroup(s_lava);
	FluidGroup* fgMercury = mercury1->getFluidGroup(s_mercury);
	uint32_t totalVolume = fgWater->totalVolume();
	s_step = 1;
	while(s_step < steps)
	{
		for(FluidGroup* fluidGroup : area.m_unstableFluidGroups)
			fluidGroup->readStep();
		area.writeStep();
		fgMercury = getFluidGroup(area, s_mercury);
		if(fgMercury != nullptr)
			CHECK(fgMercury->totalVolume() == totalVolume);
		s_step++;
	}
	uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
	uint32_t expectedHeight = std::max(1u, maxZ / 4);
	uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
	CHECK(fgWater->m_stable);
	CHECK(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgWater->totalVolume() == totalVolume);
	fgCO2 = getFluidGroup(area, s_CO2);
	CHECK(fgCO2 != nullptr);
	CHECK(fgCO2->m_stable);
	CHECK(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgCO2->totalVolume() == totalVolume);
	CHECK(fgLava->m_stable);
	CHECK(fgLava->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgLava->totalVolume() == totalVolume);
	CHECK(fgMercury != nullptr);
	CHECK(fgMercury->m_stable);
	CHECK(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgMercury->totalVolume() == totalVolume);
	CHECK(area.m_blocks[1][1][1].m_fluids.contains(s_lava));
	CHECK(area.m_blocks[maxX - 2][1][1].m_fluids.contains(s_lava));
	CHECK(area.m_blocks[1][1][maxZ - 1].m_fluids.contains(s_CO2));
	CHECK(area.m_blocks[maxX - 2][1][maxZ - 1].m_fluids.contains(s_CO2));
}
TEST_CASE("trench test 4 fluids scale 4-1")
{
	trenchTest4Fluids(4, 1, 10);
}
TEST_CASE("trench test 4 fluids scale 4-2")
{
	trenchTest4Fluids(4, 2, 10);
}
TEST_CASE("trench test 4 fluids scale 4-4")
{
	trenchTest4Fluids(4, 4, 15);
}
TEST_CASE("trench test 4 fluids scale 4-8")
{
	trenchTest4Fluids(4, 8, 17);
}
TEST_CASE("trench test 4 fluids scale 8-8")
{
	trenchTest4Fluids(8, 8, 20);
}
TEST_CASE("trench test 4 fluids scale 16-4")
{
	trenchTest4Fluids(16, 4, 25);
}
void trenchTest2FluidsMerge(uint32_t scaleL, uint32_t scaleW, uint32_t steps)
{
	uint32_t maxX = scaleL + 2;
	uint32_t maxY = scaleW + 2;
	uint32_t maxZ = scaleW + 1;
	uint32_t quarterMaxX = maxX / 4;
	Area area(maxX, maxY, maxZ);
	s_step = 0;
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, maxZ - 1, s_stone);
	// Water
	Block* water1 = &area.m_blocks[1][1][1];
	Block* water2 = &area.m_blocks[quarterMaxX][maxY - 2][maxZ - 1];		
	setFullFluidCuboid(area, water1, water2, s_water);
	// CO2
	Block* CO2_1 = &area.m_blocks[quarterMaxX + 1][1][1];
	Block* CO2_2 = &area.m_blocks[(quarterMaxX * 2)][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, CO2_1, CO2_2, s_CO2);
	// Water
	Block* water3 = &area.m_blocks[(quarterMaxX * 2) + 1][1][1];
	Block* water4 = &area.m_blocks[quarterMaxX * 3][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, water3, water4, s_water);
	// CO2
	Block* CO2_3 = &area.m_blocks[(quarterMaxX * 3) + 1][1][1];
	Block* CO2_4 = &area.m_blocks[maxX - 2][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, CO2_3, CO2_4, s_CO2);
	CHECK(area.m_fluidGroups.size() == 4);
	FluidGroup* fgWater = water1->getFluidGroup(s_water);
	uint32_t totalVolume = fgWater->totalVolume() * 2;
	s_step = 1;
	while(s_step < steps)
	{
		for(FluidGroup* fluidGroup : area.m_unstableFluidGroups)
			fluidGroup->readStep();
		area.writeStep();
		s_step++;
	}
	uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
	uint32_t expectedHeight = std::max(1u, maxZ / 2);
	uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
	CHECK(area.m_unstableFluidGroups.empty());
	fgWater = water1->getFluidGroup(s_water);
	FluidGroup* fgCO2 = water2->getFluidGroup(s_CO2);
	CHECK(fgWater->totalVolume() == totalVolume);
	CHECK(fgCO2->totalVolume() == totalVolume);
	CHECK(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(area.m_fluidGroups.size() == 2);
}
TEST_CASE("trench test 2 fluids merge scale 4-1")
{
	trenchTest2FluidsMerge(4, 1, 6);
}
TEST_CASE("trench test 2 fluids merge scale 4-4")
{
	trenchTest2FluidsMerge(4, 4, 6);
}
TEST_CASE("trench test 2 fluids merge scale 8-4")
{
	trenchTest2FluidsMerge(8, 4, 8);
}
TEST_CASE("trench test 2 fluids merge scale 16-4")
{
	trenchTest2FluidsMerge(16, 4, 12);
}
void trenchTest3FluidsMerge(uint32_t scaleL, uint32_t scaleW, uint32_t steps)
{
	uint32_t maxX = scaleL + 2;
	uint32_t maxY = scaleW + 2;
	uint32_t maxZ = scaleW + 1;
	uint32_t quarterMaxX = maxX / 4;
	Area area(maxX, maxY, maxZ);
	s_step = 0;
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, maxZ - 1, s_stone);
	// Water
	Block* water1 = &area.m_blocks[1][1][1];
	Block* water2 = &area.m_blocks[quarterMaxX][maxY - 2][maxZ - 1];		
	setFullFluidCuboid(area, water1, water2, s_water);
	// CO2
	Block* CO2_1 = &area.m_blocks[quarterMaxX + 1][1][1];
	Block* CO2_2 = &area.m_blocks[(quarterMaxX * 2)][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, CO2_1, CO2_2, s_CO2);
	// Mercury
	Block* mercury1 = &area.m_blocks[(quarterMaxX * 2) + 1][1][1];
	Block* mercury2 = &area.m_blocks[quarterMaxX * 3][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, mercury1, mercury2, s_mercury);
	// CO2
	Block* CO2_3 = &area.m_blocks[(quarterMaxX * 3) + 1][1][1];
	Block* CO2_4 = &area.m_blocks[maxX - 2][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, CO2_3, CO2_4, s_CO2);
	CHECK(area.m_fluidGroups.size() == 4);
	FluidGroup* fgWater = water1->getFluidGroup(s_water);
	uint32_t totalVolumeWater = fgWater->totalVolume();
	uint32_t totalVolumeMercury = totalVolumeWater;
	uint32_t totalVolumeCO2 = totalVolumeWater * 2;
	s_step = 1;
	while(s_step < steps)
	{
		for(FluidGroup* fluidGroup : area.m_unstableFluidGroups)
			fluidGroup->readStep();
		area.writeStep();
		s_step++;
	}
	uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
	uint32_t expectedHeight = std::max(1u, maxZ / 4);
	uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
	CHECK(area.m_unstableFluidGroups.empty());
	fgWater = getFluidGroup(area, s_water);
	FluidGroup* fgCO2 = getFluidGroup(area, s_CO2);
	FluidGroup* fgMercury = getFluidGroup(area, s_mercury);
	CHECK(fgWater != nullptr);
	CHECK(fgCO2 != nullptr);
	CHECK(fgMercury != nullptr);
	CHECK(fgWater->totalVolume() == totalVolumeWater);
	CHECK(fgCO2->totalVolume() == totalVolumeCO2);
	CHECK(fgMercury->totalVolume() == totalVolumeMercury);
	CHECK(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK((fgCO2->m_drainQueue.m_set.size() == expectedBlocks || fgCO2->m_drainQueue.m_set.size() == expectedBlocks * 2));
	CHECK(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgWater->m_stable);
	CHECK(fgCO2->m_stable);
	CHECK(fgMercury->m_stable);
	CHECK(area.m_fluidGroups.size() == 3);
}
TEST_CASE("trench test 3 fluids merge scale 4-1")
{
	trenchTest3FluidsMerge(4, 1, 8);
}
TEST_CASE("trench test 3 fluids merge scale 4-4")
{
	trenchTest3FluidsMerge(4, 4, 8);
}
TEST_CASE("trench test 3 fluids merge scale 8-4")
{
	trenchTest3FluidsMerge(8, 4, 15);
}
TEST_CASE("trench test 3 fluids merge scale 16-4")
{
	trenchTest3FluidsMerge(16, 4, 24);
}
void fourFluidsTest(uint32_t scale, uint32_t steps)
{
	uint32_t maxX = (scale * 2) + 2;
	uint32_t maxY = (scale * 2) + 2;
	uint32_t maxZ = (scale * 1) + 1;
	uint32_t halfMaxX = maxX / 2;
	uint32_t halfMaxY = maxY / 2;
	Area area(maxX, maxY, maxZ);
	s_step = 0;
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, maxZ - 1, s_stone);
	std::vector<FluidGroup*> newlySplit;
	// Water is at 0,0
	Block* water1 = &area.m_blocks[1][1][1];
	Block* water2 = &area.m_blocks[halfMaxX - 1][halfMaxY - 1][maxZ - 1];		
	setFullFluidCuboid(area, water1, water2, s_water);
	// CO2 is at 0,1
	Block* CO2_1 = &area.m_blocks[1][halfMaxY][1];
	Block* CO2_2 = &area.m_blocks[halfMaxX - 1][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, CO2_1, CO2_2, s_CO2);
	// Lava is at 1,0
	Block* lava1 = &area.m_blocks[halfMaxX][1][1];
	Block* lava2 = &area.m_blocks[maxX - 2][halfMaxY - 1][maxZ - 1];
	setFullFluidCuboid(area, lava1, lava2, s_lava);
	// Mercury is at 1,1
	Block* mercury1 = &area.m_blocks[halfMaxX][halfMaxY][1];
	Block* mercury2 = &area.m_blocks[maxX - 2][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, mercury1, mercury2, s_mercury);
	CHECK(area.m_fluidGroups.size() == 4);
	FluidGroup* fgWater = water1->getFluidGroup(s_water);
	FluidGroup* fgCO2 = CO2_1->getFluidGroup(s_CO2);
	FluidGroup* fgLava = lava1->getFluidGroup(s_lava);
	FluidGroup* fgMercury = mercury1->getFluidGroup(s_mercury);
	CHECK(!fgWater->m_merged);
	CHECK(!fgCO2->m_merged);
	CHECK(!fgLava->m_merged);
	CHECK(!fgMercury->m_merged);
	uint32_t totalVolume = fgWater->totalVolume();
	s_step = 1;
	while(s_step < steps)
	{
		for(FluidGroup* fluidGroup : area.m_unstableFluidGroups)
			fluidGroup->readStep();
		area.writeStep();
		s_step++;
	}
	uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
	uint32_t expectedHeight = ((maxZ - 2) / 4) + 1;
	uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
	CHECK(area.m_fluidGroups.size() == 4);
	fgMercury = getFluidGroup(area, s_mercury);
	fgWater = getFluidGroup(area, s_water);
	fgLava = getFluidGroup(area, s_lava);
	fgCO2 = getFluidGroup(area, s_CO2);
	CHECK(fgWater->m_stable);
	CHECK(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgWater->totalVolume() == totalVolume);
	CHECK(fgCO2->m_stable);
	CHECK(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgCO2->totalVolume() == totalVolume);
	CHECK(fgLava->m_stable);
	CHECK(fgLava->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgLava->totalVolume() == totalVolume);
	CHECK(fgMercury->m_stable);
	CHECK(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgMercury->totalVolume() == totalVolume);
	CHECK(area.m_blocks[1][1][1].m_fluids.contains(s_lava));
	CHECK(area.m_blocks[1][1][maxZ - 1].m_fluids.contains(s_CO2));
}
TEST_CASE("four fluids scale 2")
{
	fourFluidsTest(2, 10);
}
// Scale 3 doesn't work due to rounding issues with expectedBlocks.
TEST_CASE("four fluids scale 4")
{
	fourFluidsTest(4, 21);
}
TEST_CASE("four fluids scale 5")
{
	fourFluidsTest(5, 28);
}
TEST_CASE("four fluids scale 6")
{
	fourFluidsTest(6, 30);
}
