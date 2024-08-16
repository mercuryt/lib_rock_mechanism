#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/plants.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/items/items.h"
#include "../../engine/materialType.h"
#include "../../engine/moveType.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
TEST_CASE("route_10_10_10")
{
	Simulation simulation(L"", Step::create(1));
	static MaterialTypeId marble = MaterialType::byName("marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	static AnimalSpeciesId troll = AnimalSpecies::byName("troll");
	static AnimalSpeciesId eagle = AnimalSpecies::byName("golden eagle");
	static AnimalSpeciesId carp = AnimalSpecies::byName("carp");
	static FluidTypeId water = FluidType::byName("water");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	SUBCASE("Route through open space")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(3, 3, 1);
		BlockIndex destination = blocks.getIndex_i(7, 7, 1);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).size() == 4);
	}
	SUBCASE("Route around walls")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(3, 3, 1);
		BlockIndex block1 = blocks.getIndex_i(5, 3, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 4, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block4 = blocks.getIndex_i(5, 6, 1);
		BlockIndex block5 = blocks.getIndex_i(5, 7, 1);
		BlockIndex destination = blocks.getIndex_i(7, 7, 1);
		blocks.solid_set(block1, marble, false);
		blocks.solid_set(block2, marble, false);
		blocks.solid_set(block3, marble, false);
		blocks.solid_set(block4, marble, false);
		blocks.solid_set(block5, marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).size() == 7);
	}
	SUBCASE("No route found")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(0, 0, 1);
		BlockIndex block1 = blocks.getIndex_i(2, 0, 1);
		BlockIndex block2 = blocks.getIndex_i(2, 1, 1);
		BlockIndex block3 = blocks.getIndex_i(2, 2, 1);
		BlockIndex block4 = blocks.getIndex_i(1, 2, 1);
		BlockIndex block5 = blocks.getIndex_i(0, 2, 1);
		BlockIndex destination = blocks.getIndex_i(7, 7, 1);
		blocks.solid_set(block1, marble, false);
		blocks.solid_set(block2, marble, false);
		blocks.solid_set(block3, marble, false);
		blocks.solid_set(block4, marble, false);
		blocks.solid_set(block5, marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
	}
	SUBCASE("Walk")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(3, 3, 1);
		BlockIndex block1 = blocks.getIndex_i(4, 4, 1);
		BlockIndex destination = blocks.getIndex_i(5, 5, 1);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		REQUIRE(!actors.move_hasEvent(actor));
		simulation.doStep();
		REQUIRE(actors.move_hasEvent(actor));
		Step scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		float seconds = (float)scheduledStep.get() / (float)Config::stepsPerSecond.get();
		REQUIRE(seconds >= 0.6f);
		REQUIRE(seconds <= 0.9f);
		simulation.m_step = scheduledStep;
		// step 1
		simulation.m_eventSchedule.doStep(scheduledStep);
		REQUIRE(actors.getLocation(actor) == block1);
		REQUIRE(actors.move_hasEvent(actor));
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		REQUIRE(actors.move_hasEvent(actor));
		// step 2
		simulation.m_eventSchedule.doStep(scheduledStep);
		REQUIRE(actors.getLocation(actor) == destination);
		REQUIRE(!actors.move_hasEvent(actor));
		REQUIRE(actors.move_getPath(actor).empty());
		REQUIRE(actors.move_getDestination(actor).empty());
	}
	SUBCASE("Repath when route is blocked")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(3, 3, 1);
		BlockIndex block1 = blocks.getIndex_i(3, 4, 1);
		BlockIndex block2 = blocks.getIndex_i(3, 5, 1);
		BlockIndex block3 = blocks.getIndex_i(4, 5, 1);
		BlockIndex destination = blocks.getIndex_i(3, 6, 1);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		// Set solid to make choice of route deterministic.
		blocks.solid_set(blocks.getIndex_i(2,5,1), marble, false);
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		REQUIRE(actors.move_hasEvent(actor));
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		// Step 1.
		Step scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.doStep(scheduledStep);
		REQUIRE(actors.getLocation(actor) == block1);
		REQUIRE(actors.move_hasEvent(actor));
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		blocks.solid_set(block2, marble, false);
		// Step 2.
		REQUIRE(actors.move_hasEvent(actor));
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.doStep(scheduledStep);
		REQUIRE(actors.getLocation(actor) == block1);
		REQUIRE(!actors.move_hasEvent(actor));
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 1);
		// Step 3.
		simulation.doStep();
		REQUIRE(actors.move_hasEvent(actor));
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		scheduledStep = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_step = scheduledStep;
		simulation.m_eventSchedule.doStep(scheduledStep);
		REQUIRE(actors.getLocation(actor) == block3);
	}
	SUBCASE("Unpathable route becomes pathable when blockage is removed.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(3, 3, 1);
		BlockIndex wallStart = blocks.getIndex_i(0, 4, 1);
		BlockIndex wallEnd = blocks.getIndex_i(9, 4, 2);
		BlockIndex destination = blocks.getIndex_i(3, 6, 1);
		areaBuilderUtil::setSolidWall(area, wallStart, wallEnd, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
		blocks.solid_setNot(wallStart);
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
	}
	SUBCASE("two by two creature cannot path through one block gap")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(2, 2, 1);
		BlockIndex destination = blocks.getIndex_i(8, 8, 1);
		blocks.solid_set(blocks.getIndex_i(1, 5, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 5, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(5, 5, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(7, 5, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(9, 5, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=troll,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
	}
	SUBCASE("walking path blocked by elevation")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 9, marble);
		BlockIndex origin = blocks.getIndex_i(1, 1, 1);
		BlockIndex destination = blocks.getIndex_i(8, 8, 8);		
		BlockIndex ledge = blocks.getIndex_i(8, 8, 7);
		blocks.solid_set(ledge, marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
	}
	SUBCASE("flying path")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 9, marble);
		BlockIndex origin = blocks.getIndex_i(1, 1, 1);
		BlockIndex destination = blocks.getIndex_i(8, 8, 8);		
		ActorIndex actor = actors.create({
			.species=eagle,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).size() == 7);
	}
	SUBCASE("swimming path")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 9, marble);
		BlockIndex water1 = blocks.getIndex_i(1, 1, 1);
		BlockIndex water2 = blocks.getIndex_i(8, 8, 8);		
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		ActorIndex actor = actors.create({
			.species=carp,
			.location=water1,
		});
		actors.move_setDestination(actor, water2);
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).size() == 7);
	}
}
TEST_CASE("route_5_5_3")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(5,5,3);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	static MaterialTypeId marble = MaterialType::byName("marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	static AnimalSpeciesId carp = AnimalSpecies::byName("carp");
	static FluidTypeId water = FluidType::byName("water");
	static MoveTypeId twoLegsAndSwimInWater = MoveType::byName("two legs and swim in water");
	static MoveTypeId twoLegs = MoveType::byName("two legs");
	SUBCASE("swimming path blocked")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, 1, marble);
		BlockIndex origin = blocks.getIndex_i(2, 1, 1);
		BlockIndex destination = blocks.getIndex_i(2, 3, 1);		
		blocks.solid_set(blocks.getIndex_i(1, 2, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(2, 2, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 2, 1), marble, false);
		BlockIndex water1 = blocks.getIndex_i(1, 1, 1);
		BlockIndex water2 = blocks.getIndex_i(3, 1, 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		BlockIndex water3 = blocks.getIndex_i(1, 3, 1);
		BlockIndex water4 = blocks.getIndex_i(3, 3, 1);
		areaBuilderUtil::setFullFluidCuboid(area, water3, water4, water);
		ActorIndex actor = actors.create({
			.species=carp,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
	}
	SUBCASE("walking path blocked by water if not also swimming")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		areaBuilderUtil::setSolidWalls(area, 2, marble);
		BlockIndex origin = blocks.getIndex_i(2, 1, 2);
		BlockIndex destination = blocks.getIndex_i(2, 3, 2);		
		BlockIndex midpoint = blocks.getIndex_i(2, 2, 1);
		blocks.solid_setNot(blocks.getIndex_i(1, 2, 1));
		blocks.solid_setNot(midpoint);
		blocks.solid_setNot(blocks.getIndex_i(3, 2, 1));
		BlockIndex water1 = blocks.getIndex_i(1, 2, 1);
		BlockIndex water2 = blocks.getIndex_i(3, 2, 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setType(actor, twoLegs);
		actors.move_setDestination(actor, destination);
		REQUIRE(!blocks.shape_moveTypeCanEnter(midpoint, actors.getMoveType(actor)));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
		actors.move_setType(actor, twoLegsAndSwimInWater);
		actors.move_setDestination(actor, destination);
		REQUIRE(blocks.shape_moveTypeCanEnter(midpoint, actors.getMoveType(actor)));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
	}
}
TEST_CASE("route_5_5_5")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(5,5,5);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	static MaterialTypeId marble = MaterialType::byName("marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	static MoveTypeId twoLegsAndClimb1 = MoveType::byName("two legs and climb 1");
	static MoveTypeId twoLegsAndClimb2 = MoveType::byName("two legs and climb 2");
	static const BlockFeatureType& stairs = BlockFeatureType::stairs;
	static const BlockFeatureType& ramp = BlockFeatureType::ramp;
	static const BlockFeatureType& door = BlockFeatureType::door;
	static const BlockFeatureType& fortification = BlockFeatureType::fortification;
	static FluidTypeId water = FluidType::byName("water");
	SUBCASE("walking path blocked by one height cliff if not climbing")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,1),
		});
		blocks.solid_set(blocks.getIndex_i(4, 4, 1), marble, false);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 4, 2));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
		actors.move_setType(actor, twoLegsAndClimb1);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 4, 2));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());

	}
	SUBCASE("walking path blocked by two height cliff if not climbing 2")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,1),
		});
		actors.move_setType(actor, twoLegsAndClimb1);
		blocks.solid_set(blocks.getIndex_i(4, 4, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(4, 4, 2), marble, false);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 4, 3));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
		actors.move_setType(actor, twoLegsAndClimb2);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 4, 3));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());

	}
	SUBCASE("stairs")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(1, 1, 1);
		blocks.blockFeature_construct(blocks.getIndex_i(2, 2, 1), stairs, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 2), stairs, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(2, 2, 3), stairs, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, blocks.getIndex_i(2, 2, 4));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
	}
	SUBCASE("ramp")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,1),
		});
		blocks.solid_set(blocks.getIndex_i(4, 4, 1), marble, false);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 4, 2));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(4, 3, 1), ramp, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 4, 2));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
	}
	SUBCASE("door")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex_i(3, 0, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 1, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 3, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 4, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,1),
		});
		actors.move_setDestination(actor, blocks.getIndex_i(4, 3, 1));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 2, 1), door, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 3, 1));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.blockFeature_lock(blocks.getIndex_i(3, 2, 1), door);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 3, 1));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
	}
	SUBCASE("fortification")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex_i(3, 0, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 1, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 3, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 4, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,1),
		});
		actors.move_setDestination(actor, blocks.getIndex_i(4, 3, 1));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 2, 1), fortification, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 3, 1));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
	}
	SUBCASE("can walk on floor")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex_i(1, 1, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(1, 3, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,2),
		});
		actors.move_setDestination(actor, blocks.getIndex_i(1, 3, 2));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(1, 2, 2), BlockFeatureType::floor, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(1, 3, 2));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
	}
	SUBCASE("floor blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_setNot(blocks.getIndex_i(3, 3, 1));
		blocks.solid_setNot(blocks.getIndex_i(3, 3, 2));
		blocks.solid_setNot(blocks.getIndex_i(3, 3, 3));
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 1), BlockFeatureType::stairs, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 2), BlockFeatureType::stairs, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(3,3,1),
		});
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 3), BlockFeatureType::floor, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
	}
	SUBCASE("can walk on floor grate")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex_i(1, 1, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(1, 3, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,2),
		});
		actors.move_setDestination(actor, blocks.getIndex_i(1, 3, 2));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(1, 2, 2), BlockFeatureType::floorGrate, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(1, 3, 2));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
	}
	SUBCASE("floor grate blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_setNot(blocks.getIndex_i(3, 3, 1));
		blocks.solid_setNot(blocks.getIndex_i(3, 3, 2));
		blocks.solid_setNot(blocks.getIndex_i(3, 3, 3));
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 1), BlockFeatureType::stairs, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 2), BlockFeatureType::stairs, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(3,3,1),
		});
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 3), BlockFeatureType::floorGrate, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
	}
	SUBCASE("can walk on hatch")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex_i(1, 1, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(1, 3, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,2),
		});
		actors.move_setDestination(actor, blocks.getIndex_i(1, 3, 2));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(1, 2, 2), BlockFeatureType::hatch, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(1, 3, 2));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
	}
	SUBCASE("locked hatch blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_setNot(blocks.getIndex_i(3, 3, 1));
		blocks.solid_setNot(blocks.getIndex_i(3, 3, 2));
		blocks.solid_setNot(blocks.getIndex_i(3, 3, 3));
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 1), BlockFeatureType::stairs, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 2), BlockFeatureType::stairs, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(3,3,1),
		});
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 3), BlockFeatureType::hatch, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.blockFeature_lock(blocks.getIndex_i(3, 3, 3), BlockFeatureType::hatch);
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
	}
	SUBCASE("multi-block actors can use ramps")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex_i(4, 4, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(4, 3, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 4, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 3, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,1),
		});
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 2));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 2, 1), BlockFeatureType::ramp, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(4, 2, 1), BlockFeatureType::ramp, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(3, 1, 1), BlockFeatureType::ramp, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(4, 1, 1), BlockFeatureType::ramp, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 2));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
	}
	SUBCASE("detour")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin = blocks.getIndex_i(2, 3, 1);
		blocks.solid_set(blocks.getIndex_i(3, 1, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 2, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(3, 4, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(3,3,1),
		});
		actors.move_setDestination(actor, blocks.getIndex_i(4, 3, 1));
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).size() == 2);
		//pathThreadedTask.clearReferences();
		//simulation.m_threadedTaskEngine.remove(pathThreadedTask);
		REQUIRE(blocks.shape_shapeAndMoveTypeCanEnterEverFrom(blocks.getIndex_i(3, 3, 1), actors.getShape(actor), actors.getMoveType(actor), actors.getLocation(actor)));
		REQUIRE(!blocks.shape_canEnterCurrentlyFrom(blocks.getIndex_i(3, 3, 1), actors.getShape(actor), actors.getLocation(actor), actors.getBlocks(actor)));
		REQUIRE(actors.move_hasEvent(actor));
		// Move attempt 1.
		simulation.m_step = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_eventSchedule.doStep(simulation.m_step);
		REQUIRE(actors.getLocation(actor) == origin);
		REQUIRE(actors.move_hasEvent(actor));
		// Move attempt 2.
		simulation.m_step = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_eventSchedule.doStep(simulation.m_step);
		// Move attempt 3.
		simulation.m_step = simulation.m_eventSchedule.m_data.begin()->first;
		simulation.m_eventSchedule.doStep(simulation.m_step);
		REQUIRE(actors.getLocation(actor) == origin);
		REQUIRE(simulation.m_threadedTaskEngine.count() == 1);
		REQUIRE(!actors.move_hasEvent(actor));
		// Detour.
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).size() == 6);
	}
	SUBCASE("air breathers cannot dive")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		BlockIndex surface = blocks.getIndex_i(3,3,3);
		BlockIndex deep = blocks.getIndex_i(3,3,2);
		blocks.solid_setNot(surface);
		blocks.solid_setNot(deep);
		blocks.fluid_add(surface, Config::maxBlockVolume, water);
		blocks.fluid_add(deep, Config::maxBlockVolume, water);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,4),
		});
		actors.move_setDestination(actor, deep);
		simulation.doStep();
		REQUIRE(actors.move_getPath(actor).empty());
	}
}
