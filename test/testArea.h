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
	area.registerActor(&actor);
	actor.setDestination(&destination);
	area.readStep();
	area.writeStep();
	s_step++;
	CHECK(actor.m_route->size() == 14);
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
TEST_CASE("four fluids scale 2")
{
	uint32_t scale = 2;
	uint32_t maxX = scale * 2;
	uint32_t maxY = scale * 2;
	uint32_t maxZ = scale * 1;
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
	s_step = 1;
	while(s_step < 8)
	{
		if(!fgWater->m_stable)
			fgWater->readStep();
		if(!fgCO2->m_stable)
			fgCO2->readStep();
		if(!fgLava->m_stable)
			fgLava->readStep();
		if(!fgMercury->m_stable)
			fgMercury->readStep();
		if(!fgWater->m_stable)
			fgWater->writeStep();
		if(!fgCO2->m_stable)
			fgCO2->writeStep();
		if(!fgLava->m_stable)
			fgLava->writeStep();
		if(!fgMercury->m_stable)
			fgMercury->writeStep();
		if(!fgWater->m_stable)
			fgWater->afterWriteStep();
		if(!fgCO2->m_stable)
			fgCO2->afterWriteStep();
		if(!fgLava->m_stable)
			fgLava->afterWriteStep();
		if(!fgMercury->m_stable)
			fgMercury->afterWriteStep();
		if(!fgWater->m_stable)
			fgWater->splitStep();
		if(!fgCO2->m_stable)
			fgCO2->splitStep();
		if(!fgLava->m_stable)
			fgLava->splitStep();
		if(!fgMercury->m_stable)
			fgMercury->splitStep();
		if(!fgWater->m_stable)
			fgWater->mergeStep();
		if(!fgCO2->m_stable)
			fgCO2->mergeStep();
		if(!fgLava->m_stable)
			fgLava->mergeStep();
		if(!fgMercury->m_stable)
			fgMercury->mergeStep();
		s_step++;
		CHECK(area.m_fluidGroups.size() == 4);
	}
	CHECK(fgWater->m_stable);
	CHECK(fgCO2->m_stable);
	CHECK(fgLava->m_stable);
	CHECK(fgMercury->m_stable);
}
TEST_CASE("four fluids scale 5")
{
	uint32_t scale = 5;
	uint32_t maxX = scale * 2;
	uint32_t maxY = scale * 2;
	uint32_t maxZ = scale * 1;
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
	s_step = 1;
	while(s_step < 21)
	{
		if(!fgWater->m_stable)
			fgWater->readStep();
		if(!fgCO2->m_stable)
			fgCO2->readStep();
		if(!fgLava->m_stable)
			fgLava->readStep();
		if(!fgMercury->m_stable)
			fgMercury->readStep();
		if(!fgWater->m_stable)
			fgWater->writeStep();
		if(!fgCO2->m_stable)
			fgCO2->writeStep();
		if(!fgLava->m_stable)
			fgLava->writeStep();
		if(!fgMercury->m_stable)
			fgMercury->writeStep();
		if(!fgWater->m_stable)
			fgWater->afterWriteStep();
		if(!fgCO2->m_stable)
			fgCO2->afterWriteStep();
		if(!fgLava->m_stable)
			fgLava->afterWriteStep();
		if(!fgMercury->m_stable)
			fgMercury->afterWriteStep();
		if(!fgWater->m_stable)
			fgWater->splitStep();
		if(!fgCO2->m_stable)
			fgCO2->splitStep();
		if(!fgLava->m_stable)
			fgLava->splitStep();
		if(!fgMercury->m_stable)
			fgMercury->splitStep();
		if(!fgWater->m_stable)
			fgWater->mergeStep();
		if(!fgCO2->m_stable)
			fgCO2->mergeStep();
		if(!fgLava->m_stable)
			fgLava->mergeStep();
		if(!fgMercury->m_stable)
			fgMercury->mergeStep();
		s_step++;
		CHECK(area.m_fluidGroups.size() == 4);
	}
	CHECK(fgWater->m_stable);
	CHECK(fgCO2->m_stable);
	CHECK(fgLava->m_stable);
	CHECK(fgMercury->m_stable);
}
