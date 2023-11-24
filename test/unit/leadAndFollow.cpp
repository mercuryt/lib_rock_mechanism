#include "../../lib/doctest.h"
#include "../../src/simulation.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/actor.h"
#include "../../src/station.h"
TEST_CASE("leadAndFollow")
{
	Simulation simulation;
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const AnimalSpecies& troll = AnimalSpecies::byName("troll");
	static const MaterialType& marble = MaterialType::byName("marble");
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	SUBCASE("two dwarves")
	{
		Block& origin1 = area.m_blocks[2][2][1];
		Block& origin2 = area.m_blocks[1][1][1];
		Block& destination1 = area.m_blocks[9][9][1];
		Block& destination2 = area.m_blocks[8][8][1];
		Actor& dwarf1 = simulation.createActor(dwarf, origin1);
		Actor& dwarf2 = simulation.createActor(dwarf, origin2);
		dwarf2.m_canFollow.follow(dwarf1.m_canLead);
		REQUIRE(dwarf1.m_canLead.getMoveSpeed() == dwarf1.m_canMove.getMoveSpeed());
		dwarf1.m_canMove.setDestination(destination1);
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getPath().size() == 7);
		REQUIRE(dwarf1.m_canMove.getDestination() == &destination1);
		while(dwarf1.m_location != &destination1)
			simulation.doStep();
		REQUIRE(dwarf2.m_location == &destination2);
	}
	SUBCASE("dwarf leads troll")
	{
		Block& origin1 = area.m_blocks[3][3][1];
		Block& origin2 = area.m_blocks[2][2][1];
		Block& destination1 = area.m_blocks[9][9][1];
		Block& destination2 = area.m_blocks[8][8][1];
		Actor& dwarf1 = simulation.createActor(dwarf, origin1);
		Actor& troll1 = simulation.createActor(troll, origin2);
		troll1.m_canFollow.follow(dwarf1.m_canLead);
		REQUIRE(dwarf1.m_canLead.getMoveSpeed() == dwarf1.m_canMove.getMoveSpeed());
		dwarf1.m_canMove.setDestination(destination1);
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getPath().size() == 6);
		REQUIRE(dwarf1.m_canMove.getDestination() == &destination1);
		while(dwarf1.m_location != &destination1)
			simulation.doStep();
		REQUIRE(troll1.m_location == &destination2);
	}
	SUBCASE("troll leads dwarf")
	{
		Block& origin1 = area.m_blocks[3][2][1];
		Block& origin2 = area.m_blocks[1][1][1];
		Block& destination1 = area.m_blocks[8][8][1];
		Block& destination2 = area.m_blocks[7][6][1];
		Actor& troll1 = simulation.createActor(troll, origin1);
		Actor& dwarf1 = simulation.createActor(dwarf, origin2);
		dwarf1.m_canFollow.follow(troll1.m_canLead);
		REQUIRE(troll1.m_canLead.getMoveSpeed() == troll1.m_canMove.getMoveSpeed());
		troll1.m_canMove.setDestination(destination1);
		simulation.doStep();
		REQUIRE(troll1.m_canMove.getPath().size() == 6);
		REQUIRE(troll1.m_canMove.getDestination() == &destination1);
		while(troll1.m_location != &destination1)
			simulation.doStep();
		REQUIRE(dwarf1.m_location == &destination2);
	}
	SUBCASE("use largest shape for pathing")
	{
		Block& origin1 = area.m_blocks[2][2][1];
		Block& origin2 = area.m_blocks[1][1][1];
		Block& destination1 = area.m_blocks[9][9][1];
		area.m_blocks[5][1][1].setSolid(marble);
		area.m_blocks[5][3][1].setSolid(marble);
		area.m_blocks[5][5][1].setSolid(marble);
		area.m_blocks[5][7][1].setSolid(marble);
		Actor& dwarf1 = simulation.createActor(AnimalSpecies::byName("dwarf"), origin1);
		Actor& troll1 = simulation.createActor(troll, origin2);
		troll1.m_canFollow.follow(dwarf1.m_canLead);
		dwarf1.m_canMove.setDestination(destination1);
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getPath().size() == 8);
	}
	SUBCASE("wait for follower to keep up if blocked temporarily")
	{
		Block& origin1 = area.m_blocks[2][2][1];
		Block& origin2 = area.m_blocks[1][1][1];
		Block& origin3 = area.m_blocks[3][2][1];
		Block& destination1 = area.m_blocks[9][9][1];
		Block& destination2 = area.m_blocks[8][8][1];
		Actor& dwarf1 = simulation.createActor(dwarf, origin1);
		Actor& troll1 = simulation.createActor(troll, origin2);
		Actor& dwarf2 = simulation.createActor(dwarf, origin3);
		std::unique_ptr<Objective> stationObjective = std::make_unique<StationObjective>(dwarf2, origin3);
		dwarf2.m_hasObjectives.addTaskToStart(std::move(stationObjective));
		troll1.m_canFollow.follow(dwarf1.m_canLead);
		dwarf1.m_canMove.setDestination(destination1);
		area.m_hasActors.add(dwarf1);
		area.m_hasActors.add(dwarf2);
		area.m_hasActors.add(troll1);
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getPath().size() == 7);
		while(dwarf1.m_location == &origin1)
			simulation.doStep();
		Block* dwarf1FirstStep = dwarf1.m_location;
		REQUIRE(troll1.m_location == &origin2);
		REQUIRE(!origin1.m_hasShapes.canEnterCurrentlyFrom(troll1, *troll1.m_location));
		dwarf2.setLocation(area.m_blocks[9][1][1]);
		REQUIRE(origin1.m_hasShapes.canEnterCurrentlyFrom(troll1, *troll1.m_location));
		simulation.fastForwardUntillActorIsAt(troll1, origin1);
		REQUIRE(dwarf1.m_location == dwarf1FirstStep);
		while(dwarf1.m_location != &destination1)
			simulation.doStep();
		REQUIRE(troll1.m_location == &destination2);
	}
	SUBCASE("disband line if follower blocked permanantly")
	{
		Block& origin1 = area.m_blocks[2][2][1];
		Block& origin2 = area.m_blocks[1][1][1];
		Block& destination1 = area.m_blocks[9][9][1];
		Actor& dwarf1 = simulation.createActor(dwarf, origin1);
		Actor& troll1 = simulation.createActor(troll, origin2);
		troll1.m_canFollow.follow(dwarf1.m_canLead);
		dwarf1.m_canMove.setDestination(destination1);
		simulation.doStep();
		area.m_blocks[2][1][1].setSolid(marble);
		while(dwarf1.m_location == &origin1)
			simulation.doStep();
		REQUIRE(!dwarf1.m_canLead.isLeading());
		REQUIRE(!troll1.m_canFollow.isFollowing());
	}
}
