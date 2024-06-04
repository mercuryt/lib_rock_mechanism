#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/actor.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/threadedTask.h"
#include "config.h"
#include "types.h"

TEST_CASE("Area")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const FluidType& water = FluidType::byName("water");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation{L"", 1};
	Area& area = simulation.m_hasAreas->createArea(10, 10, 10);
	Blocks& blocks = area.getBlocks();
	SUBCASE("Make Area")
	{
		CHECK(blocks.m_sizeX == 10);
		CHECK(blocks.m_sizeY == 10);
		CHECK(blocks.m_sizeZ == 10);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		CHECK(water.name == "water");
		CHECK(blocks.solid_get(blocks.getIndex({5, 5, 0})) == marble);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
	}
	SUBCASE("Test fluid in area")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block1 = blocks.getIndex({5, 5, 1});
		BlockIndex block2 = blocks.getIndex({5, 5, 2});
		blocks.solid_setNot(block1);
		blocks.fluid_add(block2, 100, water);
		FluidGroup* fluidGroup = blocks.fluid_getGroup(block2, water);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		simulation.m_step++;
		REQUIRE(area.m_hasFluidGroups.getUnstable().size() == 1);
		REQUIRE(blocks.fluid_getTotalVolume(block1) == 100);
		REQUIRE(blocks.fluid_getTotalVolume(block2) == 0);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		REQUIRE(fluidGroup->m_stable);
		REQUIRE(!area.m_hasFluidGroups.getUnstable().contains(fluidGroup));
	}
	SUBCASE("Cave in falls in fluid and pistons it up with threading")
	{
		if constexpr(!Config::fluidPiston)
			return
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block1 = blocks.getIndex({5, 5, 1});
		BlockIndex block2 = blocks.getIndex({5, 5, 2});
		blocks.solid_setNot(block1);
		blocks.fluid_add(block1, 100, water);
		blocks.solid_set(block2, marble, false);
		area.m_caveInCheck.insert(block2);
		FluidGroup* fluidGroup = blocks.fluid_getGroup(block1, water);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		simulation.m_step++;
		REQUIRE(area.m_hasFluidGroups.getUnstable().size() == 1);
		REQUIRE(area.m_hasFluidGroups.getUnstable().contains(fluidGroup));
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(blocks.fluid_getTotalVolume(block1) == 0);
		REQUIRE(blocks.solid_get(block1) == marble);
		REQUIRE(!blocks.solid_is(block2));
		REQUIRE(fluidGroup->m_excessVolume == 100);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 0);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(block2));
		REQUIRE(blocks.fluid_canEnterEver(block2));
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		REQUIRE(fluidGroup->m_fillQueue.m_queue.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_queue[0].block == block2);
		REQUIRE(fluidGroup->m_fillQueue.m_queue[0].delta == 100);
		REQUIRE(!fluidGroup->m_stable);
		area.writeStep();
		simulation.m_step++;
		REQUIRE(blocks.fluid_getTotalVolume(block2) == 100);
		REQUIRE(fluidGroup->m_excessVolume == 0);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
	}
	SUBCASE("Test move with threading")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex({1, 1, 1});
		BlockIndex destination = blocks.getIndex({8, 8, 1});
		Actor& actor = simulation.m_hasActors->createActor(ActorParamaters{
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
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
		REQUIRE(actor.m_location == origin);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		simulation.m_eventSchedule.execute(simulation.m_step);
		REQUIRE(actor.m_location != origin);
		while(actor.m_location != destination)
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(actor.m_canMove.getDestination() == BLOCK_INDEX_MAX);
		REQUIRE(!actor.m_canMove.hasEvent());
	}
	SUBCASE("Test mist spreads")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex({5, 5, 1});
		BlockIndex block1 = blocks.getIndex({5, 6, 1});
		BlockIndex block2 = blocks.getIndex({6, 6, 1});
		BlockIndex block3 = blocks.getIndex({5, 5, 2});
		blocks.fluid_spawnMist(origin, water);
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
		REQUIRE(blocks.fluid_getMist(block1) == nullptr);
		area.readStep();
		simulation.m_pool.wait_for_tasks();
		area.writeStep();
		simulation.m_eventSchedule.execute(simulation.m_step);
		simulation.m_step++;
		REQUIRE(blocks.fluid_getMist(origin) == &water);
		REQUIRE(blocks.fluid_getMist(block1) == &water);
		REQUIRE(blocks.fluid_getMist(block2) == nullptr);
		REQUIRE(blocks.fluid_getMist(block3) == &water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep +1 )
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(blocks.fluid_getMist(origin) == &water);
		REQUIRE(blocks.fluid_getMist(block1) == &water);
		REQUIRE(blocks.fluid_getMist(block2) == &water);
		REQUIRE(blocks.fluid_getMist(block3) == &water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(blocks.fluid_getMist(origin) == nullptr);
		REQUIRE(blocks.fluid_getMist(block1) == &water);
		REQUIRE(blocks.fluid_getMist(block2) == &water);
		REQUIRE(blocks.fluid_getMist(block3) == &water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(blocks.fluid_getMist(origin) == nullptr);
		REQUIRE(blocks.fluid_getMist(block1) == nullptr);
		REQUIRE(blocks.fluid_getMist(block2) == &water);
		REQUIRE(blocks.fluid_getMist(block3) == nullptr);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.readStep();
			simulation.m_pool.wait_for_tasks();
			area.writeStep();
			simulation.m_eventSchedule.execute(simulation.m_step);
			simulation.m_step++;
		}
		REQUIRE(blocks.fluid_getMist(origin) == nullptr);
		REQUIRE(blocks.fluid_getMist(block1) == nullptr);
		REQUIRE(blocks.fluid_getMist(block2) == nullptr);
		REQUIRE(blocks.fluid_getMist(block3) == nullptr);
	}
}
TEST_CASE("vision-threading")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation{L"", 1};
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Blocks& blocks = area.getBlocks();
	BlockIndex block1 = blocks.getIndex({3, 3, 1});
	BlockIndex block2 = blocks.getIndex({7, 7, 1});
	Actor& a1 = simulation.m_hasActors->createActor(ActorParamaters{
		.species=dwarf, 
		.location=block1,
		.area=&area,
	});
	REQUIRE(area.m_hasActors.m_visionFacadeBuckets.getForStep(a1.m_id).size() == 1);
	REQUIRE(&a1.m_canSee.m_hasVisionFacade.getVisionFacade() == &area.m_hasActors.m_visionFacadeBuckets.getForStep(a1.m_id));
	Actor& a2 = simulation.m_hasActors->createActor(ActorParamaters{
		.species=dwarf, 
		.location=block2,
		.area=&area,
	});
	REQUIRE(area.m_hasActors.m_visionFacadeBuckets.getForStep(a2.m_id).size() == 1);
	REQUIRE(!a2.m_canSee.m_hasVisionFacade.empty());
	REQUIRE(&a2.m_canSee.m_hasVisionFacade.getVisionFacade() == &area.m_hasActors.m_visionFacadeBuckets.getForStep(a2.m_id));
	area.readStep();
	simulation.m_pool.wait_for_tasks();
	area.writeStep();
	REQUIRE(a1.m_canSee.isCurrentlyVisible(a2));
	simulation.m_step++;
	area.readStep();
	simulation.m_pool.wait_for_tasks();
	area.writeStep();
	REQUIRE(a2.m_canSee.isCurrentlyVisible(a1));
}
TEST_CASE("multiMergeOnAdd")
{
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation{L"", 1};
	Area& area = simulation.m_hasAreas->createArea(2,2,1);
	Blocks& blocks = area.getBlocks();
	BlockIndex block1 = blocks.getIndex({0, 0, 0});
	BlockIndex block2 = blocks.getIndex({0, 1, 0});
	BlockIndex block3 = blocks.getIndex({1, 0, 0});
	BlockIndex block4 = blocks.getIndex({1, 1, 0});
	blocks.fluid_add(block1, Config::maxBlockVolume, water);
	REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
	blocks.fluid_add(block4, Config::maxBlockVolume, water);
	REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
	blocks.fluid_add(block2, Config::maxBlockVolume, water);
	REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
	blocks.fluid_add(block3, Config::maxBlockVolume, water);
	REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
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
	Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
	Blocks& blocks = area.getBlocks();
	simulation.m_step = 0;
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
	std::vector<FluidGroup*> newlySplit;
	// Water is at 0,0
	BlockIndex water1 = blocks.getIndex({1, 1, 1});
	BlockIndex water2 = blocks.getIndex({halfMaxX - 1, halfMaxY - 1, maxZ - 1});		
	areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
	// CO2 is at 0,1
	BlockIndex CO2_1 = blocks.getIndex({1, halfMaxY, 1});
	BlockIndex CO2_2 = blocks.getIndex({halfMaxX - 1, maxY - 2, maxZ - 1});
	areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
	// Lava is at 1,0
	BlockIndex lava1 = blocks.getIndex({halfMaxX, 1, 1});
	BlockIndex lava2 = blocks.getIndex({maxX - 2, halfMaxY - 1, maxZ - 1});
	areaBuilderUtil::setFullFluidCuboid(area, lava1, lava2, lava);
	// Mercury is at 1,1
	BlockIndex mercury1 = blocks.getIndex({halfMaxX, halfMaxY, 1});
	BlockIndex mercury2 = blocks.getIndex({maxX - 2, maxY - 2, maxZ - 1});
	areaBuilderUtil::setFullFluidCuboid(area, mercury1, mercury2, mercury);
	REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
	FluidGroup* fgWater = blocks.fluid_getGroup(water1, water);
	FluidGroup* fgCO2 = blocks.fluid_getGroup(CO2_1, CO2);
	FluidGroup* fgLava = blocks.fluid_getGroup(lava1, lava);
	FluidGroup* fgMercury = blocks.fluid_getGroup(mercury1, mercury);
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
	REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
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
	REQUIRE(blocks.fluid_contains(blocks.getIndex({1, 1, 1}), mercury));
	REQUIRE(blocks.fluid_contains(blocks.getIndex({1, 1, maxZ - 1}), CO2));
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
