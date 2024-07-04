#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/materialType.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "config.h"
#include "types.h"
TEST_CASE("route_10_10_10")
{
	Simulation simulation(L"", 1);
	static const MaterialType& marble = MaterialType::byName("marble");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const AnimalSpecies& troll = AnimalSpecies::byName("troll");
	static const AnimalSpecies& eagle = AnimalSpecies::byName("golden eagle");
	static const AnimalSpecies& carp = AnimalSpecies::byName("carp");
	static const FluidType& water = FluidType::byName("water");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	SUBCASE("Route through open space")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex({3, 3, 1});
		BlockIndex destination = blocks.getIndex({7, 7, 1});
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 4);
		pathThreadedTask.writeStep();
		REQUIRE(actor.m_canMove.getPath().size() == 4);
	}
	SUBCASE("Route around walls")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex({3, 3, 1});
		BlockIndex block1 = blocks.getIndex({5, 3, 1});
		BlockIndex block2 = blocks.getIndex({5, 4, 1});
		BlockIndex block3 = blocks.getIndex({5, 5, 1});
		BlockIndex block4 = blocks.getIndex({5, 6, 1});
		BlockIndex block5 = blocks.getIndex({5, 7, 1});
		BlockIndex destination = blocks.getIndex({7, 7, 1});
		blocks.solid_set(block1, marble, false);
		blocks.solid_set(block2, marble, false);
		blocks.solid_set(block3, marble, false);
		blocks.solid_set(block4, marble, false);
		blocks.solid_set(block5, marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 7);
		pathThreadedTask.writeStep();
		REQUIRE(actor.m_canMove.getPath().size() == 7);
	}
	SUBCASE("No route found")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex({0, 0, 1});
		BlockIndex block1 = blocks.getIndex({2, 0, 1});
		BlockIndex block2 = blocks.getIndex({2, 1, 1});
		BlockIndex block3 = blocks.getIndex({2, 2, 1});
		BlockIndex block4 = blocks.getIndex({1, 2, 1});
		BlockIndex block5 = blocks.getIndex({0, 2, 1});
		BlockIndex destination = blocks.getIndex({7, 7, 1});
		blocks.solid_set(block1, marble, false);
		blocks.solid_set(block2, marble, false);
		blocks.solid_set(block3, marble, false);
		blocks.solid_set(block4, marble, false);
		blocks.solid_set(block5, marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
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
		BlockIndex origin = blocks.getIndex({3, 3, 1});
		BlockIndex block1 = blocks.getIndex({4, 4, 1});
		BlockIndex destination = blocks.getIndex({5, 5, 1});
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
		actor.m_canMove.setDestination(destination);
		REQUIRE(!actor.m_canMove.hasEvent());
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		pathThreadedTask.writeStep();
		REQUIRE(actor.m_canMove.hasEvent());
		Step scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		float seconds = (float)scheduledStep / (float)Config::stepsPerSecond;
		REQUIRE(seconds >= 0.6f);
		REQUIRE(seconds <= 0.9f);
		simulation.m_step = scheduledStep;
		// step 1
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == block1);
		REQUIRE(actor.m_canMove.hasEvent());
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		REQUIRE(actor.m_canMove.hasEvent());
		// step 2
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == destination);
		REQUIRE(!actor.m_canMove.hasEvent());
		REQUIRE(actor.m_canMove.getPath().empty());
		REQUIRE(actor.m_canMove.getDestination() == BLOCK_INDEX_MAX);
	}
	SUBCASE("Repath when route is blocked")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex({3, 3, 1});
		BlockIndex block1 = blocks.getIndex({3, 4, 1});
		BlockIndex block2 = blocks.getIndex({3, 5, 1});
		BlockIndex block3 = blocks.getIndex({4, 5, 1});
		BlockIndex destination = blocks.getIndex({3, 6, 1});
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
		// Set solid to make choice of route deterministic.
		blocks.solid_set(blocks.getIndex(2,5,1), marble, false);
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
		REQUIRE(actor.m_location == block1);
		REQUIRE(actor.m_canMove.hasEvent());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		blocks.solid_set(block2, marble, false);
		// Step 2.
		REQUIRE(actor.m_canMove.hasEvent());
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.execute(scheduledStep);
		REQUIRE(actor.m_location == block1);
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
		REQUIRE(actor.m_location == block3);
	}
	SUBCASE("Unpathable route becomes pathable when blockage is removed.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex({3, 3, 1});
		BlockIndex wallStart = blocks.getIndex({0, 4, 1});
		BlockIndex wallEnd = blocks.getIndex({9, 4, 2});
		BlockIndex destination = blocks.getIndex({3, 6, 1});
		areaBuilderUtil::setSolidWall(area, wallStart, wallEnd, marble);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
		actor.m_canMove.setDestination(destination);
		simulation.doStep();
		REQUIRE(actor.m_canMove.getPath().empty());
		blocks.solid_setNot(wallStart);
		actor.m_canMove.setDestination(destination);
		simulation.doStep();
		REQUIRE(!actor.m_canMove.getPath().empty());
	}
	SUBCASE("two by two creature cannot path through one block gap")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex({2, 2, 1});
		BlockIndex destination = blocks.getIndex({8, 8, 1});
		blocks.solid_set(blocks.getIndex({1, 5, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 5, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({5, 5, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({7, 5, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({9, 5, 1}), marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=troll,
			.location=origin,
			.area=&area,
		});
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("walking path blocked by elevation")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 9, marble);
		BlockIndex origin = blocks.getIndex({1, 1, 1});
		BlockIndex destination = blocks.getIndex({8, 8, 8});		
		BlockIndex ledge = blocks.getIndex({8, 8, 7});
		blocks.solid_set(ledge, marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("flying path")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 9, marble);
		BlockIndex origin = blocks.getIndex({1, 1, 1});
		BlockIndex destination = blocks.getIndex({8, 8, 8});		
		Actor& actor = simulation.m_hasActors->createActor({
			.species=eagle,
			.location=origin,
			.area=&area,
		});
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 7);
	}
	SUBCASE("swimming path")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 9, marble);
		BlockIndex water1 = blocks.getIndex({1, 1, 1});
		BlockIndex water2 = blocks.getIndex({8, 8, 8});		
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=carp,
			.location=water1,
			.area=&area,
		});
		actor.m_canMove.setDestination(water2);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 7);
	}
}
TEST_CASE("route_5_5_3")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(5,5,3);
	Blocks& blocks = area.getBlocks();
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
		BlockIndex origin = blocks.getIndex({2, 1, 1});
		BlockIndex destination = blocks.getIndex({2, 3, 1});		
		blocks.solid_set(blocks.getIndex({1, 2, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({2, 2, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 2, 1}), marble, false);
		BlockIndex water1 = blocks.getIndex({1, 1, 1});
		BlockIndex water2 = blocks.getIndex({3, 1, 1});
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		BlockIndex water3 = blocks.getIndex({1, 3, 1});
		BlockIndex water4 = blocks.getIndex({3, 3, 1});
		areaBuilderUtil::setFullFluidCuboid(area, water3, water4, water);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=carp,
			.location=origin,
			.area=&area,
		});
		actor.m_canMove.setDestination(destination);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("walking path blocked by water if not also swimming")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		areaBuilderUtil::setSolidWalls(area, 2, marble);
		BlockIndex origin = blocks.getIndex({2, 1, 2});
		BlockIndex destination = blocks.getIndex({2, 3, 2});		
		BlockIndex midpoint = blocks.getIndex(2, 2, 1);
		blocks.solid_setNot(blocks.getIndex({1, 2, 1}));
		blocks.solid_setNot(midpoint);
		blocks.solid_setNot(blocks.getIndex({3, 2, 1}));
		BlockIndex water1 = blocks.getIndex({1, 2, 1});
		BlockIndex water2 = blocks.getIndex({3, 2, 1});
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
		actor.m_canMove.setMoveType(twoLegs);
		actor.m_canMove.setDestination(destination);
		REQUIRE(!blocks.shape_moveTypeCanEnter(midpoint, actor.m_canMove.getMoveType()));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		actor.m_canMove.setMoveType(twoLegsAndSwimInWater);
		actor.m_canMove.setDestination(destination);
		REQUIRE(blocks.shape_moveTypeCanEnter(midpoint, actor.m_canMove.getMoveType()));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
}
TEST_CASE("route_5_5_5")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(5,5,5);
	Blocks& blocks = area.getBlocks();
	static const MaterialType& marble = MaterialType::byName("marble");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const MoveType& twoLegsAndClimb1 = MoveType::byName("two legs and climb 1");
	static const MoveType& twoLegsAndClimb2 = MoveType::byName("two legs and climb 2");
	static const BlockFeatureType& stairs = BlockFeatureType::stairs;
	static const BlockFeatureType& ramp = BlockFeatureType::ramp;
	static const BlockFeatureType& door = BlockFeatureType::door;
	static const BlockFeatureType& fortification = BlockFeatureType::fortification;
	static const FluidType& water = FluidType::byName("water");
	SUBCASE("walking path blocked by one height cliff if not climbing")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,1),
			.area=&area,
		});
		blocks.solid_set(blocks.getIndex(4, 4, 1), marble, false);
		actor.m_canMove.setDestination(blocks.getIndex(4, 4, 2));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		actor.m_canMove.setMoveType(twoLegsAndClimb1);
		actor.m_canMove.setDestination(blocks.getIndex(4, 4, 2));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());

	}
	SUBCASE("walking path blocked by two height cliff if not climbing 2")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,1),
			.area=&area,
		});
		actor.m_canMove.setMoveType(twoLegsAndClimb1);
		blocks.solid_set(blocks.getIndex({4, 4, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({4, 4, 2}), marble, false);
		actor.m_canMove.setDestination(blocks.getIndex({4, 4, 3}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		actor.m_canMove.setMoveType(twoLegsAndClimb2);
		actor.m_canMove.setDestination(blocks.getIndex({4, 4, 3}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());

	}
	SUBCASE("stairs")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex({1, 1, 1});
		blocks.blockFeature_construct(blocks.getIndex({2, 2, 1}), stairs, marble);
		blocks.blockFeature_construct(blocks.getIndex({3, 3, 2}), stairs, marble);
		blocks.blockFeature_construct(blocks.getIndex({2, 2, 3}), stairs, marble);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({2, 2, 4}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
	}
	SUBCASE("ramp")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,1),
			.area=&area,
		});
		blocks.solid_set(blocks.getIndex({4, 4, 1}), marble, false);
		actor.m_canMove.setDestination(blocks.getIndex({4, 4, 2}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({4, 3, 1}), ramp, marble);
		actor.m_canMove.setDestination(blocks.getIndex({4, 4, 2}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("door")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex({3, 0, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 1, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 3, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 4, 1}), marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,1),
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({4, 3, 1}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({3, 2, 1}), door, marble);
		actor.m_canMove.setDestination(blocks.getIndex({4, 3, 1}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
		blocks.blockFeature_lock(blocks.getIndex({3, 2, 1}), door);
		actor.m_canMove.setDestination(blocks.getIndex({4, 3, 1}));
		PathThreadedTask& pathThreadedTask3 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask3.readStep();
		REQUIRE(pathThreadedTask3.getFindsPath().getPath().empty());
	}
	SUBCASE("fortification")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex({3, 0, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 1, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 3, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 4, 1}), marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,1),
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({4, 3, 1}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({3, 2, 1}), fortification, marble);
		actor.m_canMove.setDestination(blocks.getIndex({4, 3, 1}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("flood gate blocks entry")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex({3, 0, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 1, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 3, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 4, 1}), marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,1),
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({4, 3, 1}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({3, 2, 1}), BlockFeatureType::floodGate, marble);
		actor.m_canMove.setDestination(blocks.getIndex({4, 3, 1}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("can walk on floor")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex({1, 1, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({1, 3, 1}), marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,2),
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({1, 3, 2}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({1, 2, 2}), BlockFeatureType::floor, marble);
		actor.m_canMove.setDestination(blocks.getIndex({1, 3, 2}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("floor blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_setNot(blocks.getIndex({3, 3, 1}));
		blocks.solid_setNot(blocks.getIndex({3, 3, 2}));
		blocks.solid_setNot(blocks.getIndex({3, 3, 3}));
		blocks.blockFeature_construct(blocks.getIndex({3, 3, 1}), BlockFeatureType::stairs, marble);
		blocks.blockFeature_construct(blocks.getIndex({3, 3, 2}), BlockFeatureType::stairs, marble);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(3,3,1),
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({3, 3, 3}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({3, 3, 3}), BlockFeatureType::floor, marble);
		actor.m_canMove.setDestination(blocks.getIndex({3, 3, 3}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("can walk on floor grate")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex({1, 1, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({1, 3, 1}), marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,2),
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({1, 3, 2}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({1, 2, 2}), BlockFeatureType::floorGrate, marble);
		actor.m_canMove.setDestination(blocks.getIndex({1, 3, 2}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("floor grate blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_setNot(blocks.getIndex({3, 3, 1}));
		blocks.solid_setNot(blocks.getIndex({3, 3, 2}));
		blocks.solid_setNot(blocks.getIndex({3, 3, 3}));
		blocks.blockFeature_construct(blocks.getIndex({3, 3, 1}), BlockFeatureType::stairs, marble);
		blocks.blockFeature_construct(blocks.getIndex({3, 3, 2}), BlockFeatureType::stairs, marble);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(3,3,1),
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({3, 3, 3}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({3, 3, 3}), BlockFeatureType::floorGrate, marble);
		actor.m_canMove.setDestination(blocks.getIndex({3, 3, 3}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("can walk on hatch")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex({1, 1, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({1, 3, 1}), marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,2),
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({1, 3, 2}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({1, 2, 2}), BlockFeatureType::hatch, marble);
		actor.m_canMove.setDestination(blocks.getIndex({1, 3, 2}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("locked hatch blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_setNot(blocks.getIndex({3, 3, 1}));
		blocks.solid_setNot(blocks.getIndex({3, 3, 2}));
		blocks.solid_setNot(blocks.getIndex({3, 3, 3}));
		blocks.blockFeature_construct(blocks.getIndex({3, 3, 1}), BlockFeatureType::stairs, marble);
		blocks.blockFeature_construct(blocks.getIndex({3, 3, 2}), BlockFeatureType::stairs, marble);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(3,3,1),
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({3, 3, 3}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(!pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({3, 3, 3}), BlockFeatureType::hatch, marble);
		actor.m_canMove.setDestination(blocks.getIndex({3, 3, 3}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
		blocks.blockFeature_lock(blocks.getIndex({3, 3, 3}), BlockFeatureType::hatch);
		actor.m_canMove.setDestination(blocks.getIndex({3, 3, 3}));
		PathThreadedTask& pathThreadedTask3 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask3.readStep();
		REQUIRE(pathThreadedTask3.getFindsPath().getPath().empty());
	}
	SUBCASE("multi-block actors can use ramps")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex({4, 4, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({4, 3, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 4, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 3, 1}), marble, false);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,1),
			.area=&area,
		});
		actor.m_canMove.setDestination(blocks.getIndex({3, 3, 2}));
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
		blocks.blockFeature_construct(blocks.getIndex({3, 2, 1}), BlockFeatureType::ramp, marble);
		blocks.blockFeature_construct(blocks.getIndex({4, 2, 1}), BlockFeatureType::ramp, marble);
		blocks.blockFeature_construct(blocks.getIndex({3, 1, 1}), BlockFeatureType::ramp, marble);
		blocks.blockFeature_construct(blocks.getIndex({4, 1, 1}), BlockFeatureType::ramp, marble);
		actor.m_canMove.setDestination(blocks.getIndex({3, 3, 2}));
		PathThreadedTask& pathThreadedTask2 = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask2.readStep();
		REQUIRE(!pathThreadedTask2.getFindsPath().getPath().empty());
	}
	SUBCASE("detour")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex({2, 3, 1});
		blocks.solid_set(blocks.getIndex({3, 1, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 2, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({3, 4, 1}), marble, false);
		Actor& a1 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin,
			.area=&area,
		});
		simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(3,3,1),
			.area=&area,
		});
		a1.m_canMove.setDestination(blocks.getIndex({4, 3, 1}));
		PathThreadedTask& pathThreadedTask = a1.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().size() == 2);
		pathThreadedTask.writeStep();
		REQUIRE(a1.m_canMove.getPath().size() == 2);
		//pathThreadedTask.clearReferences();
		//simulation.m_threadedTaskEngine.remove(pathThreadedTask);
		REQUIRE(blocks.shape_canEnterEverFrom(blocks.getIndex({3, 3, 1}), a1, a1.m_location));
		REQUIRE(!blocks.shape_canEnterCurrentlyFrom(blocks.getIndex({3, 3, 1}), a1, a1.m_location));
		REQUIRE(a1.m_canMove.hasEvent());
		// Move attempt 1.
		simulation.m_step = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_eventSchedule.execute(simulation.m_step);
		REQUIRE(a1.m_location == origin);
		REQUIRE(a1.m_canMove.hasEvent());
		// Move attempt 2.
		simulation.m_step = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_eventSchedule.execute(simulation.m_step);
		// Move attempt 3.
		simulation.m_step = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_eventSchedule.execute(simulation.m_step);
		REQUIRE(a1.m_location == origin);
		REQUIRE(simulation.m_threadedTaskEngine.count() == 1);
		REQUIRE(!a1.m_canMove.hasEvent());
		// Detour.
		PathThreadedTask& detour = a1.m_canMove.getPathThreadedTask();
		REQUIRE(detour.isDetour());
		detour.readStep();
		REQUIRE(detour.getFindsPath().getPath().size() == 6);
	}
	SUBCASE("air breathers cannot dive")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		BlockIndex surface = blocks.getIndex({3,3,3});
		BlockIndex deep = blocks.getIndex({3,3,2});
		blocks.solid_setNot(surface);
		blocks.solid_setNot(deep);
		blocks.fluid_add(surface, Config::maxBlockVolume, water);
		blocks.fluid_add(deep, Config::maxBlockVolume, water);
		Actor& actor = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex(1,1,4),
			.area=&area,
		});
		actor.m_canMove.setDestination(deep);
		PathThreadedTask& pathThreadedTask = actor.m_canMove.getPathThreadedTask();
		pathThreadedTask.readStep();
		REQUIRE(pathThreadedTask.getFindsPath().getPath().empty());
	}
}
