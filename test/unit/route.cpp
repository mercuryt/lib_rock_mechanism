#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/plants.h"
#include "../../engine/items/items.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/definitions/materialType.h"
#include "../../engine/definitions/moveType.h"
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
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	SUBCASE("Route through open space")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D origin = Point3D::create(3, 3, 1);
		Point3D destination = Point3D::create(7, 7, 1);
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
		Point3D origin = Point3D::create(3, 3, 1);
		Point3D block1 = Point3D::create(5, 3, 1);
		Point3D block2 = Point3D::create(5, 4, 1);
		Point3D block3 = Point3D::create(5, 5, 1);
		Point3D block4 = Point3D::create(5, 6, 1);
		Point3D block5 = Point3D::create(5, 7, 1);
		Point3D destination = Point3D::create(7, 7, 1);
		space.solid_set(block1, marble, false);
		space.solid_set(block2, marble, false);
		space.solid_set(block3, marble, false);
		space.solid_set(block4, marble, false);
		space.solid_set(block5, marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		CHECK(actors.move_getPath(actor).size() > 4);
	}
	SUBCASE("No route found")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D origin = Point3D::create(0, 0, 1);
		Point3D block1 = Point3D::create(2, 0, 1);
		Point3D block2 = Point3D::create(2, 1, 1);
		Point3D block3 = Point3D::create(2, 2, 1);
		Point3D block4 = Point3D::create(1, 2, 1);
		Point3D block5 = Point3D::create(0, 2, 1);
		Point3D destination = Point3D::create(7, 7, 1);
		space.solid_set(block1, marble, false);
		space.solid_set(block2, marble, false);
		space.solid_set(block3, marble, false);
		space.solid_set(block4, marble, false);
		space.solid_set(block5, marble, false);
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
		Point3D origin = Point3D::create(3, 3, 1);
		Point3D block1 = Point3D::create(4, 4, 1);
		Point3D destination = Point3D::create(5, 5, 1);
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
		Point3D origin = Point3D::create(3, 3, 1);
		Point3D block1 = Point3D::create(3, 4, 1);
		Point3D block2 = Point3D::create(3, 5, 1);
		Point3D block3 = Point3D::create(4, 5, 1);
		Point3D destination = Point3D::create(3, 6, 1);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		// Set solid to make choice of route deterministic.
		Cuboid wall = {Point3D::create(2, 9, 1), Point3D::create(2,0,1)};
		space.solid_setCuboid(wall, marble, false);
		actors.objective_addTaskToEnd(actor, std::make_unique<GoToObjective>(destination));
		simulation.doStep();
		CHECK(actors.move_hasEvent(actor));
		CHECK(actors.move_getPath(actor)[2] == block1);
		CHECK(actors.move_getPath(actor)[1] == block2);
		CHECK(actors.move_getPath(actor)[0] == destination);
		// Step 1.
		//simulation.fastForwardUntillActorIsAt(area, actor, block1);
		simulation.doStep();
		simulation.doStep();
		CHECK(actors.getLocation(actor) == origin);
		simulation.doStep();
		CHECK(actors.getLocation(actor) == block1);
		CHECK(actors.move_hasEvent(actor));
		space.solid_set(block2, marble, false);
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
		CHECK(actors.move_getPath(actor).back() == block3);
		stepsTillNextMove = actors.move_stepsTillNextMoveEvent(actor);
		simulation.fastForwardUntillActorIsAt(area, actor, block3);
		CHECK(actors.move_hasEvent(actor));
	}
	SUBCASE("Unpathable route becomes pathable when blockage is removed.")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D origin = Point3D::create(3, 3, 1);
		Point3D wallStart = Point3D::create(0, 4, 1);
		Point3D wallEnd = Point3D::create(9, 4, 2);
		Point3D destination = Point3D::create(3, 6, 1);
		areaBuilderUtil::setSolidWall(area, wallStart, wallEnd, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		space.solid_setNot(wallStart);
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("two by two creature cannot path through one block gap")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D origin = Point3D::create(2, 2, 1);
		Point3D destination = Point3D::create(8, 8, 1);
		space.solid_set(Point3D::create(1, 5, 1), marble, false);
		space.solid_set(Point3D::create(3, 5, 1), marble, false);
		space.solid_set(Point3D::create(5, 5, 1), marble, false);
		space.solid_set(Point3D::create(7, 5, 1), marble, false);
		space.solid_set(Point3D::create(9, 5, 1), marble, false);
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
		Point3D origin = Point3D::create(1, 1, 1);
		Point3D destination = Point3D::create(8, 8, 8);
		Point3D ledge = Point3D::create(8, 8, 7);
		space.solid_set(ledge, marble, false);
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
		Point3D origin = Point3D::create(1, 1, 1);
		Point3D destination = Point3D::create(8, 8, 8);
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
		Point3D water1 = Point3D::create(1, 1, 1);
		Point3D water2 = Point3D::create(8, 8, 8);
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
	Space& space = area.getSpace();
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
		Point3D origin = Point3D::create(2, 1, 1);
		Point3D destination = Point3D::create(2, 3, 1);
		space.solid_set(Point3D::create(1, 2, 1), marble, false);
		space.solid_set(Point3D::create(2, 2, 1), marble, false);
		space.solid_set(Point3D::create(3, 2, 1), marble, false);
		Point3D water1 = Point3D::create(1, 1, 1);
		Point3D water2 = Point3D::create(3, 1, 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		Point3D water3 = Point3D::create(1, 3, 1);
		Point3D water4 = Point3D::create(3, 3, 1);
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
		Point3D origin = Point3D::create(2, 1, 2);
		Point3D destination = Point3D::create(2, 3, 2);
		Point3D midpoint = Point3D::create(2, 2, 1);
		Point3D water1 = Point3D::create(1, 2, 1);
		Point3D water2 = Point3D::create(3, 2, 1);
		space.solid_setNot(water1);
		space.solid_setNot(midpoint);
		space.solid_setNot(water2);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.move_setType(actor, twoLegs);
		actors.move_setDestination(actor, destination);
		CHECK(!space.shape_moveTypeCanEnter(midpoint, actors.getMoveType(actor)));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		actors.move_setType(actor, twoLegsAndSwimInWater);
		actors.move_setDestination(actor, destination);
		CHECK(space.shape_moveTypeCanEnter(midpoint, actors.getMoveType(actor)));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
}
TEST_CASE("route_5_5_5")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(5,5,5);
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	static MaterialTypeId marble = MaterialType::byName("marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	static MoveTypeId twoLegsAndClimb1 = MoveType::byName("two legs and climb 1");
	static MoveTypeId twoLegsAndClimb2 = MoveType::byName("two legs and climb 2");
	static MoveTypeId twoLegsAndSwimInWater = MoveType::byName("two legs and swim in water");
	static const PointFeatureTypeId stairs = PointFeatureTypeId::Stairs;
	static const PointFeatureTypeId ramp = PointFeatureTypeId::Ramp;
	static const PointFeatureTypeId door = PointFeatureTypeId::Door;
	static const PointFeatureTypeId fortification = PointFeatureTypeId::Fortification;
	static FluidTypeId water = FluidType::byName("water");
	SUBCASE("walking path blocked by one height cliff if not climbing")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(1,1,1),
		});
		space.solid_set(Point3D::create(4, 4, 1), marble, false);
		actors.move_setDestination(actor, Point3D::create(4, 4, 2));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		actors.move_setType(actor, twoLegsAndClimb1);
		actors.move_setDestination(actor, Point3D::create(4, 4, 2));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());

	}
	SUBCASE("walking path blocked by two height cliff if not climbing 2")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		const Point3D& start = Point3D::create(0,0,1);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=start,
		});
		actors.move_setType(actor, twoLegsAndClimb1);
		space.solid_set(Point3D::create(0, 1, 1), marble, false);
		space.solid_set(Point3D::create(0, 1, 2), marble, false);
		const Point3D& destination = Point3D::create(0, 1, 3);
		const Point3D& belowDestination = Point3D::create(0, 0, 2);
		bool test = !space.shape_moveTypeCanEnter(belowDestination, twoLegsAndClimb1);
		CHECK(test);
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		actors.move_setType(actor, twoLegsAndClimb2);
		test = space.shape_moveTypeCanEnterFrom(belowDestination, twoLegsAndClimb2, start);
		CHECK(test);
		test = space.shape_moveTypeCanEnterFrom(destination, twoLegsAndClimb2, belowDestination);
		CHECK(test);
		AdjacentData adjacentData = area.m_hasTerrainFacades.getForMoveType(twoLegsAndClimb2).getAdjacentData(start);
		CHECK(adjacentData.check({21}));
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());

	}
	SUBCASE("stairs")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		const Point3D origin = Point3D::create(0, 0, 1);
		const Point3D middle = Point3D::create(0, 0, 2);
		const Point3D highest = Point3D::create(0, 0, 3);
		space.pointFeature_construct(origin, stairs, marble);
		space.pointFeature_construct(middle, stairs, marble);
		space.pointFeature_construct(highest, stairs, marble);
		space.prepareRtrees();
		CHECK(space.shape_canStandIn(middle));
		bool test = space.shape_moveTypeCanEnter(middle, twoLegsAndSwimInWater);
		CHECK(test);
		test = space.shape_moveTypeCanEnterFrom(middle, twoLegsAndSwimInWater, origin);
		CHECK(test);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		CHECK(actors.getMoveType(actor) == twoLegsAndSwimInWater);
		AdjacentData adjacentData = area.m_hasTerrainFacades.getForMoveType(twoLegsAndSwimInWater).getAdjacentData(origin);
		CHECK(adjacentData.check({21}));
		actors.move_setDestination(actor, Point3D::create(0, 0, 4));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("ramp")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		space.solid_set(Point3D::create(4, 4, 1), marble, false);
		Point3D destination = Point3D::create(4, 4, 2);
		Point3D rampLocation = Point3D::create(4, 3, 1);
		Point3D adjacentToRamp = Point3D::create(4, 2, 1);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(1,1,1),
		});
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		space.pointFeature_construct(rampLocation, ramp, marble);
		CHECK(space.shape_shapeAndMoveTypeCanEnterEverFrom(rampLocation, actors.getShape(actor), actors.getMoveType(actor), adjacentToRamp));
		CHECK(space.getAdjacentWithEdgeAndCornerAdjacent(adjacentToRamp).contains(rampLocation));
		auto& facade = area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(actor));
		CHECK(facade.getValue(adjacentToRamp, rampLocation));
		actors.move_setDestination(actor, destination);
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("door")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		space.solid_set(Point3D::create(3, 0, 1), marble, false);
		space.solid_set(Point3D::create(3, 1, 1), marble, false);
		space.solid_set(Point3D::create(3, 3, 1), marble, false);
		space.solid_set(Point3D::create(3, 4, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(1,1,1),
		});
		actors.move_setDestination(actor, Point3D::create(4, 3, 1));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		space.pointFeature_construct(Point3D::create(3, 2, 1), door, marble);
		actors.move_setDestination(actor, Point3D::create(4, 3, 1));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		space.pointFeature_lock(Point3D::create(3, 2, 1), door);
		actors.move_setDestination(actor, Point3D::create(4, 3, 1));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
	}
	SUBCASE("fortification")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		space.solid_set(Point3D::create(3, 0, 1), marble, false);
		space.solid_set(Point3D::create(3, 1, 1), marble, false);
		space.solid_set(Point3D::create(3, 3, 1), marble, false);
		space.solid_set(Point3D::create(3, 4, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(1,1,1),
		});
		actors.move_setDestination(actor, Point3D::create(4, 3, 1));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		space.pointFeature_construct(Point3D::create(3, 2, 1), fortification, marble);
		actors.move_setDestination(actor, Point3D::create(4, 3, 1));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
	}
	SUBCASE("can walk on floor")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		space.solid_set(Point3D::create(1, 1, 1), marble, false);
		space.solid_set(Point3D::create(1, 3, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(1,1,2),
		});
		actors.move_setDestination(actor, Point3D::create(1, 3, 2));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		space.pointFeature_construct(Point3D::create(1, 2, 2), PointFeatureTypeId::Floor, marble);
		actors.move_setDestination(actor, Point3D::create(1, 3, 2));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("floor blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		space.solid_setNot(Point3D::create(3, 3, 1));
		space.solid_setNot(Point3D::create(3, 3, 2));
		space.solid_setNot(Point3D::create(3, 3, 3));
		space.pointFeature_construct(Point3D::create(3, 3, 1), PointFeatureTypeId::Stairs, marble);
		space.pointFeature_construct(Point3D::create(3, 3, 2), PointFeatureTypeId::Stairs, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(3,3,1),
		});
		actors.move_setDestination(actor, Point3D::create(3, 3, 3));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		space.pointFeature_construct(Point3D::create(3, 3, 3), PointFeatureTypeId::Floor, marble);
		actors.move_setDestination(actor, Point3D::create(3, 3, 3));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
	}
	SUBCASE("can walk on floor grate")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		space.solid_set(Point3D::create(1, 1, 1), marble, false);
		space.solid_set(Point3D::create(1, 3, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(1,1,2),
		});
		actors.move_setDestination(actor, Point3D::create(1, 3, 2));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		space.pointFeature_construct(Point3D::create(1, 2, 2), PointFeatureTypeId::FloorGrate, marble);
		actors.move_setDestination(actor, Point3D::create(1, 3, 2));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("floor grate blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		space.solid_setNot(Point3D::create(3, 3, 1));
		space.solid_setNot(Point3D::create(3, 3, 2));
		space.solid_setNot(Point3D::create(3, 3, 3));
		space.pointFeature_construct(Point3D::create(3, 3, 1), PointFeatureTypeId::Stairs, marble);
		space.pointFeature_construct(Point3D::create(3, 3, 2), PointFeatureTypeId::Stairs, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(3,3,1),
		});
		actors.move_setDestination(actor, Point3D::create(3, 3, 3));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		space.pointFeature_construct(Point3D::create(3, 3, 3), PointFeatureTypeId::FloorGrate, marble);
		actors.move_setDestination(actor, Point3D::create(3, 3, 3));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
	}
	SUBCASE("can walk on hatch")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		space.solid_set(Point3D::create(1, 1, 1), marble, false);
		space.solid_set(Point3D::create(1, 3, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(1,1,2),
		});
		actors.move_setDestination(actor, Point3D::create(1, 3, 2));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		space.pointFeature_construct(Point3D::create(1, 2, 2), PointFeatureTypeId::Hatch, marble);
		actors.move_setDestination(actor, Point3D::create(1, 3, 2));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("locked hatch blocks vertical travel")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		space.solid_setNot(Point3D::create(3, 3, 1));
		space.solid_setNot(Point3D::create(3, 3, 2));
		space.solid_setNot(Point3D::create(3, 3, 3));
		space.pointFeature_construct(Point3D::create(3, 3, 1), PointFeatureTypeId::Stairs, marble);
		space.pointFeature_construct(Point3D::create(3, 3, 2), PointFeatureTypeId::Stairs, marble);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(3,3,1),
		});
		actors.move_setDestination(actor, Point3D::create(3, 3, 3));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		space.pointFeature_construct(Point3D::create(3, 3, 3), PointFeatureTypeId::Hatch, marble);
		actors.move_setDestination(actor, Point3D::create(3, 3, 3));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		space.pointFeature_lock(Point3D::create(3, 3, 3), PointFeatureTypeId::Hatch);
		actors.move_setDestination(actor, Point3D::create(3, 3, 3));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
	}
	SUBCASE("multi-block actors can use ramps")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		space.solid_set(Point3D::create(4, 4, 1), marble, false);
		space.solid_set(Point3D::create(4, 3, 1), marble, false);
		space.solid_set(Point3D::create(3, 4, 1), marble, false);
		space.solid_set(Point3D::create(3, 3, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(1,1,1),
		});
		actors.move_setDestination(actor, Point3D::create(3, 3, 2));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).empty());
		space.pointFeature_construct(Point3D::create(3, 2, 1), PointFeatureTypeId::Ramp, marble);
		space.pointFeature_construct(Point3D::create(4, 2, 1), PointFeatureTypeId::Ramp, marble);
		space.pointFeature_construct(Point3D::create(3, 1, 1), PointFeatureTypeId::Ramp, marble);
		space.pointFeature_construct(Point3D::create(4, 1, 1), PointFeatureTypeId::Ramp, marble);
		actors.move_setDestination(actor, Point3D::create(3, 3, 2));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
	}
	SUBCASE("detour")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D origin = Point3D::create(2, 3, 1);
		Point3D blockerLocation = Point3D::create(3,3,1);
		space.solid_set(Point3D::create(3, 1, 1), marble, false);
		space.solid_set(Point3D::create(3, 2, 1), marble, false);
		space.solid_set(Point3D::create(3, 4, 1), marble, false);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=origin,
		});
		actors.create({
			.species=dwarf,
			.location=blockerLocation
		});
		actors.move_setDestination(actor, Point3D::create(4, 3, 1));
		simulation.doStep();
		CHECK(actors.move_getPath(actor).size() == 2);
		//pathThreadedTask.clearReferences();
		//simulation.m_threadedTaskEngine.remove(pathThreadedTask);
		CHECK(space.shape_shapeAndMoveTypeCanEnterEverFrom(Point3D::create(3, 3, 1), actors.getShape(actor), actors.getMoveType(actor), actors.getLocation(actor)));
		CHECK(space.shape_getDynamicVolume(blockerLocation) == 100);
		CHECK(!space.shape_canEnterCurrentlyFrom(blockerLocation, actors.getShape(actor), origin, actors.getOccupied(actor)));
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
		Point3D surface = Point3D::create(3,3,3);
		Point3D deep = Point3D::create(3,3,2);
		space.solid_setNot(surface);
		space.solid_setNot(deep);
		space.fluid_add(surface, Config::maxPointVolume, water);
		space.fluid_add(deep, Config::maxPointVolume, water);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=Point3D::create(1,1,4),
		});
		CHECK(!actors.move_canPathTo(actor, deep));
	}
}
