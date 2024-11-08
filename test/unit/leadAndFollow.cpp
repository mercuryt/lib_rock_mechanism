#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/objectives/station.h"
#include "../../engine/portables.h"
TEST_CASE("leadAndFollow")
{
	Simulation simulation;
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	static AnimalSpeciesId troll = AnimalSpecies::byName("troll");
	static MaterialTypeId marble = MaterialType::byName("marble");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	SUBCASE("two dwarves")
	{
		BlockIndex origin1 = blocks.getIndex_i(2, 2, 1);
		BlockIndex origin2 = blocks.getIndex_i(1, 1, 1);
		BlockIndex destination1 = blocks.getIndex_i(9, 9, 1);
		BlockIndex destination2 = blocks.getIndex_i(8, 8, 1);
		ActorIndex dwarf1 = actors.create({
			.species=dwarf,
			.location=origin1,
		});
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=origin2,
		});
		actors.followActor(dwarf2, dwarf1);
		REQUIRE(actors.lineLead_getPath(dwarf1).size() == 2);
		REQUIRE(actors.lead_getSpeed(dwarf1) == actors.move_getSpeed(dwarf1));
		actors.move_setDestination(dwarf1, destination1);
		simulation.doStep();
		REQUIRE(actors.move_getPath(dwarf1).size() == 7);
		REQUIRE(actors.move_getDestination(dwarf1) == destination1);
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, destination1);
		REQUIRE(actors.getLocation(dwarf2) == destination2);
	}
	SUBCASE("dwarf leads troll")
	{
		BlockIndex origin1 = blocks.getIndex_i(3, 3, 1);
		BlockIndex origin2 = blocks.getIndex_i(2, 2, 1);
		BlockIndex destination1 = blocks.getIndex_i(8, 8, 1);
		BlockIndex destination2 = blocks.getIndex_i(7, 7, 1);
		ActorIndex dwarf1 = actors.create({
			.species=dwarf,
			.location=origin1,
		});
		ActorIndex troll1 = actors.create({
			.species=troll,
			.location=origin2,
		});
		actors.followActor(troll1, dwarf1);
		REQUIRE(actors.lead_getSpeed(dwarf1) == actors.move_getSpeed(dwarf1));
		actors.move_setDestination(dwarf1, destination1);
		simulation.doStep();
		REQUIRE(actors.move_getPath(dwarf1).size() == 5);
		REQUIRE(actors.move_getDestination(dwarf1) == destination1);
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, destination1);
		REQUIRE(actors.getLocation(troll1) == destination2);
	}
	SUBCASE("troll leads dwarf")
	{
		BlockIndex origin1 = blocks.getIndex_i(3, 2, 1);
		BlockIndex origin2 = blocks.getIndex_i(1, 1, 1);
		BlockIndex destination1 = blocks.getIndex_i(8, 8, 1);
		ActorIndex troll1 = actors.create({
			.species=troll,
			.location=origin1,
		});
		ActorIndex dwarf1 = actors.create({
			.species=dwarf,
			.location=origin2,
		});
		actors.followActor(dwarf1, troll1);
		REQUIRE(actors.lead_getSpeed(troll1) == actors.move_getSpeed(troll1));
		actors.move_setDestination(troll1, destination1);
		simulation.doStep();
		REQUIRE(actors.move_getPath(troll1).size() == 6);
		REQUIRE(actors.move_getDestination(troll1) == destination1);
		simulation.fastForwardUntillActorIsAtDestination(area, troll1, destination1);
		REQUIRE(actors.isAdjacentToActor(dwarf1, troll1));
	}
	SUBCASE("use largest shape for pathing")
	{
		BlockIndex origin1 = blocks.getIndex_i(2, 2, 1);
		BlockIndex origin2 = blocks.getIndex_i(1, 1, 1);
		BlockIndex destination1 = blocks.getIndex_i(8, 8, 1);
		blocks.solid_set(blocks.getIndex_i(5, 1, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(5, 3, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(5, 5, 1), marble, false);
		blocks.solid_set(blocks.getIndex_i(5, 7, 1), marble, false);
		ActorIndex dwarf1 = actors.create({
			.species=AnimalSpecies::byName("dwarf"),
			.location=origin1,
		});
		ActorIndex troll1 = actors.create({
			.species=troll,
			.location=origin2,
		});
		actors.followActor(troll1, dwarf1);
		actors.move_setDestination(dwarf1, destination1);
		simulation.doStep();
		REQUIRE(actors.move_getPath(dwarf1).size() > 6);
	}
	SUBCASE("wait for follower to keep up if blocked temporarily")
	{
		BlockIndex origin1 = blocks.getIndex_i(2, 2, 1);
		BlockIndex origin2 = blocks.getIndex_i(1, 1, 1);
		BlockIndex origin3 = blocks.getIndex_i(3, 2, 1);
		BlockIndex destination1 = blocks.getIndex_i(8, 8, 1);
		BlockIndex destination2 = blocks.getIndex_i(7, 7, 1);
		ActorIndex dwarf1 = actors.create({
			.species=dwarf,
			.location=origin1,
		});
		ActorIndex troll1 = actors.create({
			.species=troll,
			.location=origin2,
		});
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=origin3,
		});
		std::unique_ptr<Objective> stationObjective = std::make_unique<StationObjective>(origin3);
		actors.objective_addTaskToStart(dwarf2,std::move(stationObjective)); 
		actors.followActor(troll1, dwarf1);
		actors.move_setDestination(dwarf1, destination1);
		simulation.doStep();
		REQUIRE(actors.move_getPath(dwarf1).size() == 6);
		Step stepsTillMoveEvent = actors.move_stepsTillNextMoveEvent(dwarf1);
		simulation.fastForward(stepsTillMoveEvent);
		//Because troll1 is blocked neither dwarf1 or troll1 have moved.
		REQUIRE(actors.getLocation(dwarf1) == origin1);
		REQUIRE(actors.getLocation(troll1) == origin2);
		REQUIRE(actors.move_getRetries(dwarf1) == 1);
		REQUIRE(actors.move_hasEvent(dwarf1));
		REQUIRE(!blocks.shape_canEnterCurrentlyFrom(origin1, actors.getShape(troll1), actors.getLocation(troll1), actors.lineLead_getOccupiedBlocks(dwarf1)));
		actors.setLocation(dwarf2, blocks.getIndex_i(9, 1, 1));
		REQUIRE(blocks.shape_canEnterCurrentlyFrom(origin1, actors.getShape(troll1), actors.getLocation(troll1), actors.lineLead_getOccupiedBlocks(dwarf1)));
		REQUIRE(actors.move_hasEvent(dwarf1));
		Step delay = actors.move_stepsTillNextMoveEvent(dwarf1);
		simulation.fastForward(delay);
		REQUIRE(actors.getLocation(troll1) != origin2);
		REQUIRE(actors.isAdjacentToActor(troll1, dwarf1));
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, destination1);
		REQUIRE(actors.getLocation(troll1) == destination2);
	}
	SUBCASE("repath if follower blocked permanantly")
	{
		BlockIndex origin1 = blocks.getIndex_i(2, 2, 1);
		BlockIndex origin2 = blocks.getIndex_i(1, 1, 1);
		BlockIndex destination1 = blocks.getIndex_i(8, 8, 1);
		ActorIndex dwarf1 = actors.create({
			.species=dwarf,
			.location=origin1,
		});
		ActorIndex troll1 = actors.create({
			.species=troll,
			.location=origin2,
		});
		actors.followActor(troll1, dwarf1);
		actors.move_setDestination(dwarf1, destination1);
		simulation.doStep();
		blocks.solid_set(blocks.getIndex_i(2, 1, 1), marble, false);
		Step stepsTillMoveEvent = actors.move_stepsTillNextMoveEvent(dwarf1);
		simulation.fastForward(stepsTillMoveEvent);
		//Because troll1 is blocked neither dwarf1 or troll1 have moved.
		REQUIRE(actors.getLocation(dwarf1) == origin1);
		REQUIRE(actors.getLocation(troll1) == origin2);
		REQUIRE(actors.move_getRetries(dwarf1) == 0);
		REQUIRE(!actors.move_hasEvent(dwarf1));
		REQUIRE(actors.move_hasPathRequest(dwarf1));
	}
}
