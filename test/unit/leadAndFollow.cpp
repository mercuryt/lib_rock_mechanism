#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actor.h"
#include "../../engine/objectives/station.h"
TEST_CASE("leadAndFollow")
{
	Simulation simulation;
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const AnimalSpecies& troll = AnimalSpecies::byName("troll");
	static const MaterialType& marble = MaterialType::byName("marble");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	SUBCASE("two dwarves")
	{
		BlockIndex origin1 = blocks.getIndex({2, 2, 1});
		BlockIndex origin2 = blocks.getIndex({1, 1, 1});
		BlockIndex destination1 = blocks.getIndex({9, 9, 1});
		BlockIndex destination2 = blocks.getIndex({8, 8, 1});
		Actor& dwarf1 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin1,
			.area=&area,
		});
		Actor& dwarf2 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin2,
			.area=&area,
		});
		dwarf2.m_canFollow.follow(dwarf1.m_canLead);
		REQUIRE(dwarf1.m_canLead.getMoveSpeed() == dwarf1.m_canMove.getMoveSpeed());
		dwarf1.m_canMove.setDestination(destination1);
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getPath().size() == 7);
		REQUIRE(dwarf1.m_canMove.getDestination() == destination1);
		while(dwarf1.m_location != destination1)
			simulation.doStep();
		REQUIRE(dwarf2.m_location == destination2);
	}
	SUBCASE("dwarf leads troll")
	{
		BlockIndex origin1 = blocks.getIndex({3, 3, 1});
		BlockIndex origin2 = blocks.getIndex({2, 2, 1});
		BlockIndex destination1 = blocks.getIndex({9, 9, 1});
		BlockIndex destination2 = blocks.getIndex({8, 8, 1});
		Actor& dwarf1 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin1,
			.area=&area,
		});
		Actor& troll1 = simulation.m_hasActors->createActor({
			.species=troll,
			.location=origin2,
			.area=&area,
		});
		troll1.m_canFollow.follow(dwarf1.m_canLead);
		REQUIRE(dwarf1.m_canLead.getMoveSpeed() == dwarf1.m_canMove.getMoveSpeed());
		dwarf1.m_canMove.setDestination(destination1);
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getPath().size() == 6);
		REQUIRE(dwarf1.m_canMove.getDestination() == destination1);
		while(dwarf1.m_location != destination1)
			simulation.doStep();
		REQUIRE(troll1.m_location == destination2);
	}
	SUBCASE("troll leads dwarf")
	{
		BlockIndex origin1 = blocks.getIndex({3, 2, 1});
		BlockIndex origin2 = blocks.getIndex({1, 1, 1});
		BlockIndex destination1 = blocks.getIndex({8, 8, 1});
		BlockIndex destination2 = blocks.getIndex({7, 6, 1});
		Actor& troll1 = simulation.m_hasActors->createActor({
			.species=troll,
			.location=origin1,
			.area=&area,
		});
		Actor& dwarf1 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin2,
			.area=&area,
		});
		dwarf1.m_canFollow.follow(troll1.m_canLead);
		REQUIRE(troll1.m_canLead.getMoveSpeed() == troll1.m_canMove.getMoveSpeed());
		troll1.m_canMove.setDestination(destination1);
		simulation.doStep();
		REQUIRE(troll1.m_canMove.getPath().size() == 6);
		REQUIRE(troll1.m_canMove.getDestination() == destination1);
		while(troll1.m_location != destination1)
			simulation.doStep();
		REQUIRE(dwarf1.m_location == destination2);
	}
	SUBCASE("use largest shape for pathing")
	{
		BlockIndex origin1 = blocks.getIndex({2, 2, 1});
		BlockIndex origin2 = blocks.getIndex({1, 1, 1});
		BlockIndex destination1 = blocks.getIndex({9, 9, 1});
		blocks.solid_set(blocks.getIndex({5, 1, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({5, 3, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({5, 5, 1}), marble, false);
		blocks.solid_set(blocks.getIndex({5, 7, 1}), marble, false);
		Actor& dwarf1 = simulation.m_hasActors->createActor({
			.species=AnimalSpecies::byName("dwarf"),
			.location=origin1,
			.area=&area,
		});
		Actor& troll1 = simulation.m_hasActors->createActor({
			.species=troll,
			.location=origin2,
			.area=&area,
		});
		troll1.m_canFollow.follow(dwarf1.m_canLead);
		dwarf1.m_canMove.setDestination(destination1);
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getPath().size() == 8);
	}
	SUBCASE("wait for follower to keep up if blocked temporarily")
	{
		BlockIndex origin1 = blocks.getIndex({2, 2, 1});
		BlockIndex origin2 = blocks.getIndex({1, 1, 1});
		BlockIndex origin3 = blocks.getIndex({3, 2, 1});
		BlockIndex destination1 = blocks.getIndex({9, 9, 1});
		BlockIndex destination2 = blocks.getIndex({8, 8, 1});
		Actor& dwarf1 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin1,
			.area=&area,
		});
		Actor& troll1 = simulation.m_hasActors->createActor({
			.species=troll,
			.location=origin2,
			.area=&area,
		});
		Actor& dwarf2 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin3,
			.area=&area,
		});
		std::unique_ptr<Objective> stationObjective = std::make_unique<StationObjective>(dwarf2, origin3);
		dwarf2.m_hasObjectives.addTaskToStart(std::move(stationObjective));
		troll1.m_canFollow.follow(dwarf1.m_canLead);
		dwarf1.m_canMove.setDestination(destination1);
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getPath().size() == 7);
		while(dwarf1.m_location == origin1)
			simulation.doStep();
		// Troll has not moved yet.
		REQUIRE(troll1.m_location == origin2);
		REQUIRE(!blocks.shape_canEnterCurrentlyFrom(origin1, troll1, troll1.m_location));
		dwarf2.setLocation(blocks.getIndex({9, 1, 1}));
		REQUIRE(blocks.shape_canEnterCurrentlyFrom(origin1, troll1, troll1.m_location));
		REQUIRE(troll1.m_canFollow.hasEvent());
		Step delay = troll1.m_canFollow.getEventStep() - simulation.m_step;
		simulation.fastForward(delay);
		REQUIRE(troll1.m_location != origin2);
		REQUIRE(troll1.isAdjacentTo(dwarf1));
		while(dwarf1.m_location != destination1)
			simulation.doStep();
		REQUIRE(troll1.m_location == destination2);
	}
	SUBCASE("disband line if follower blocked permanantly")
	{
		BlockIndex origin1 = blocks.getIndex({2, 2, 1});
		BlockIndex origin2 = blocks.getIndex({1, 1, 1});
		BlockIndex destination1 = blocks.getIndex({9, 9, 1});
		Actor& dwarf1 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=origin1,
			.area=&area,
		});
		Actor& troll1 = simulation.m_hasActors->createActor({
			.species=troll,
			.location=origin2,
			.area=&area,
		});
		troll1.m_canFollow.follow(dwarf1.m_canLead);
		dwarf1.m_canMove.setDestination(destination1);
		simulation.doStep();
		blocks.solid_set(blocks.getIndex({2, 1, 1}), marble, false);
		while(dwarf1.m_location == origin1)
			simulation.doStep();
		REQUIRE(!dwarf1.m_canLead.isLeading());
		REQUIRE(!troll1.m_canFollow.isFollowing());
	}
}
