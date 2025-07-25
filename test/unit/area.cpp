#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/actors/actors.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/space/space.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/definitions/animalSpecies.h"
#include "config.h"
#include "numericTypes/types.h"

TEST_CASE("Area")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static FluidTypeId water = FluidType::byName("water");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation("", Step::create(1));
	Area& area = simulation.m_hasAreas->createArea(10, 10, 10);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	SUBCASE("Make Area")
	{
		CHECK(space.m_sizeX == 10);
		CHECK(space.m_sizeY == 10);
		CHECK(space.m_sizeZ == 10);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		CHECK(FluidType::getName(water) == "water");
		CHECK(space.solid_get(Point3D::create(5, 5, 0)) == marble);
		area.doStep();
	}
	SUBCASE("Test fluid in area")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D block1 = Point3D::create(5, 5, 1);
		Point3D block2 = Point3D::create(5, 5, 2);
		space.solid_setNot(block1);
		space.fluid_add(block2, CollisionVolume::create(100), water);
		FluidGroup* fluidGroup = space.fluid_getGroup(block2, water);
		area.doStep();
		++simulation.m_step;
		CHECK(area.m_hasFluidGroups.getUnstable().size() == 1);
		CHECK(space.fluid_getTotalVolume(block1) == 100);
		CHECK(space.fluid_getTotalVolume(block2) == 0);
		area.doStep();
		CHECK(fluidGroup->m_stable);
		CHECK(!area.m_hasFluidGroups.getUnstable().contains(fluidGroup));
	}
	SUBCASE("Cave in falls in fluid and pistons it up with threading")
	{
		if constexpr(!Config::fluidPiston)
			return
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D block1 = Point3D::create(5, 5, 1);
		Point3D block2 = Point3D::create(5, 5, 2);
		space.solid_setNot(block1);
		space.fluid_add(block1, CollisionVolume::create(100), water);
		space.solid_set(block2, marble, false);
		space.getSupport().maybeFall({block2, block2});
		FluidGroup* fluidGroup = space.fluid_getGroup(block1, water);
		area.doStep();
		++simulation.m_step;
		CHECK(area.m_hasFluidGroups.getUnstable().size() == 1);
		CHECK(area.m_hasFluidGroups.getUnstable().contains(fluidGroup));
		CHECK(!fluidGroup->m_stable);
		CHECK(space.fluid_getTotalVolume(block1) == 0);
		CHECK(space.solid_get(block1) == marble);
		CHECK(!space.solid_is(block2));
		CHECK(fluidGroup->m_excessVolume == 100);
		CHECK(fluidGroup->m_drainQueue.m_set.size() == 0);
		CHECK(fluidGroup->m_fillQueue.m_set.size() == 1);
		CHECK(fluidGroup->m_fillQueue.m_set.contains(block2));
		CHECK(space.fluid_canEnterEver(block2));
		area.doStep();
		++simulation.m_step;
		CHECK(space.fluid_getTotalVolume(block2) == 100);
		CHECK(fluidGroup->m_excessVolume == 0);
		CHECK(!fluidGroup->m_stable);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
	}
	SUBCASE("Test move with threading")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D origin = Point3D::create(1, 1, 1);
		Point3D destination = Point3D::create(8, 8, 1);
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
		Point3D origin = Point3D::create(5, 5, 1);
		Point3D block1 = Point3D::create(5, 6, 1);
		Point3D block2 = Point3D::create(6, 6, 1);
		Point3D block3 = Point3D::create(5, 5, 2);
		space.fluid_spawnMist(origin, water);
		Step scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		CHECK(scheduledStep == 11);
		while(simulation.m_step != scheduledStep)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(space.fluid_getMist(block1).empty());
		area.doStep();
		simulation.m_eventSchedule.doStep(simulation.m_step);
		++simulation.m_step;
		CHECK(space.fluid_getMist(origin) == water);
		CHECK(space.fluid_getMist(block1) == water);
		CHECK(space.fluid_getMist(block2).empty());
		CHECK(space.fluid_getMist(block3) == water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1 )
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(space.fluid_getMist(origin) == water);
		CHECK(space.fluid_getMist(block1) == water);
		CHECK(space.fluid_getMist(block2) == water);
		CHECK(space.fluid_getMist(block3) == water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(space.fluid_getMist(origin).empty());
		CHECK(space.fluid_getMist(block1) == water);
		CHECK(space.fluid_getMist(block2) == water);
		CHECK(space.fluid_getMist(block3) == water);
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(space.fluid_getMist(origin).empty());
		CHECK(space.fluid_getMist(block1).empty());
		CHECK(space.fluid_getMist(block2) == water);
		CHECK(space.fluid_getMist(block3).empty());
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		while(simulation.m_step != scheduledStep + 1)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(space.fluid_getMist(origin).empty());
		CHECK(space.fluid_getMist(block1).empty());
		CHECK(space.fluid_getMist(block2).empty());
		CHECK(space.fluid_getMist(block3).empty());
	}
}
TEST_CASE("vision-threading")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation("", Step::create(100000000000));
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Actors& actors = area.getActors();
	Point3D point1 = Point3D::create(3, 3, 1);
	Point3D point2 = Point3D::create(7, 7, 1);
	CHECK(point2.isInFrontOf(point1, point1.getFacingTwords(point2)));
	CHECK(point1.isInFrontOf(point2, point2.getFacingTwords(point1)));
	ActorIndex a1 = actors.create(ActorParamaters{
		.species=dwarf,
		.location=point1,
		.facing=point1.getFacingTwords(point2),
	});
	CHECK(area.m_visionRequests.size() == 1);
	ActorIndex a2 = actors.create(ActorParamaters{
		.species=dwarf,
		.location=point2,
		.facing=point2.getFacingTwords(point1),
	});
	CHECK(area.m_visionRequests.size() == 2);
	simulation.doStep();
	CHECK(actors.vision_canSeeActor(a1, a2));
	CHECK(actors.vision_canSeeActor(a2, a1));
}
TEST_CASE("multiMergeOnAdd")
{
	static FluidTypeId water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(2,2,1);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	Point3D block1 = Point3D::create(0, 0, 0);
	Point3D block2 = Point3D::create(0, 1, 0);
	Point3D block3 = Point3D::create(1, 0, 0);
	Point3D block4 = Point3D::create(1, 1, 0);
	space.fluid_add(block1, Config::maxPointVolume, water);
	CHECK(area.m_hasFluidGroups.getAll().size() == 1);
	space.fluid_add(block4, Config::maxPointVolume, water);
	CHECK(area.m_hasFluidGroups.getAll().size() == 2);
	space.fluid_add(block2, Config::maxPointVolume, water);
	CHECK(area.m_hasFluidGroups.getAll().size() == 1);
	space.fluid_add(block3, Config::maxPointVolume, water);
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
	static MaterialTypeId marble = MaterialType::byName("marble");
	static FluidTypeId water = FluidType::byName("water");
	static FluidTypeId CO2 = FluidType::byName("CO2");
	static FluidTypeId mercury = FluidType::byName("mercury");
	static FluidTypeId lava = FluidType::byName("lava");
	Simulation simulation("", Step::create(0));
	Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
	std::vector<FluidGroup*> newlySplit;
	// Water is at 0,0
	Point3D water1 = Point3D::create(1, 1, 1);
	Point3D water2 = Point3D::create(halfMaxX - 1, halfMaxY - 1, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
	// CO2 is at 0,1
	Point3D CO2_1 = Point3D::create(1, halfMaxY, 1);
	Point3D CO2_2 = Point3D::create(halfMaxX - 1, maxY - 2, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
	// Lava is at 1,0
	Point3D lava1 = Point3D::create(halfMaxX, 1, 1);
	Point3D lava2 = Point3D::create(maxX - 2, halfMaxY - 1, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(area, lava1, lava2, lava);
	// Mercury is at 1,1
	Point3D mercury1 = Point3D::create(halfMaxX, halfMaxY, 1);
	Point3D mercury2 = Point3D::create(maxX - 2, maxY - 2, maxZ - 1);
	areaBuilderUtil::setFullFluidCuboid(area, mercury1, mercury2, mercury);
	CHECK(area.m_hasFluidGroups.getAll().size() == 4);
	FluidGroup* fgWater = space.fluid_getGroup(water1, water);
	FluidGroup* fgCO2 = space.fluid_getGroup(CO2_1, CO2);
	FluidGroup* fgLava = space.fluid_getGroup(lava1, lava);
	FluidGroup* fgMercury = space.fluid_getGroup(mercury1, mercury);
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
	CHECK(space.fluid_contains(Point3D::create(1, 1, 1), mercury));
	CHECK(space.fluid_contains(Point3D::create(1, 1, maxZ - 1), CO2));
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
