TEST_CASE("Make Area")
{
	Area area(10,10,10);
	s_step = 0;
	CHECK(area.m_sizeX == 10);
	CHECK(area.m_sizeY == 10);
	CHECK(area.m_sizeZ == 10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	CHECK(s_water->name == "water");
	CHECK(area.m_blocks[5][5][0].getSolidMaterial() == s_stone);
	area.readStep();
	area.writeStep();
}
TEST_CASE("Test fluid in area")
{
	Area area(10,10,10);
	s_step = 0;
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	block1.setNotSolid();
	block2.addFluid(100, s_water);
	FluidGroup* fluidGroup = block2.getFluidGroup(s_water);
	area.readStep();
	area.writeStep();
	s_step++;
	CHECK(area.m_unstableFluidGroups.size() == 1);
	CHECK(block1.m_totalFluidVolume == 100);
	CHECK(block2.m_totalFluidVolume == 0);
	area.readStep();
	area.writeStep();
	CHECK(fluidGroup->m_stable);
	CHECK(!area.m_unstableFluidGroups.contains(fluidGroup));
}
TEST_CASE("Cave in falls in fluid and pistons it up with threading")
{
	Area area(10,10,10);
	s_step = 0;
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	block1.setNotSolid();
	block1.addFluid(100, s_water);
	block2.setSolid(s_stone);
	area.m_caveInCheck.insert(&block2);
	FluidGroup* fluidGroup = block1.getFluidGroup(s_water);
	area.readStep();
	area.writeStep();
	s_step++;
	CHECK(area.m_unstableFluidGroups.size() == 1);
	CHECK(area.m_unstableFluidGroups.contains(fluidGroup));
	CHECK(!fluidGroup->m_stable);
	CHECK(block1.m_totalFluidVolume == 0);
	CHECK(block1.getSolidMaterial() == s_stone);
	CHECK(!block2.isSolid());
	CHECK(fluidGroup->m_excessVolume == 100);
	CHECK(fluidGroup->m_drainQueue.m_set.size() == 0);
	CHECK(fluidGroup->m_fillQueue.m_set.size() == 1);
	CHECK(fluidGroup->m_fillQueue.m_set.contains(&block2));
	CHECK(block2.fluidCanEnterEver());
	area.readStep();
	s_pool.wait_for_tasks();
	CHECK(fluidGroup->m_fillQueue.m_queue.size() == 1);
	CHECK(fluidGroup->m_fillQueue.m_queue[0].block == &block2);
	CHECK(fluidGroup->m_fillQueue.m_queue[0].delta == 100);
	CHECK(!fluidGroup->m_stable);
	area.writeStep();
	s_step++;
	CHECK(block2.m_totalFluidVolume == 100);
	CHECK(fluidGroup->m_excessVolume == 0);
	CHECK(!fluidGroup->m_stable);
	CHECK(area.m_fluidGroups.size() == 1);
}
TEST_CASE("Test move with threading")
{
	Area area(10,10,10);
	s_step = 0;
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& origin = area.m_blocks[1][1][1];
	Block& destination = area.m_blocks[8][8][1];
	Actor actor(&origin, s_oneByOneFull, s_twoLegs);
	area.registerActor(actor);
	actor.setDestination(destination);
	area.readStep();
	area.writeStep();
	s_step++;
	CHECK(actor.m_route->size() == 7);
	uint32_t scheduledStep = area.m_scheduledEvents.begin()->first;
	while(s_step != scheduledStep)
	{
		area.readStep();
		area.writeStep();
		s_step++;
	}
	CHECK(actor.m_location == &origin);
	area.readStep();
	area.writeStep();
	CHECK(actor.m_location != &origin);
	while(actor.m_location != &destination)
	{
		area.readStep();
		area.writeStep();
		s_step++;
	}
	CHECK(actor.m_destination == nullptr);
	CHECK(area.m_scheduledEvents.empty());
}
TEST_CASE("Test mist spreads")
{
	Area area(10,10,10);
	s_step = 0;
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& origin = area.m_blocks[5][5][1];
	Block& block1 = area.m_blocks[5][6][1];
	Block& block2 = area.m_blocks[6][6][1];
	Block& block3 = area.m_blocks[5][5][2];
	origin.m_mistSource = s_water;
	origin.spawnMist(s_water);
	uint32_t scheduledStep = area.m_scheduledEvents.begin()->first;
	CHECK(scheduledStep == 10);
	while(s_step != scheduledStep)
	{
		area.readStep();
		area.writeStep();
		s_step++;
	}
	CHECK(block1.m_mist == nullptr);
	area.readStep();
	area.writeStep();
	s_step++;
	CHECK(origin.m_mist == s_water);
	CHECK(block1.m_mist == s_water);
	CHECK(block2.m_mist == nullptr);
	CHECK(block3.m_mist == s_water);
	CHECK(!area.m_scheduledEvents.empty());
	scheduledStep = area.m_scheduledEvents.begin()->first;
	CHECK(area.m_scheduledEvents.at(scheduledStep).size() == 6);
	while(s_step != scheduledStep +1 )
	{
		area.readStep();
		area.writeStep();
		s_step++;
	}
	CHECK(origin.m_mist == s_water);
	CHECK(block1.m_mist == s_water);
	CHECK(block2.m_mist == s_water);
	CHECK(block3.m_mist == s_water);
	CHECK(!area.m_scheduledEvents.empty());
	origin.m_mistSource = nullptr;
	scheduledStep = area.m_scheduledEvents.begin()->first;
	CHECK(area.m_scheduledEvents.at(scheduledStep).size() == 19);
	while(s_step != scheduledStep + 1)
	{
		area.readStep();
		area.writeStep();
		s_step++;
	}
	CHECK(origin.m_mist == nullptr);
	CHECK(block1.m_mist == s_water);
	CHECK(block2.m_mist == s_water);
	CHECK(block3.m_mist == s_water);
	scheduledStep = area.m_scheduledEvents.begin()->first;
	while(s_step != scheduledStep + 1)
	{
		area.readStep();
		area.writeStep();
		s_step++;
	}
	CHECK(origin.m_mist == nullptr);
	CHECK(block1.m_mist == nullptr);
	CHECK(block2.m_mist == s_water);
	CHECK(block3.m_mist == nullptr);
	scheduledStep = area.m_scheduledEvents.begin()->first;
	while(s_step != scheduledStep + 1)
	{
		area.readStep();
		area.writeStep();
		s_step++;
	}
	CHECK(origin.m_mist == nullptr);
	CHECK(block1.m_mist == nullptr);
	CHECK(block2.m_mist == nullptr);
	CHECK(block3.m_mist == nullptr);
	CHECK(area.m_scheduledEvents.empty());
}
void fourFluidsTestParallel(uint32_t scale, uint32_t steps)
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
	Block& water1 = area.m_blocks[1][1][1];
	Block& water2 = area.m_blocks[halfMaxX - 1][halfMaxY - 1][maxZ - 1];		
	setFullFluidCuboid(water1, water2, s_water);
	// CO2 is at 0,1
	Block& CO2_1 = area.m_blocks[1][halfMaxY][1];
	Block& CO2_2 = area.m_blocks[halfMaxX - 1][maxY - 2][maxZ - 1];
	setFullFluidCuboid(CO2_1, CO2_2, s_CO2);
	// Lava is at 1,0
	Block& lava1 = area.m_blocks[halfMaxX][1][1];
	Block& lava2 = area.m_blocks[maxX - 2][halfMaxY - 1][maxZ - 1];
	setFullFluidCuboid(lava1, lava2, s_lava);
	// Mercury is at 1,1
	Block& mercury1 = area.m_blocks[halfMaxX][halfMaxY][1];
	Block& mercury2 = area.m_blocks[maxX - 2][maxY - 2][maxZ - 1];
	setFullFluidCuboid(mercury1, mercury2, s_mercury);
	CHECK(area.m_fluidGroups.size() == 4);
	FluidGroup* fgWater = water1.getFluidGroup(s_water);
	FluidGroup* fgCO2 = CO2_1.getFluidGroup(s_CO2);
	FluidGroup* fgLava = lava1.getFluidGroup(s_lava);
	FluidGroup* fgMercury = mercury1.getFluidGroup(s_mercury);
	CHECK(!fgWater->m_merged);
	CHECK(!fgCO2->m_merged);
	CHECK(!fgLava->m_merged);
	CHECK(!fgMercury->m_merged);
	uint32_t totalVolume = fgWater->totalVolume();
	s_step = 1;
	while(s_step < steps)
	{
		area.readStep();
		area.writeStep();
		s_step++;
	}
	uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
	uint32_t expectedHeight = ((maxZ - 2) / 4) + 1;
	uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
	fgMercury = getFluidGroup(area, s_mercury);
	fgWater = getFluidGroup(area, s_water);
	fgLava = getFluidGroup(area, s_lava);
	fgCO2 = getFluidGroup(area, s_CO2);
	CHECK(area.m_fluidGroups.size() == 4);
	CHECK(fgWater->m_stable);
	if(scale != 3)
		CHECK(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgWater->totalVolume() == totalVolume);
	CHECK(fgCO2->m_stable);
	CHECK(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgCO2->totalVolume() == totalVolume);
	CHECK(fgLava->m_stable);
	CHECK(fgLava->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgLava->totalVolume() == totalVolume);
	CHECK(fgMercury->m_stable);
	if(scale != 3)
		CHECK(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgMercury->totalVolume() == totalVolume);
	CHECK(area.m_blocks[1][1][1].m_fluids.contains(s_lava));
	CHECK(area.m_blocks[1][1][maxZ - 1].m_fluids.contains(s_CO2));
}
TEST_CASE("four fluids scale 2 parallel")
{
	fourFluidsTestParallel(2, 10);
}
TEST_CASE("four fluids scale 3 parallel")
{
	fourFluidsTestParallel(3, 15);
}
TEST_CASE("four fluids scale 4 parallel")
{
	fourFluidsTestParallel(4, 23);
}
TEST_CASE("four fluids scale 5 parallel")
{
	fourFluidsTestParallel(5, 24);
}
TEST_CASE("four fluids scale 10 parallel")
{
	fourFluidsTestParallel(10, 75);
}
TEST_CASE("test vision with threading")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& block1 = area.m_blocks[3][3][1];
	Block& block2 = area.m_blocks[7][7][1];
	Actor a1(&block1, s_oneByOneFull, s_twoLegs);
	area.registerActor(a1);
	CHECK(area.m_visionBuckets.get(a1.m_id).size() == 1);
	CHECK(area.m_visionBuckets.get(a1.m_id)[0] == &a1);
	Actor a2(&block2, s_oneByOneFull, s_twoLegs);
	area.registerActor(a2);
	CHECK(area.m_visionBuckets.get(a2.m_id).size() == 1);
	CHECK(area.m_visionBuckets.get(a2.m_id)[0] == &a2);
	s_step = a1.m_id;
	area.readStep();
	area.writeStep();
	CHECK(a1.m_canSee.contains(&a2));
	s_step++;
	area.readStep();
	area.writeStep();
	CHECK(a2.m_canSee.contains(&a1));
}
