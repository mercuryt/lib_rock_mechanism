#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/threadedTask.h"
#include "config.h"

TEST_CASE("Area")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const FluidType& water = FluidType::byName("water");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation{L"", 1};
	Area& area = simulation.createArea(10, 10, 10);
	SUBCASE("Make Area")
	{
		CHECK(area.m_sizeX == 10);
		CHECK(area.m_sizeY == 10);
		CHECK(area.m_sizeZ == 10);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		CHECK(water.name == "water");
		CHECK(area.getBlock(5, 5, 0).getSolidMaterial() == marble);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
	}
	SUBCASE("Test fluid in area")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block1 = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(5, 5, 2);
		block1.setNotSolid();
		block2.addFluid(100, water);
		FluidGroup* fluidGroup = block2.getFluidGroup(water);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		simulation.m_step++;
		REQUIRE(area.m_unstableFluidGroups.size() == 1);
		REQUIRE(block1.m_totalFluidVolume == 100);
		REQUIRE(block2.m_totalFluidVolume == 0);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		REQUIRE(fluidGroup->m_stable);
		REQUIRE(!area.m_unstableFluidGroups.contains(fluidGroup));
	}
	SUBCASE("Cave in falls in fluid and pistons it up with threading")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block1 = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(5, 5, 2);
		block1.setNotSolid();
		block1.addFluid(100, water);
		block2.setSolid(marble);
		area.m_caveInCheck.insert(&block2);
		FluidGroup* fluidGroup = block1.getFluidGroup(water);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		simulation.m_step++;
		REQUIRE(area.m_unstableFluidGroups.size() == 1);
		REQUIRE(area.m_unstableFluidGroups.contains(fluidGroup));
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(block1.m_totalFluidVolume == 0);
		REQUIRE(block1.getSolidMaterial() == marble);
		REQUIRE(!block2.isSolid());
		REQUIRE(fluidGroup->m_excessVolume == 100);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 0);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(&block2));
		REQUIRE(block2.fluidCanEnterEver());
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		REQUIRE(fluidGroup->m_fillQueue.m_queue.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_queue[0].block == &block2);
		REQUIRE(fluidGroup->m_fillQueue.m_queue[0].delta == 100);
		REQUIRE(!fluidGroup->m_stable);
		area.writeStep();
		simulation.m_step++;
		REQUIRE(block2.m_totalFluidVolume == 100);
		REQUIRE(fluidGroup->m_excessVolume == 0);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(area.m_fluidGroups.size() == 1);
	}
	SUBCASE("Test move with threading")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(1, 1, 1);
		Block& destination = area.getBlock(8, 8, 1);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(destination);
		area.readStep();
		simulation.m_threadedTaskEngine.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		simulation.m_threadedTaskEngine.writeStep();
		simulation.m_step++;
		REQUIRE(actor.m_canMove.getPath().size() == 7);
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		REQUIRE(actor.m_canMove.hasEvent());
		Step scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		float seconds = (float)scheduledStep / (float)Config::stepsPerSecond;
		REQUIRE(seconds > 0.6f);
		REQUIRE(seconds < 0.9f);
		while(simulation.m_step != scheduledStep)
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(actor.m_location == &origin);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		simulation.m_eventSchedule.execute(simulation.m_step);
		REQUIRE(actor.m_location != &origin);
		while(actor.m_location != &destination)
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(actor.m_canMove.getDestination() == nullptr);
		REQUIRE(!actor.m_canMove.hasEvent());
	}
	SUBCASE("Test mist spreads")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(5, 5, 1);
		Block& block1 = area.getBlock(5, 6, 1);
		Block& block2 = area.getBlock(6, 6, 1);
		Block& block3 = area.getBlock(5, 5, 2);
		origin.m_mistSource = &water;
		origin.spawnMist(water);
		uint32_t scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		REQUIRE(scheduledStep == 11);
		while(simulation.m_step != scheduledStep)
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(block1.m_mist == nullptr);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		simulation.m_eventSchedule.execute(simulation.m_step);
		simulation.m_step++;
		REQUIRE(origin.m_mist == &water);
		REQUIRE(block1.m_mist == &water);
		REQUIRE(block2.m_mist == nullptr);
		REQUIRE(block3.m_mist == &water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep +1 )
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(origin.m_mist == &water);
		REQUIRE(block1.m_mist == &water);
		REQUIRE(block2.m_mist == &water);
		REQUIRE(block3.m_mist == &water);
		origin.m_mistSource = nullptr;
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(origin.m_mist == nullptr);
		REQUIRE(block1.m_mist == &water);
		REQUIRE(block2.m_mist == &water);
		REQUIRE(block3.m_mist == &water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(origin.m_mist == nullptr);
		REQUIRE(block1.m_mist == nullptr);
		REQUIRE(block2.m_mist == &water);
		REQUIRE(block3.m_mist == nullptr);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(origin.m_mist == nullptr);
		REQUIRE(block1.m_mist == nullptr);
		REQUIRE(block2.m_mist == nullptr);
		REQUIRE(block3.m_mist == nullptr);
	}
}
TEST_CASE("vision-threading")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation{L"", 1};
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Block& block1 = area.getBlock(3, 3, 1);
	Block& block2 = area.getBlock(7, 7, 1);
	Actor& a1 = simulation.createActor(dwarf, block1);
	REQUIRE(area.m_hasActors.m_visionBuckets.get(a1.m_id).size() == 1);
	REQUIRE(area.m_hasActors.m_visionBuckets.get(a1.m_id)[0] == &a1);
	Actor& a2 = simulation.createActor(dwarf, block2);
	REQUIRE(area.m_hasActors.m_visionBuckets.get(a2.m_id).size() == 1);
	REQUIRE(area.m_hasActors.m_visionBuckets.get(a2.m_id)[0] == &a2);
	area.readStep();
	simulation.m_pool.wait_for_tasks();
	area.writeStep();
	REQUIRE(a1.m_canSee.contains(&a2));
	simulation.m_step++;
	area.readStep();
	simulation.m_pool.wait_for_tasks();
	area.writeStep();
	REQUIRE(a2.m_canSee.contains(&a1));
}
TEST_CASE("multiMergeOnAdd")
{
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation{L"", 1};
	Area& area = simulation.createArea(2,2,1);
	Block& block1 = area.getBlock(0, 0, 0);
	Block& block2 = area.getBlock(0, 1, 0);
	Block& block3 = area.getBlock(1, 0, 0);
	Block& block4 = area.getBlock(1, 1, 0);
	block1.addFluid(Config::maxBlockVolume, water);
	REQUIRE(area.m_fluidGroups.size() == 1);
	block4.addFluid(Config::maxBlockVolume, water);
	REQUIRE(area.m_fluidGroups.size() == 2);
	block2.addFluid(Config::maxBlockVolume, water);
	REQUIRE(area.m_fluidGroups.size() == 1);
	block3.addFluid(Config::maxBlockVolume, water);
	REQUIRE(area.m_fluidGroups.size() == 1);
	simulation.doStep();
}
inline void fourFluidsTestParallel(uint32_t scale, uint32_t steps)
{
	uint32_t maxX = (scale * 2) + 2;
	uint32_t maxY = (scale * 2) + 2;
	uint32_t maxZ = (scale * 1) + 1;
	uint32_t halfMaxX = maxX / 2;
	uint32_t halfMaxY = maxY / 2;
	static const MaterialType& marble = MaterialType::byName("marble");
	static const FluidType& water = FluidType::byName("water");
	static const FluidType& CO2 = FluidType::byName("CO2");
	static const FluidType& mercury = FluidType::byName("mercury");
	static const FluidType& lava = FluidType::byName("lava");
	Simulation simulation;
	Area& area = simulation.createArea(maxX, maxY, maxZ);
	simulation.m_step = 0;
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
	std::vector<FluidGroup*> newlySplit;
	// Water is at 0,0
	Block& water1 = area.getBlock(1, 1, 1);
	Block& water2 = area.getBlock(halfMaxX - 1, halfMaxY - 1, maxZ - 1);		
	areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
	// CO2 is at 0,1
	Block& CO2_1 = area.getBlock(1, halfMaxY, 1);
	Block& CO2_2 = area.getBlock(halfMaxX - 1, maxY - 2, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(CO2_1, CO2_2, CO2);
	// Lava is at 1,0
	Block& lava1 = area.getBlock(halfMaxX, 1, 1);
	Block& lava2 = area.getBlock(maxX - 2, halfMaxY - 1, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(lava1, lava2, lava);
	// Mercury is at 1,1
	Block& mercury1 = area.getBlock(halfMaxX, halfMaxY, 1);
	Block& mercury2 = area.getBlock(maxX - 2, maxY - 2, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(mercury1, mercury2, mercury);
	REQUIRE(area.m_fluidGroups.size() == 4);
	FluidGroup* fgWater = water1.getFluidGroup(water);
	FluidGroup* fgCO2 = CO2_1.getFluidGroup(CO2);
	FluidGroup* fgLava = lava1.getFluidGroup(lava);
	FluidGroup* fgMercury = mercury1.getFluidGroup(mercury);
	REQUIRE(!fgWater->m_merged);
	REQUIRE(!fgCO2->m_merged);
	REQUIRE(!fgLava->m_merged);
	REQUIRE(!fgMercury->m_merged);
	uint32_t totalVolume = fgWater->totalVolume();
	simulation.m_step = 1;
	while(simulation.m_step < steps)
	{
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		simulation.m_step++;
	}
	uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
	uint32_t expectedHeight = ((maxZ - 2) / 4) + 1;
	uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
	fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
	fgWater = areaBuilderUtil::getFluidGroup(area, water);
	fgLava = areaBuilderUtil::getFluidGroup(area, lava);
	fgCO2 = areaBuilderUtil::getFluidGroup(area, CO2);
	REQUIRE(area.m_fluidGroups.size() == 4);
	REQUIRE(fgWater->m_stable);
	if(scale != 3)
		REQUIRE(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
	REQUIRE(fgWater->totalVolume() == totalVolume);
	REQUIRE(fgCO2->m_stable);
	REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
	REQUIRE(fgCO2->totalVolume() == totalVolume);
	REQUIRE(fgMercury->m_stable);
	REQUIRE(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
	REQUIRE(fgMercury->totalVolume() == totalVolume);
	REQUIRE(fgLava->m_stable);
	if(scale != 3)
		REQUIRE(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
	REQUIRE(fgMercury->totalVolume() == totalVolume);
	REQUIRE(area.getBlock(1, 1, 1).m_fluids.contains(&mercury));
	REQUIRE(area.getBlock(1, 1, maxZ - 1).m_fluids.contains(&CO2));
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
