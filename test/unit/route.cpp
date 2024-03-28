#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actor.h"
#include "../../engine/materialType.h"
#include "../../engine/simulation.h"
TEST_CASE("route_10_10_10")
{
	Simulation simulation(L"", 1);
	static const MaterialType& marble = MaterialType::byName("marble");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const AnimalSpecies& troll = AnimalSpecies::byName("troll");
	static const AnimalSpecies& eagle = AnimalSpecies::byName("golden eagle");
	static const AnimalSpecies& carp = AnimalSpecies::byName("carp");
	static const FluidType& water = FluidType::byName("water");
	Area& area = simulation.createArea(10,10,10);
	SUBCASE("Route through open space")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(3, 3, 1);
		Block& destination = area.getBlock(7, 7, 1);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		auto& moveCostsToCache = pathThreadedTask.getFindsPath().getMoveCostsToCache();
		REQUIRE(!moveCostsToCache.empty());
		REQUIRE(moveCostsToCache.at(&origin).size() == 8);
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 4);
		pathThreadedTask.writeStep();
		REQUIRE(actor.m_canMove.getPath().size() == 4);
	}
	SUBCASE("Route around walls")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(3, 3, 1);
		Block& block1 = area.getBlock(5, 3, 1);
		Block& block2 = area.getBlock(5, 4, 1);
		Block& block3 = area.getBlock(5, 5, 1);
		Block& block4 = area.getBlock(5, 6, 1);
		Block& block5 = area.getBlock(5, 7, 1);
		Block& destination = area.getBlock(7, 7, 1);
		block1.setSolid(marble);
		block2.setSolid(marble);
		block3.setSolid(marble);
		block4.setSolid(marble);
		block5.setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getMoveCostsToCache().empty());
		REQUIRE(pathThreadedTask.getFindsPath().getMoveCostsToCache().at(&origin).size() == 8);
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 7);
		pathThreadedTask.writeStep();
		REQUIRE(actor.m_canMove.getPath().size() == 7);
	}
	SUBCASE("No route found")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(0, 0, 1);
		Block& block1 = area.getBlock(2, 0, 1);
		Block& block2 = area.getBlock(2, 1, 1);
		Block& block3 = area.getBlock(2, 2, 1);
		Block& block4 = area.getBlock(1, 2, 1);
		Block& block5 = area.getBlock(0, 2, 1);
		Block& destination = area.getBlock(7, 7, 1);
		block1.setSolid(marble);
		block2.setSolid(marble);
		block3.setSolid(marble);
		block4.setSolid(marble);
		block5.setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		pathThreadedTask.writeStep();
		REQUIRE(actor.m_canMove.getPath().empty());
	}
	SUBCASE("Walk")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(3, 3, 1);
		Block& block1 = area.getBlock(4, 4, 1);
		Block& destination = area.getBlock(5, 5, 1);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(destination);
		REQUIRE(!actor.m_canMove.hasEvent());
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		pathThreadedTask.writeStep();
		REQUIRE(actor.m_canMove.hasEvent());
		uint32_t scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		REQUIRE(scheduledStep == 8);
		simulation.m_step = scheduledStep;
		// step 1
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == &block1);
		REQUIRE(actor.m_canMove.hasEvent());
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		REQUIRE(scheduledStep == 15);
		simulation.m_step = scheduledStep;
		REQUIRE(actor.m_canMove.hasEvent());
		// step 2
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == &destination);
		REQUIRE(!actor.m_canMove.hasEvent());
		REQUIRE(actor.m_canMove.getPath().empty());
		REQUIRE(actor.m_canMove.getDestination() == nullptr);
	}
	SUBCASE("Repath when route is blocked")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(3, 3, 1);
		Block& block1 = area.getBlock(3, 4, 1);
		Block& block2 = area.getBlock(3, 5, 1);
		Block& block3 = area.getBlock(4, 5, 1);
		Block& destination = area.getBlock(3, 6, 1);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		pathThreadedTask.writeStep();
		REQUIRE(actor.m_canMove.hasEvent());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		// Step 1.
		uint32_t scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == &block1);
		REQUIRE(actor.m_canMove.hasEvent());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		block2.setSolid(marble);
		// Step 2.
		REQUIRE(actor.m_canMove.hasEvent());
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == &block1);
		REQUIRE(!actor.m_canMove.hasEvent());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 1);
		// Step 3.
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		pathThreadedTask2.writeStep();
		REQUIRE(actor.m_canMove.hasEvent());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == &block3);
	}
	SUBCASE("Unpathable route becomes pathable when blockage is removed.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(3, 3, 1);
		Block& wallStart = area.getBlock(0, 4, 1);
		Block& wallEnd = area.getBlock(9, 4, 2);
		Block& destination = area.getBlock(3, 6, 1);
		Block& inFrontOfWallStart = area.getBlock(0, 3, 1);
		areaBuilderUtil::setSolidWall(wallStart, wallEnd, marble);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(destination);
		simulation.doStep();
		REQUIRE(actor.m_canMove.getPath().empty());
		wallStart.setNotSolid();
		REQUIRE(wallStart.m_hasShapes.moveCostCacheIsEmpty());
		REQUIRE(inFrontOfWallStart.m_hasShapes.moveCostCacheIsEmpty());
		actor.m_canMove.setDestination(destination);
		simulation.doStep();
		REQUIRE(!actor.m_canMove.getPath().empty());
	}
	SUBCASE("two by two creature cannot path through one block gap")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(2, 2, 1);
		Block& destination = area.getBlock(8, 8, 1);
		area.getBlock(1, 5, 1).setSolid(marble);
		area.getBlock(3, 5, 1).setSolid(marble);
		area.getBlock(5, 5, 1).setSolid(marble);
		area.getBlock(7, 5, 1).setSolid(marble);
		area.getBlock(9, 5, 1).setSolid(marble);
		Actor& actor = simulation.createActor(troll, origin);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("walking path blocked by elevation")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 9, marble);
		Block& origin = area.getBlock(1, 1, 1);
		Block& destination = area.getBlock(8, 8, 8);		
		Block& ledge = area.getBlock(8, 8, 7);
		ledge.setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("flying path")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 9, marble);
		Block& origin = area.getBlock(1, 1, 1);
		Block& destination = area.getBlock(8, 8, 8);		
		Actor& actor = simulation.createActor(eagle, origin);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 7);
	}
	SUBCASE("swimming path")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 9, marble);
		Block& water1 = area.getBlock(1, 1, 1);
		Block& water2 = area.getBlock(8, 8, 8);		
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		Actor& actor = simulation.createActor(carp, water1);
		actor.m_canMove.setDestination(water2);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 7);
	}
}
TEST_CASE("route_5_5_3")
{
	Simulation simulation;
	Area& area = simulation.createArea(5,5,3);
	static const MaterialType& marble = MaterialType::byName("marble");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const AnimalSpecies& carp = AnimalSpecies::byName("carp");
	static const FluidType& water = FluidType::byName("water");
	static const MoveType& twoLegsAndSwimInWater = MoveType::byName("two legs and swim in water");
	static const MoveType& twoLegs = MoveType::byName("two legs");
	SUBCASE("swimming path blocked")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 1, marble);
		Block& origin = area.getBlock(2, 1, 1);
		Block& destination = area.getBlock(2, 3, 1);		
		area.getBlock(1, 2, 1).setSolid(marble);
		area.getBlock(2, 2, 1).setSolid(marble);
		area.getBlock(3, 2, 1).setSolid(marble);
		Block& water1 = area.getBlock(1, 1, 1);
		Block& water2 = area.getBlock(3, 1, 1);
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		Block& water3 = area.getBlock(1, 3, 1);
		Block& water4 = area.getBlock(3, 3, 1);
		areaBuilderUtil::setFullFluidCuboid(water3, water4, water);
		Actor& actor = simulation.createActor(carp, origin);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("walking path blocked by water if not also swimming")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		areaBuilderUtil::setSolidWalls(area, 2, marble);
		Block& origin = area.getBlock(2, 1, 2);
		Block& destination = area.getBlock(2, 3, 2);		
		area.getBlock(1, 2, 1).setNotSolid();
		area.getBlock(2, 2, 1).setNotSolid();
		area.getBlock(3, 2, 1).setNotSolid();
		Block& water1 = area.getBlock(1, 2, 1);
		Block& water2 = area.getBlock(3, 2, 1);
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setMoveType(twoLegs);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		actor.m_canMove.setMoveType(twoLegsAndSwimInWater);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
}
TEST_CASE("route_5_5_5")
{
	Simulation simulation;
	Area& area = simulation.createArea(5,5,5);
	static const MaterialType& marble = MaterialType::byName("marble");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const MoveType& twoLegsAndClimb1 = MoveType::byName("two legs and climb 1");
	static const MoveType& twoLegsAndClimb2 = MoveType::byName("two legs and climb 2");
	static const BlockFeatureType& stairs = BlockFeatureType::stairs;
	static const BlockFeatureType& ramp = BlockFeatureType::ramp;
	static const BlockFeatureType& door = BlockFeatureType::door;
	static const BlockFeatureType& fortification = BlockFeatureType::fortification;
	SUBCASE("walking path blocked by one height cliff if not climbing")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 1));
		area.getBlock(4, 4, 1).setSolid(marble);
		actor.m_canMove.setDestination(area.getBlock(4, 4, 2));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		actor.m_canMove.setMoveType(twoLegsAndClimb1);
		actor.m_canMove.setDestination(area.getBlock(4, 4, 2));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());

	}
	SUBCASE("walking path blocked by two height cliff if not climbing 2")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 1));
		actor.m_canMove.setMoveType(twoLegsAndClimb1);
		area.getBlock(4, 4, 1).setSolid(marble);
		area.getBlock(4, 4, 2).setSolid(marble);
		actor.m_canMove.setDestination(area.getBlock(4, 4, 3));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		actor.m_canMove.setMoveType(twoLegsAndClimb2);
		actor.m_canMove.setDestination(area.getBlock(4, 4, 3));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());

	}
	SUBCASE("stairs")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(1, 1, 1);
		area.getBlock(2, 2, 1).m_hasBlockFeatures.construct(stairs, marble);
		area.getBlock(3, 3, 2).m_hasBlockFeatures.construct(stairs, marble);
		area.getBlock(2, 2, 3).m_hasBlockFeatures.construct(stairs, marble);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(area.getBlock(2, 2, 4));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("ramp")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 1));
		area.getBlock(4, 4, 1).setSolid(marble);
		actor.m_canMove.setDestination(area.getBlock(4, 4, 2));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(4, 3, 1).m_hasBlockFeatures.construct(ramp, marble);
		actor.m_canMove.setDestination(area.getBlock(4, 4, 2));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("door")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.getBlock(3, 0, 1).setSolid(marble);
		area.getBlock(3, 1, 1).setSolid(marble);
		area.getBlock(3, 3, 1).setSolid(marble);
		area.getBlock(3, 4, 1).setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 1));
		actor.m_canMove.setDestination(area.getBlock(4, 3, 1));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(3, 2, 1).m_hasBlockFeatures.construct(door, marble);
		actor.m_canMove.setDestination(area.getBlock(4, 3, 1));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
		area.getBlock(3, 2, 1).m_hasBlockFeatures.at(door)->locked = true;
		actor.m_canMove.setDestination(area.getBlock(4, 3, 1));
		PathThreadedTask& pathThreadedTask3 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask3.readStep();
		REQUIRE(pathThreadedTask3.getFindsPath().getPath().empty());
	}
	SUBCASE("fortification")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.getBlock(3, 0, 1).setSolid(marble);
		area.getBlock(3, 1, 1).setSolid(marble);
		area.getBlock(3, 3, 1).setSolid(marble);
		area.getBlock(3, 4, 1).setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 1));
		actor.m_canMove.setDestination(area.getBlock(4, 3, 1));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(3, 2, 1).m_hasBlockFeatures.construct(fortification, marble);
		actor.m_canMove.setDestination(area.getBlock(4, 3, 1));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("flood gate blocks entry")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.getBlock(3, 0, 1).setSolid(marble);
		area.getBlock(3, 1, 1).setSolid(marble);
		area.getBlock(3, 3, 1).setSolid(marble);
		area.getBlock(3, 4, 1).setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 1));
		actor.m_canMove.setDestination(area.getBlock(4, 3, 1));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(3, 2, 1).m_hasBlockFeatures.construct(BlockFeatureType::floodGate, marble);
		actor.m_canMove.setDestination(area.getBlock(4, 3, 1));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("can walk on floor")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.getBlock(1, 1, 1).setSolid(marble);
		area.getBlock(1, 3, 1).setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 2));
		actor.m_canMove.setDestination(area.getBlock(1, 3, 2));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(1, 2, 2).m_hasBlockFeatures.construct(BlockFeatureType::floor, marble);
		actor.m_canMove.setDestination(area.getBlock(1, 3, 2));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("floor blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.getBlock(3, 3, 1).setNotSolid();
		area.getBlock(3, 3, 2).setNotSolid();
		area.getBlock(3, 3, 3).setNotSolid();
		area.getBlock(3, 3, 1).m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		area.getBlock(3, 3, 2).m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(3, 3, 1));
		actor.m_canMove.setDestination(area.getBlock(3, 3, 3));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(3, 3, 3).m_hasBlockFeatures.construct(BlockFeatureType::floor, marble);
		actor.m_canMove.setDestination(area.getBlock(3, 3, 3));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("can walk on floor grate")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.getBlock(1, 1, 1).setSolid(marble);
		area.getBlock(1, 3, 1).setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 2));
		actor.m_canMove.setDestination(area.getBlock(1, 3, 2));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(1, 2, 2).m_hasBlockFeatures.construct(BlockFeatureType::floorGrate, marble);
		actor.m_canMove.setDestination(area.getBlock(1, 3, 2));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("floor grate blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.getBlock(3, 3, 1).setNotSolid();
		area.getBlock(3, 3, 2).setNotSolid();
		area.getBlock(3, 3, 3).setNotSolid();
		area.getBlock(3, 3, 1).m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		area.getBlock(3, 3, 2).m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(3, 3, 1));
		actor.m_canMove.setDestination(area.getBlock(3, 3, 3));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(3, 3, 3).m_hasBlockFeatures.construct(BlockFeatureType::floorGrate, marble);
		actor.m_canMove.setDestination(area.getBlock(3, 3, 3));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("can walk on hatch")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.getBlock(1, 1, 1).setSolid(marble);
		area.getBlock(1, 3, 1).setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 2));
		actor.m_canMove.setDestination(area.getBlock(1, 3, 2));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(1, 2, 2).m_hasBlockFeatures.construct(BlockFeatureType::hatch, marble);
		actor.m_canMove.setDestination(area.getBlock(1, 3, 2));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("locked hatch blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.getBlock(3, 3, 1).setNotSolid();
		area.getBlock(3, 3, 2).setNotSolid();
		area.getBlock(3, 3, 3).setNotSolid();
		area.getBlock(3, 3, 1).m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		area.getBlock(3, 3, 2).m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(3, 3, 1));
		actor.m_canMove.setDestination(area.getBlock(3, 3, 3));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(3, 3, 3).m_hasBlockFeatures.construct(BlockFeatureType::hatch, marble);
		actor.m_canMove.setDestination(area.getBlock(3, 3, 3));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
		area.getBlock(3, 3, 3).m_hasBlockFeatures.at(BlockFeatureType::hatch)->locked = true;
		actor.m_canMove.setDestination(area.getBlock(3, 3, 3));
		PathThreadedTask& pathThreadedTask3 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask3.readStep();
		REQUIRE(pathThreadedTask3.getFindsPath().getPath().empty());
	}
	SUBCASE("multi-block actors can use ramps")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.getBlock(4, 4, 1).setSolid(marble);
		area.getBlock(4, 3, 1).setSolid(marble);
		area.getBlock(3, 4, 1).setSolid(marble);
		area.getBlock(3, 3, 1).setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 1));
		actor.m_canMove.setDestination(area.getBlock(3, 3, 2));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		area.getBlock(3, 2, 1).m_hasBlockFeatures.construct(BlockFeatureType::ramp, marble);
		area.getBlock(4, 2, 1).m_hasBlockFeatures.construct(BlockFeatureType::ramp, marble);
		area.getBlock(3, 1, 1).m_hasBlockFeatures.construct(BlockFeatureType::ramp, marble);
		area.getBlock(4, 1, 1).m_hasBlockFeatures.construct(BlockFeatureType::ramp, marble);
		actor.m_canMove.setDestination(area.getBlock(3, 3, 2));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("detour")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.getBlock(2, 3, 1);
		area.getBlock(3, 1, 1).setSolid(marble);
		area.getBlock(3, 2, 1).setSolid(marble);
		area.getBlock(3, 4, 1).setSolid(marble);
		Actor& a1 = simulation.createActor(dwarf, origin);
		simulation.createActor(dwarf, area.getBlock(3, 3, 1));
		a1.m_canMove.setDestination(area.getBlock(4, 3, 1));
		PathThreadedTask& pathThreadedTask = a1.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 2);
		pathThreadedTask.writeStep();
		REQUIRE(a1.m_canMove.getPath().size() == 2);
		//pathThreadedTask.clearReferences();
		//simulation.m_threadedTaskEngine.remove(pathThreadedTask);
		REQUIRE(area.getBlock(3, 3, 1).m_hasShapes.canEnterEverFrom(a1, *a1.m_location));
		REQUIRE(!area.getBlock(3, 3, 1).m_hasShapes.canEnterCurrentlyFrom(a1, *a1.m_location));
		REQUIRE(a1.m_canMove.hasEvent());
		// Move attempt 1.
		simulation.m_step = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_eventSchedule.execute(simulation.m_step);
		REQUIRE(a1.m_location == &origin);
		REQUIRE(a1.m_canMove.hasEvent());
		// Move attempt 2.
		simulation.m_step = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_eventSchedule.execute(simulation.m_step);
		// Move attempt 3.
		simulation.m_step = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_eventSchedule.execute(simulation.m_step);
		REQUIRE(a1.m_location == &origin);
		REQUIRE(simulation.m_threadedTaskEngine.count() == 1);
		REQUIRE(!a1.m_canMove.hasEvent());
		// Detour.
		PathThreadedTask& detour = a1.m_canMove.getPathThreadedTask();
		REQUIRE(detour.isDetour());
		detour.readStep();
		REQUIRE(detour.getFindsPath().getPath().size() == 6);
	}
}
