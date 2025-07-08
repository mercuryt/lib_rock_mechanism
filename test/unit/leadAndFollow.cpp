#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/definitions/animalSpecies.h"
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
	Blocks& blocks = area.getSpace();
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
		CHECK(actors.lineLead_getPath(dwarf1).size() == 2);
		CHECK(actors.lead_getSpeed(dwarf1) == actors.move_getSpeed(dwarf1));
		actors.move_setDestination(dwarf1, destination1);
		simulation.doStep();
		CHECK(actors.move_getPath(dwarf1).size() == 7);
		CHECK(actors.move_getDestination(dwarf1) == destination1);
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, destination1);
		CHECK(actors.getLocation(dwarf2) == destination2);
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
		CHECK(actors.lead_getSpeed(dwarf1) == actors.move_getSpeed(dwarf1));
		actors.move_setDestination(dwarf1, destination1);
		simulation.doStep();
		CHECK(actors.move_getPath(dwarf1).size() == 5);
		CHECK(actors.move_getDestination(dwarf1) == destination1);
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, destination1);
		CHECK(actors.getLocation(troll1) == destination2);
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
		CHECK(actors.lead_getSpeed(troll1) == actors.move_getSpeed(troll1));
		actors.move_setDestination(troll1, destination1);
		simulation.doStep();
		CHECK(actors.move_getPath(troll1).size() == 6);
		CHECK(actors.move_getDestination(troll1) == destination1);
		simulation.fastForwardUntillActorIsAtDestination(area, troll1, destination1);
		CHECK(actors.isAdjacentToActor(dwarf1, troll1));
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
		CHECK(actors.move_getPath(dwarf1).size() > 6);
	}
	SUBCASE("wait for follower to keep up if blocked temporarily")
	{
		BlockIndex leaderOrigin = blocks.getIndex_i(1, 2, 1);
		BlockIndex followerOrigin = blocks.getIndex_i(1, 1, 1);
		BlockIndex blockerOrigin = blocks.getIndex_i(2, 2, 1);
		BlockIndex destination1 = blocks.getIndex_i(1, 8, 1);
		BlockIndex destination2 = blocks.getIndex_i(1, 7, 1);
		ActorIndex dwarf1 = actors.create({
			.species=dwarf,
			.location=leaderOrigin,
			.facing=Facing4::South,
		});
		ActorIndex troll1 = actors.create({
			.species=troll,
			.location=followerOrigin,
			.facing=Facing4::South,
		});
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=blockerOrigin,
		});
		std::unique_ptr<Objective> stationObjective = std::make_unique<StationObjective>(blockerOrigin);
		actors.objective_addTaskToStart(dwarf2,std::move(stationObjective));
		actors.followActor(troll1, dwarf1);
		actors.move_setDestination(dwarf1, destination1);
		simulation.doStep();
		CHECK(actors.move_getPath(dwarf1).size() == 6);
		Step stepsTillMoveEvent = actors.move_stepsTillNextMoveEvent(dwarf1);
		simulation.fastForward(stepsTillMoveEvent);
		//Because troll1 is blocked neither dwarf1 or troll1 have moved.
		CHECK(actors.getLocation(dwarf1) == leaderOrigin);
		CHECK(actors.getLocation(troll1) == followerOrigin);
		CHECK(actors.move_getRetries(dwarf1) == 1);
		CHECK(actors.move_hasEvent(dwarf1));
		CHECK(!blocks.shape_canEnterCurrentlyFrom(leaderOrigin, actors.getShape(troll1), actors.getLocation(troll1), actors.lineLead_getOccupiedPoints(dwarf1)));
		const BlockIndex& toMoveTo = blocks.getIndex_i(9, 1, 1);
		actors.location_set(dwarf2, toMoveTo, Facing4::North);
		CHECK(blocks.shape_canEnterCurrentlyFrom(leaderOrigin, actors.getShape(troll1), actors.getLocation(troll1), actors.lineLead_getOccupiedPoints(dwarf1)));
		CHECK(actors.move_hasEvent(dwarf1));
		Step delay = actors.move_stepsTillNextMoveEvent(dwarf1);
		simulation.fastForward(delay);
		CHECK(actors.getLocation(troll1) != followerOrigin);
		CHECK(actors.isAdjacentToActor(troll1, dwarf1));
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, destination1);
		CHECK(actors.getLocation(troll1) == destination2);
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
		BlockIndex firstStep = actors.move_getPath(dwarf1).back();
		blocks.solid_set(firstStep, marble, false);
		Step stepsTillMoveEvent = actors.move_stepsTillNextMoveEvent(dwarf1);
		//TODO: why minus 1? is steps till next move event wrong?
		simulation.fastForward(stepsTillMoveEvent - 1);
		//Because troll1 is blocked neither dwarf1 or troll1 have moved.
		CHECK(actors.getLocation(dwarf1) == origin1);
		CHECK(actors.getLocation(troll1) == origin2);
		CHECK(actors.move_getRetries(dwarf1) == 0);
		CHECK(!actors.move_hasEvent(dwarf1));
		CHECK(actors.move_hasPathRequest(dwarf1));
	}
}