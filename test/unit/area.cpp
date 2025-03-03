#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/actors/actors.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/animalSpecies.h"
#include "config.h"
#include "types.h"

TEST_CASE("Area")
{
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static FluidTypeId water = FluidType::byName(L"water");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	Simulation simulation(L"", Step::create(1));
	Area& area = simulation.m_hasAreas->createArea(10, 10, 10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	SUBCASE("Make Area")
	{
		CHECK(blocks.m_sizeX == 10);
		CHECK(blocks.m_sizeY == 10);
		CHECK(blocks.m_sizeZ == 10);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		CHECK(FluidType::getName(water) == L"water");
		CHECK(blocks.solid_get(blocks.getIndex_i(5, 5, 0)) == marble);
		area.doStep();
	}
	SUBCASE("Test fluid in area")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		blocks.solid_setNot(block1);
		blocks.fluid_add(block2, CollisionVolume::create(100), water);
		FluidGroup* fluidGroup = blocks.fluid_getGroup(block2, water);
		area.doStep();
		++simulation.m_step;
		CHECK(area.m_hasFluidGroups.getUnstable().size() == 1);
		CHECK(blocks.fluid_getTotalVolume(block1) == 100);
		CHECK(blocks.fluid_getTotalVolume(block2) == 0);
		area.doStep();
		CHECK(fluidGroup->m_stable);
		CHECK(!area.m_hasFluidGroups.getUnstable().contains(fluidGroup));
	}
	SUBCASE("Cave in falls in fluid and pistons it up with threading")
	{
		if constexpr(!Config::fluidPiston)
			return
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		blocks.solid_setNot(block1);
		blocks.fluid_add(block1, CollisionVolume::create(100), water);
		blocks.solid_set(block2, marble, false);
		area.m_caveInCheck.add(block2);
		FluidGroup* fluidGroup = blocks.fluid_getGroup(block1, water);
		area.doStep();
		++simulation.m_step;
		CHECK(area.m_hasFluidGroups.getUnstable().size() == 1);
		CHECK(area.m_hasFluidGroups.getUnstable().contains(fluidGroup));
		CHECK(!fluidGroup->m_stable);
		CHECK(blocks.fluid_getTotalVolume(block1) == 0);
		CHECK(blocks.solid_get(block1) == marble);
		CHECK(!blocks.solid_is(block2));
		CHECK(fluidGroup->m_excessVolume == 100);
		CHECK(fluidGroup->m_drainQueue.m_set.size() == 0);
		CHECK(fluidGroup->m_fillQueue.m_set.size() == 1);
		CHECK(fluidGroup->m_fillQueue.m_set.contains(block2));
		CHECK(blocks.fluid_canEnterEver(block2));
		area.doStep();
		++simulation.m_step;
		CHECK(blocks.fluid_getTotalVolume(block2) == 100);
		CHECK(fluidGroup->m_excessVolume == 0);
		CHECK(!fluidGroup->m_stable);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
	}
	SUBCASE("Test move with threading")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(1, 1, 1);
		BlockIndex destination = blocks.getIndex_i(8, 8, 1);
		ActorIndex actor = actors.create(ActorParamaters{
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		area.doStep();
		++simulation.m_step;
		CHECK(actors.move_getPath(actor).size() == 7);
		CHECK(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		CHECK(actors.move_hasEvent(actor));
		Step scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		float seconds = (float)scheduledStep.get() / (float)Config::stepsPerSecond.get();
		CHECK(seconds > 0.6f);
		CHECK(seconds < 0.9f);
		while(simulation.m_step != scheduledStep)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(actors.getLocation(actor) == origin);
		area.doStep();
		simulation.m_eventSchedule.doStep(simulation.m_step);
		CHECK(actors.getLocation(actor) != origin);
		while(actors.getLocation(actor) != destination)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(actors.move_getDestination(actor).empty());
		CHECK(!actors.move_hasEvent(actor));
	}
	SUBCASE("Test mist spreads")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(5, 5, 1);
		BlockIndex block1 = blocks.getIndex_i(5, 6, 1);
		BlockIndex block2 = blocks.getIndex_i(6, 6, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 5, 2);
		blocks.fluid_spawnMist(origin, water);
		Step scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		CHECK(scheduledStep == 11);
		while(simulation.m_step != scheduledStep)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(blocks.fluid_getMist(block1).empty());
		area.doStep();
		simulation.m_eventSchedule.doStep(simulation.m_step);
		++simulation.m_step;
		CHECK(blocks.fluid_getMist(origin) == water);
		CHECK(blocks.fluid_getMist(block1) == water);
		CHECK(blocks.fluid_getMist(block2).empty());
		CHECK(blocks.fluid_getMist(block3) == water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1 )
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(blocks.fluid_getMist(origin) == water);
		CHECK(blocks.fluid_getMist(block1) == water);
		CHECK(blocks.fluid_getMist(block2) == water);
		CHECK(blocks.fluid_getMist(block3) == water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(blocks.fluid_getMist(origin).empty());
		CHECK(blocks.fluid_getMist(block1) == water);
		CHECK(blocks.fluid_getMist(block2) == water);
		CHECK(blocks.fluid_getMist(block3) == water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(blocks.fluid_getMist(origin).empty());
		CHECK(blocks.fluid_getMist(block1).empty());
		CHECK(blocks.fluid_getMist(block2) == water);
		CHECK(blocks.fluid_getMist(block3).empty());
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(blocks.fluid_getMist(origin).empty());
		CHECK(blocks.fluid_getMist(block1).empty());
		CHECK(blocks.fluid_getMist(block2).empty());
		CHECK(blocks.fluid_getMist(block3).empty());
	}
}
TEST_CASE("vision-threading")
{
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	Simulation simulation(L"", Step::create(100000000000));
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	BlockIndex block1 = blocks.getIndex_i(3, 3, 1);
	BlockIndex block2 = blocks.getIndex_i(7, 7, 1);
	Point3D point1 = blocks.getCoordinates(block1);
	Point3D point2 = blocks.getCoordinates(block2);
	CHECK(point2.isInFrontOf(point1, point1.getFacingTwords(point2)));
	CHECK(point1.isInFrontOf(point2, point2.getFacingTwords(point1)));
	ActorIndex a1 = actors.create(ActorParamaters{
		.species=dwarf,
		.location=block1,
		.facing=point1.getFacingTwords(point2),
	});
	CHECK(area.m_visionRequests.size() == 1);
	ActorIndex a2 = actors.create(ActorParamaters{
		.species=dwarf,
		.location=block2,
		.facing=point2.getFacingTwords(point1),
	});
	CHECK(area.m_visionRequests.size() == 2);
	simulation.doStep();
	CHECK(actors.vision_canSeeActor(a1, a2));
	CHECK(actors.vision_canSeeActor(a2, a1));
}
TEST_CASE("multiMergeOnAdd")
{
	static FluidTypeId water = FluidType::byName(L"water");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(2,2,1);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	BlockIndex block1 = blocks.getIndex_i(0, 0, 0);
	BlockIndex block2 = blocks.getIndex_i(0, 1, 0);
	BlockIndex block3 = blocks.getIndex_i(1, 0, 0);
	BlockIndex block4 = blocks.getIndex_i(1, 1, 0);
	blocks.fluid_add(block1, Config::maxBlockVolume, water);
	CHECK(area.m_hasFluidGroups.getAll().size() == 1);
	blocks.fluid_add(block4, Config::maxBlockVolume, water);
	CHECK(area.m_hasFluidGroups.getAll().size() == 2);
	blocks.fluid_add(block2, Config::maxBlockVolume, water);
	CHECK(area.m_hasFluidGroups.getAll().size() == 1);
	blocks.fluid_add(block3, Config::maxBlockVolume, water);
	CHECK(area.m_hasFluidGroups.getAll().size() == 1);
	simulation.doStep();
}
inline void fourFluidsTestParallel(uint32_t scale, Step steps)
{
	uint32_t maxX = (scale * 2) + 2;
	uint32_t maxY = (scale * 2) + 2;
	uint32_t maxZ = (scale * 1) + 1;
	uint32_t halfMaxX = maxX / 2;
	uint32_t halfMaxY = maxY / 2;
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static FluidTypeId water = FluidType::byName(L"water");
	static FluidTypeId CO2 = FluidType::byName(L"CO2");
	static FluidTypeId mercury = FluidType::byName(L"mercury");
	static FluidTypeId lava = FluidType::byName(L"lava");
	Simulation simulation(L"", Step::create(0));
	Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
	std::vector<FluidGroup*> newlySplit;
	// Water is at 0,0
	BlockIndex water1 = blocks.getIndex_i(1, 1, 1);
	BlockIndex water2 = blocks.getIndex_i(halfMaxX - 1, halfMaxY - 1, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
	// CO2 is at 0,1
	BlockIndex CO2_1 = blocks.getIndex_i(1, halfMaxY, 1);
	BlockIndex CO2_2 = blocks.getIndex_i(halfMaxX - 1, maxY - 2, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
	// Lava is at 1,0
	BlockIndex lava1 = blocks.getIndex_i(halfMaxX, 1, 1);
	BlockIndex lava2 = blocks.getIndex_i(maxX - 2, halfMaxY - 1, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(area, lava1, lava2, lava);
	// Mercury is at 1,1
	BlockIndex mercury1 = blocks.getIndex_i(halfMaxX, halfMaxY, 1);
	BlockIndex mercury2 = blocks.getIndex_i(maxX - 2, maxY - 2, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(area, mercury1, mercury2, mercury);
	CHECK(area.m_hasFluidGroups.getAll().size() == 4);
	FluidGroup* fgWater = blocks.fluid_getGroup(water1, water);
	FluidGroup* fgCO2 = blocks.fluid_getGroup(CO2_1, CO2);
	FluidGroup* fgLava = blocks.fluid_getGroup(lava1, lava);
	FluidGroup* fgMercury = blocks.fluid_getGroup(mercury1, mercury);
	CHECK(!fgWater->m_merged);
	CHECK(!fgCO2->m_merged);
	CHECK(!fgLava->m_merged);
	CHECK(!fgMercury->m_merged);
	CollisionVolume totalVolume = fgWater->totalVolume(area);
	simulation.m_step.set(1);
	while(simulation.m_step < steps)
	{
		area.doStep();
		++simulation.m_step;
	}
	uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
	uint32_t expectedHeight = ((maxZ - 2) / 4) + 1;
	uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
	fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
	fgWater = areaBuilderUtil::getFluidGroup(area, water);
	fgLava = areaBuilderUtil::getFluidGroup(area, lava);
	fgCO2 = areaBuilderUtil::getFluidGroup(area, CO2);
	CHECK(area.m_hasFluidGroups.getAll().size() == 4);
	CHECK(fgWater->m_stable);
	if(scale != 3)
		CHECK(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgWater->totalVolume(area) == totalVolume);
	CHECK(fgCO2->m_stable);
	CHECK(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgCO2->totalVolume(area) == totalVolume);
	CHECK(fgMercury->m_stable);
	CHECK(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgMercury->totalVolume(area) == totalVolume);
	CHECK(fgLava->m_stable);
	if(scale != 3)
		CHECK(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
	CHECK(fgMercury->totalVolume(area) == totalVolume);
	CHECK(blocks.fluid_contains(blocks.getIndex_i(1, 1, 1), mercury));
	CHECK(blocks.fluid_contains(blocks.getIndex_i(1, 1, maxZ - 1), CO2));
}
TEST_CASE("four fluids scale 2 parallel")
{
	fourFluidsTestParallel(2, Step::create(10));
}
TEST_CASE("four fluids scale 3 parallel")
{
	fourFluidsTestParallel(3, Step::create(15));
}
TEST_CASE("four fluids scale 4 parallel")
{
	fourFluidsTestParallel(4, Step::create(23));
}
TEST_CASE("four fluids scale 5 parallel")
{
	fourFluidsTestParallel(5, Step::create(24));
}
/*
// To slow for unit testing.
TEST_CASE("four fluids scale 10 parallel")
{
	fourFluidsTestParallel(10, Step::create(75));
}
*/
