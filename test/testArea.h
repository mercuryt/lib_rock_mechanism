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
	area.readStep();
	area.writeStep();
}
TEST_CASE("Test fluid in area")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	Block& block1 = area.m_blocks[5][5][1];
	Block& block2 = area.m_blocks[5][5][2];
	block1.setNotSolid();
	block2.addFluid(100, s_water);
	FluidGroup* fluidGroup = block2.getFluidGroup(s_water);
	area.readStep();
	area.writeStep();
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
