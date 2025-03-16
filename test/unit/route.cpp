#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/plants.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/items/items.h"
#include "../../engine/materialType.h"
#include "../../engine/moveType.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/objectives/goTo.h"
#include "dummyObjective.h"
TEST_CASE("route_10_10_10")
{
	Simulation simulation;
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
		CHECK(actors.move_getPath(actor).size() == 4);
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
		CHECK(actors.move_getPath(actor).size() == 7);
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
		CHECK(actors.move_getPath(actor).empty());
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
		CHECK(!actors.move_hasEvent(actor));
		simulation.doStep();
		CHECK(actors.move_hasEvent(actor));
		Step stepsUntillScheduledStep = actors.move_stepsTillNextMoveEvent(actor);
		float seconds = (float)stepsUntillScheduledStep.get() / (float)Config::stepsPerSecond.get();
		CHECK(seconds >= 0.6f);
		CHECK(seconds <= 0.9f);
		simulation.fastForward(stepsUntillScheduledStep - 2);
		CHECK(actors.getLocation(actor) == origin);
		CHECK(actors.move_hasEvent(actor));
		// step 1
		simulation.doStep();
		CHECK(actors.getLocation(actor) == block1);
		CHECK(actors.move_hasEvent(actor));
		CHECK(stepsUntillScheduledStep == actors.move_stepsTillNextMoveEvent(actor));
		simulation.fastForward(stepsUntillScheduledStep - 2);
		CHECK(actors.getLocation(actor) == block1);
		CHECK(actors.move_hasEvent(actor));
		// step 2
		simulation.doStep();
		CHECK(actors.getLocation(actor) == destination);
		CHECK(!actors.move_hasEvent(actor));
		CHECK(actors.move_getPath(actor).empty());
		CHECK(actors.move_getDestination(actor).empty());
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
		actors.objective_addTaskToEnd(actor, std::make_unique<GoToObjective>(destination));
		simulation.doStep();
		CHECK(actors.move_hasEvent(actor));
		CHECK(actors.move_getPath(actor)[0] == block1);
		CHECK(actors.move_getPath(actor)[1] == block2);
		CHECK(actors.move_getPath(actor)[2] == destination);
		// Step 1.
		//simulation.fastForwardUntillActorIsAt(area, actor, block1);
		simulation.doStep();
		simulation.doStep();
		CHECK(actors.getLocation(actor) == origin);
		simulation.doStep();
		CHECK(actors.getLocation(actor) == block1);
		CHECK(actors.move_hasEvent(actor));
		blocks.solid_set(block2, marble, false);
		// Step 2.
		CHECK(actors.move_hasEvent(actor));
		Step stepsTillNextMove = actors.move_stepsTillNextMoveEvent(actor);
		simulation.fastForward(stepsTillNextMove - 1);
		CHECK(actors.getLocation(actor) == block1);
		CHECK(!actors.move_hasEvent(actor));
		CHECK(actors.move_hasPathRequest(actor));
		// Step 3.
		simulation.doStep();
		CHECK(!actors.move_hasPathRequest(actor));
		CHECK(actors.move_hasEvent(actor));
		CHECK(actors.move_getPath(actor)[0] == block3);
		stepsTillNextMove = actors.move_stepsTillNextMoveEvent(actor);
		simulation.fastForwardUntillActorIsAt(area, actor, block3);
		CHECK(actors.move_hasEvent(actor));
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
		CHECK(actors.move_getPath(actor).empty());
		blocks.solid_setNot(wallStart);
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
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
		CHECK(actors.move_getPath(actor).empty());
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
		CHECK(actors.move_getPath(actor).empty());
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
		CHECK(actors.move_getPath(actor).size() == 7);
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
		CHECK(actors.move_getPath(actor).size() == 7);
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
		CHECK(actors.move_getPath(actor).empty());
	}
	SUBCASE("walking path blocked by water if not also swimming")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		areaBuilderUtil::setSolidWalls(area, 2, marble);
		BlockIndex origin = blocks.getIndex_i(2, 1, 2);
		BlockIndex destination = blocks.getIndex_i(2, 3, 2);
		BlockIndex midpoint = blocks.getIndex_i(2, 2, 1);
		BlockIndex water1 = blocks.getIndex_i(1, 2, 1);
		BlockIndex water2 = blocks.getIndex_i(3, 2, 1);
		blocks.solid_setNot(water1);
		blocks.solid_setNot(midpoint);
		blocks.solid_setNot(water2);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setType(actor, twoLegs);
		actors.move_setDestination(actor, destination);
		CHECK(!blocks.shape_moveTypeCanEnter(midpoint, actors.getMoveType(actor)));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		actors.move_setType(actor, twoLegsAndSwimInWater);
		actors.move_setDestination(actor, destination);
		CHECK(blocks.shape_moveTypeCanEnter(midpoint, actors.getMoveType(actor)));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
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
		CHECK(actors.move_getPath(actor).empty());
		actors.move_setType(actor, twoLegsAndClimb1);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 4, 2));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());

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
		CHECK(actors.move_getPath(actor).empty());
		actors.move_setType(actor, twoLegsAndClimb2);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 4, 3));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());

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
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("ramp")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		blocks.solid_set(blocks.getIndex_i(4, 4, 1), marble, false);
		BlockIndex destination = blocks.getIndex_i(4, 4, 2);
		BlockIndex rampLocation = blocks.getIndex_i(4, 3, 1);
		BlockIndex adjacentToRamp = blocks.getIndex_i(4, 2, 1);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1,1,1),
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(rampLocation, ramp, marble);
		CHECK(blocks.shape_shapeAndMoveTypeCanEnterEverFrom(rampLocation, actors.getShape(actor), actors.getMoveType(actor), adjacentToRamp));
		CHECK(blocks.getAdjacentWithEdgeAndCornerAdjacent(adjacentToRamp)[13] == rampLocation);
		auto& facade = area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(actor));
		CHECK(facade.getValueForBit(adjacentToRamp, rampLocation));
		bool canEnterFrom = facade.canEnterFrom(adjacentToRamp, AdjacentIndex::create(13));
		CHECK(canEnterFrom);
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
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
		CHECK(!actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 2, 1), door, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 3, 1));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		blocks.blockFeature_lock(blocks.getIndex_i(3, 2, 1), door);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 3, 1));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
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
		CHECK(!actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 2, 1), fortification, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(4, 3, 1));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
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
		CHECK(actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(1, 2, 2), BlockFeatureType::floor, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(1, 3, 2));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("floor blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
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
		CHECK(!actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 3), BlockFeatureType::floor, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
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
		CHECK(actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(1, 2, 2), BlockFeatureType::floorGrate, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(1, 3, 2));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("floor grate blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
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
		CHECK(!actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 3), BlockFeatureType::floorGrate, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
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
		CHECK(actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(1, 2, 2), BlockFeatureType::hatch, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(1, 3, 2));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("locked hatch blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
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
		CHECK(!actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 3, 3), BlockFeatureType::hatch, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		blocks.blockFeature_lock(blocks.getIndex_i(3, 3, 3), BlockFeatureType::hatch);
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 3));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
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
		CHECK(actors.move_getPath(actor).empty());
		blocks.blockFeature_construct(blocks.getIndex_i(3, 2, 1), BlockFeatureType::ramp, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(4, 2, 1), BlockFeatureType::ramp, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(3, 1, 1), BlockFeatureType::ramp, marble);
		blocks.blockFeature_construct(blocks.getIndex_i(4, 1, 1), BlockFeatureType::ramp, marble);
		actors.move_setDestination(actor, blocks.getIndex_i(3, 3, 2));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
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
		CHECK(actors.move_getPath(actor).size() == 2);
		//pathThreadedTask.clearReferences();
		//simulation.m_threadedTaskEngine.remove(pathThreadedTask);
		CHECK(blocks.shape_shapeAndMoveTypeCanEnterEverFrom(blocks.getIndex_i(3, 3, 1), actors.getShape(actor), actors.getMoveType(actor), actors.getLocation(actor)));
		CHECK(!blocks.shape_canEnterCurrentlyFrom(blocks.getIndex_i(3, 3, 1), actors.getShape(actor), actors.getLocation(actor), actors.getBlocks(actor)));
		CHECK(actors.move_hasEvent(actor));
		// Move attempt 1.
		Step stepsUntillScheduledStep = actors.move_stepsTillNextMoveEvent(actor);
		simulation.fastForward(stepsUntillScheduledStep - 1);
		CHECK(actors.getLocation(actor) == origin);
		CHECK(actors.move_hasEvent(actor));
		CHECK(actors.move_getRetries(actor) == 1);
		// Move attempt 2.
		stepsUntillScheduledStep = actors.move_stepsTillNextMoveEvent(actor);
		simulation.fastForward(stepsUntillScheduledStep - 1);
		CHECK(actors.move_getRetries(actor) == 2);
		// Move attempt 3.
		simulation.fastForward(stepsUntillScheduledStep - 1);
		CHECK(actors.getLocation(actor) == origin);
		CHECK(actors.move_hasPathRequest(actor));
		CHECK(!actors.move_hasEvent(actor));
		// Detour.
		simulation.doStep();
		CHECK(actors.move_getPath(actor).size() == 6);
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
		CHECK(actors.move_getPath(actor).empty());
	}
}
