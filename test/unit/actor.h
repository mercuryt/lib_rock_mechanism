#include "../../lib/doctest.h"
#include "../../src/simulation.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/actor.h"
TEST_CASE("actor")
{
	Simulation simulation;
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const AnimalSpecies& troll = AnimalSpecies::byName("troll");
	static const MaterialType& marble = MaterialType::byName("marble");
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Block& origin1 = area.m_blocks[5][5][1];
	Block& origin2 = area.m_blocks[7][7][1];
	Block& block1 = area.m_blocks[6][7][1];
	Block& block2 = area.m_blocks[7][8][1];
	Block& block3 = area.m_blocks[6][8][1];
	// Single tile.
	REQUIRE(simulation.m_eventSchedule.count() == 1);
	Actor& dwarf1 = simulation.createActor(dwarf, origin1);
	REQUIRE(dwarf1.m_name == L"dwarf1");
	REQUIRE(dwarf1.m_canMove.getMoveSpeed() == 7);
	REQUIRE(simulation.m_eventSchedule.count() == 4);
	REQUIRE(dwarf1.m_location == &origin1);
	REQUIRE(dwarf1.m_location->m_hasShapes.contains(dwarf1));
	REQUIRE(dwarf1.m_canFight.getCombatScore() != 0);
	// Multi tile.
	Actor& troll1 = simulation.createActor(troll, origin2);
	REQUIRE(origin2.m_hasShapes.contains(troll1));
	REQUIRE(block1.m_hasShapes.contains(troll1));
	REQUIRE(block2.m_hasShapes.contains(troll1));
	REQUIRE(block3.m_hasShapes.contains(troll1));
}
