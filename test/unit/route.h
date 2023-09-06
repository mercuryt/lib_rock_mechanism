#include "../../lib/doctest.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/actor.h"
#include "../../src/materialType.h"
#include "../../src/simulation.h"
TEST_CASE("route 10,10,10")
{
	Simulation simulation;
	static const MaterialType& marble = MaterialType::byName("marble");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const AnimalSpecies& troll = AnimalSpecies::byName("troll");
	static const AnimalSpecies& eagle = AnimalSpecies::byName("eagle");
	static const AnimalSpecies& carp = AnimalSpecies::byName("carp");
	static const FluidType& water = FluidType::byName("water");
	Area& area = simulation.createArea(10,10,10);
	SUBCASE("Route through open space")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.m_blocks[3][3][1];
		Block& destination = area.m_blocks[7][7][1];
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
		Block& origin = area.m_blocks[3][3][1];
		Block& block1 = area.m_blocks[5][3][1];
		Block& block2 = area.m_blocks[5][4][1];
		Block& block3 = area.m_blocks[5][5][1];
		Block& block4 = area.m_blocks[5][6][1];
		Block& block5 = area.m_blocks[5][7][1];
		Block& destination = area.m_blocks[7][7][1];
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
		Block& origin = area.m_blocks[0][0][1];
		Block& block1 = area.m_blocks[2][0][1];
		Block& block2 = area.m_blocks[2][1][1];
		Block& block3 = area.m_blocks[2][2][1];
		Block& block4 = area.m_blocks[1][2][1];
		Block& block5 = area.m_blocks[0][2][1];
		Block& destination = area.m_blocks[7][7][1];
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
		Block& origin = area.m_blocks[3][3][1];
		Block& block1 = area.m_blocks[4][4][1];
		Block& destination = area.m_blocks[5][5][1];
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
		Block& origin = area.m_blocks[3][3][1];
		Block& block1 = area.m_blocks[3][4][1];
		Block& block2 = area.m_blocks[3][5][1];
		Block& block3 = area.m_blocks[4][5][1];
		Block& destination = area.m_blocks[3][6][1];
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		pathThreadedTask.writeStep();
		pathThreadedTask.clearReferences();
		simulation.m_threadedTaskEngine.remove(pathThreadedTask);
		REQUIRE(actor.m_canMove.hasEvent());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.empty());
		// Step 1.
		uint32_t scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == &block1);
		REQUIRE(actor.m_canMove.hasEvent());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.empty());
		block2.setSolid(marble);
		// Step 2.
		REQUIRE(actor.m_canMove.hasEvent());
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == &block1);
		REQUIRE(!actor.m_canMove.hasEvent());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.size() == 1);
		// Step 3.
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		pathThreadedTask2.writeStep();
		pathThreadedTask2.clearReferences();
		simulation.m_threadedTaskEngine.remove(pathThreadedTask2);
		REQUIRE(actor.m_canMove.hasEvent());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.empty());
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == &block3);
	}
	SUBCASE("Walk multi-block creature")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.m_blocks[3][3][1];
		Block& block1 = area.m_blocks[4][4][1];
		Block& block2 = area.m_blocks[2][3][1];
		Block& destination = area.m_blocks[5][5][1];
		Actor& actor = simulation.createActor(troll, origin);
		actor.m_canMove.setDestination(destination);
		REQUIRE(block2.m_hasShapes.contains(actor));
		REQUIRE(!block1.m_hasShapes.contains(actor));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 2);
		pathThreadedTask.writeStep();
		REQUIRE(actor.m_canMove.hasEvent());
		// Step 1
		uint32_t scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == &block1);
		REQUIRE(actor.m_canMove.hasEvent());
		REQUIRE(!block2.m_hasShapes.contains(actor));
		REQUIRE(block1.m_hasShapes.contains(actor));
		REQUIRE(!origin.m_hasShapes.contains(actor));
		// Step 2
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == &destination);
		REQUIRE(!actor.m_canMove.hasEvent());
		REQUIRE(actor.m_canMove.getDestination() == nullptr);
		REQUIRE(!block2.m_hasShapes.contains(actor));
		REQUIRE(!block1.m_hasShapes.contains(actor));
	}
	SUBCASE("two by two creature cannot path through one block gap")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.m_blocks[2][2][1];
		Block& destination = area.m_blocks[8][8][1];
		area.m_blocks[1][5][1].setSolid(marble);
		area.m_blocks[3][5][1].setSolid(marble);
		area.m_blocks[5][5][1].setSolid(marble);
		area.m_blocks[7][5][1].setSolid(marble);
		area.m_blocks[9][5][1].setSolid(marble);
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
		Block& origin = area.m_blocks[1][1][1];
		Block& destination = area.m_blocks[8][8][8];		
		Block& ledge = area.m_blocks[8][8][7];
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
		Block& origin = area.m_blocks[1][1][1];
		Block& destination = area.m_blocks[8][8][8];		
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
		Block& water1 = area.m_blocks[1][1][1];
		Block& water2 = area.m_blocks[8][8][8];		
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		Actor& actor = simulation.createActor(carp, water1);
		actor.m_canMove.setDestination(water2);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 7);
	}
}
TEST_CASE("route 5,5,3")
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
		Block& origin = area.m_blocks[2][1][1];
		Block& destination = area.m_blocks[2][3][1];		
		area.m_blocks[1][2][1].setSolid(marble);
		area.m_blocks[2][2][1].setSolid(marble);
		area.m_blocks[3][2][1].setSolid(marble);
		Block& water1 = area.m_blocks[1][1][1];
		Block& water2 = area.m_blocks[3][1][1];
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		Block& water3 = area.m_blocks[1][3][1];
		Block& water4 = area.m_blocks[3][3][1];
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
		Block& origin = area.m_blocks[2][1][2];
		Block& destination = area.m_blocks[2][3][2];		
		area.m_blocks[1][2][1].setNotSolid();
		area.m_blocks[2][2][1].setNotSolid();
		area.m_blocks[3][2][1].setNotSolid();
		Block& water1 = area.m_blocks[1][2][1];
		Block& water2 = area.m_blocks[3][2][1];
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setMoveType(twoLegs);
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		actor.m_canMove.setMoveType(twoLegsAndSwimInWater);
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
	}
}
TEST_CASE("route 5,5,5")
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
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][1]);
		area.m_blocks[4][4][1].setSolid(marble);
		actor.m_canMove.setDestination(area.m_blocks[4][4][2]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		actor.m_canMove.setMoveType(twoLegsAndClimb1);
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());

	}
	SUBCASE("walking path blocked by two height cliff if not climbing 2")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][1]);
		actor.m_canMove.setMoveType(twoLegsAndClimb1);
		area.m_blocks[4][4][1].setSolid(marble);
		area.m_blocks[4][4][2].setSolid(marble);
		actor.m_canMove.setDestination(area.m_blocks[4][4][3]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		actor.m_canMove.setMoveType(twoLegsAndClimb2);
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());

	}
	SUBCASE("stairs")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.m_blocks[1][1][1];
		area.m_blocks[2][2][1].m_hasBlockFeatures.construct(stairs, marble);
		area.m_blocks[3][3][2].m_hasBlockFeatures.construct(stairs, marble);
		area.m_blocks[2][2][3].m_hasBlockFeatures.construct(stairs, marble);
		Actor& actor = simulation.createActor(dwarf, origin);
		actor.m_canMove.setDestination(area.m_blocks[2][2][4]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("ramp")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][1]);
		area.m_blocks[4][4][1].setSolid(marble);
		actor.m_canMove.setDestination(area.m_blocks[4][4][2]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[4][3][1].m_hasBlockFeatures.construct(ramp, marble);
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("door")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.m_blocks[3][0][1].setSolid(marble);
		area.m_blocks[3][1][1].setSolid(marble);
		area.m_blocks[3][3][1].setSolid(marble);
		area.m_blocks[3][4][1].setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][1]);
		actor.m_canMove.setDestination(area.m_blocks[4][3][1]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[3][2][1].m_hasBlockFeatures.construct(door, marble);
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[3][2][1].m_hasBlockFeatures.at(door)->locked = true;
		pathThreadedTask.getFindsPath().getPath().clear();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("fortification")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.m_blocks[3][0][1].setSolid(marble);
		area.m_blocks[3][1][1].setSolid(marble);
		area.m_blocks[3][3][1].setSolid(marble);
		area.m_blocks[3][4][1].setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][1]);
		actor.m_canMove.setDestination(area.m_blocks[4][3][1]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[3][2][1].m_hasBlockFeatures.construct(fortification, marble);
		pathThreadedTask.getFindsPath().getPath().clear();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("flood gate blocks entry")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.m_blocks[3][0][1].setSolid(marble);
		area.m_blocks[3][1][1].setSolid(marble);
		area.m_blocks[3][3][1].setSolid(marble);
		area.m_blocks[3][4][1].setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][1]);
		actor.m_canMove.setDestination(area.m_blocks[4][3][1]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[3][2][1].m_hasBlockFeatures.construct(BlockFeatureType::floodGate, marble);
		pathThreadedTask.getFindsPath().getPath().clear();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("can walk on floor")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.m_blocks[1][1][1].setSolid(marble);
		area.m_blocks[1][3][1].setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][2]);
		actor.m_canMove.setDestination(area.m_blocks[1][3][2]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[1][2][2].m_hasBlockFeatures.construct(BlockFeatureType::floor, marble);
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("floor blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.m_blocks[3][3][1].setNotSolid();
		area.m_blocks[3][3][2].setNotSolid();
		area.m_blocks[3][3][3].setNotSolid();
		area.m_blocks[3][3][1].m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		area.m_blocks[3][3][2].m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[3][3][1]);
		actor.m_canMove.setDestination(area.m_blocks[3][3][3]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[3][3][3].m_hasBlockFeatures.construct(BlockFeatureType::floor, marble);
		pathThreadedTask.getFindsPath().getPath().clear();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("can walk on floor grate")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.m_blocks[1][1][1].setSolid(marble);
		area.m_blocks[1][3][1].setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][2]);
		actor.m_canMove.setDestination(area.m_blocks[1][3][2]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[1][2][2].m_hasBlockFeatures.construct(BlockFeatureType::floorGrate, marble);
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("floor grate blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.m_blocks[3][3][1].setNotSolid();
		area.m_blocks[3][3][2].setNotSolid();
		area.m_blocks[3][3][3].setNotSolid();
		area.m_blocks[3][3][1].m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		area.m_blocks[3][3][2].m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[3][3][1]);
		actor.m_canMove.setDestination(area.m_blocks[3][3][3]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[3][3][3].m_hasBlockFeatures.construct(BlockFeatureType::floorGrate, marble);
		pathThreadedTask.getFindsPath().getPath().clear();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("can walk on hatch")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.m_blocks[1][1][1].setSolid(marble);
		area.m_blocks[1][3][1].setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][2]);
		actor.m_canMove.setDestination(area.m_blocks[1][3][2]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[1][2][2].m_hasBlockFeatures.construct(BlockFeatureType::hatch, marble);
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("locked hatch blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.m_blocks[3][3][1].setNotSolid();
		area.m_blocks[3][3][2].setNotSolid();
		area.m_blocks[3][3][3].setNotSolid();
		area.m_blocks[3][3][1].m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		area.m_blocks[3][3][2].m_hasBlockFeatures.construct(BlockFeatureType::stairs, marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[3][3][1]);
		actor.m_canMove.setDestination(area.m_blocks[3][3][3]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[3][3][3].m_hasBlockFeatures.construct(BlockFeatureType::hatch, marble);
		pathThreadedTask.getFindsPath().getPath().clear();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[3][3][3].m_hasBlockFeatures.at(BlockFeatureType::hatch)->locked = true;
		pathThreadedTask.getFindsPath().getPath().clear();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("multi-block actors can use ramps")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.m_blocks[4][4][1].setSolid(marble);
		area.m_blocks[4][3][1].setSolid(marble);
		area.m_blocks[3][4][1].setSolid(marble);
		area.m_blocks[3][3][1].setSolid(marble);
		Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][1]);
		actor.m_canMove.setDestination(area.m_blocks[3][3][2]);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		area.m_blocks[3][2][1].m_hasBlockFeatures.construct(BlockFeatureType::ramp, marble);
		area.m_blocks[4][2][1].m_hasBlockFeatures.construct(BlockFeatureType::ramp, marble);
		area.m_blocks[3][1][1].m_hasBlockFeatures.construct(BlockFeatureType::ramp, marble);
		area.m_blocks[4][1][1].m_hasBlockFeatures.construct(BlockFeatureType::ramp, marble);
		pathThreadedTask.getFindsPath().getPath().clear();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("detour")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin = area.m_blocks[2][3][1];
		area.m_blocks[3][1][1].setSolid(marble);
		area.m_blocks[3][2][1].setSolid(marble);
		area.m_blocks[3][4][1].setSolid(marble);
		Actor& a1 = simulation.createActor(dwarf, origin);
		simulation.createActor(dwarf, area.m_blocks[3][3][1]);
		a1.m_canMove.setDestination(area.m_blocks[4][3][1]);
		PathThreadedTask& pathThreadedTask = a1.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 2);
		pathThreadedTask.writeStep();
		REQUIRE(a1.m_canMove.getPath().size() == 2);
		pathThreadedTask.clearReferences();
		simulation.m_threadedTaskEngine.remove(pathThreadedTask);
		REQUIRE(area.m_blocks[3][3][1].m_hasShapes.canEnterEverFrom(a1, *a1.m_location));
		REQUIRE(!area.m_blocks[3][3][1].m_hasShapes.canEnterCurrentlyFrom(a1, *a1.m_location));
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
