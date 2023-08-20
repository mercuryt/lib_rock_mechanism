#include "../../lib/doctest.h"
#include "../../src/simulation.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/actor.h"
TEST_CASE("actor")
{
	simulation::init();
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const AnimalSpecies& troll = AnimalSpecies::byName("troll");
	static const MaterialType& marble = MaterialType::byName("marble");
	Area area(10,10,10,280);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Block& origin1 = area.m_blocks[5][5][1];
	Block& origin2 = area.m_blocks[7][7][1];
	Block& block1 = area.m_blocks[6][7][1];
	Block& block2 = area.m_blocks[7][8][1];
	Block& block3 = area.m_blocks[6][8][1];
	// Single tile.
	Actor& dwarf1 = Actor::create(dwarf, origin1);
	CHECK(dwarf1.m_name == L"dwarf1");
	CHECK(dwarf1.m_canMove.getMoveSpeed() == 7);
	CHECK(eventSchedule::count() == 3);
	CHECK(dwarf1.m_location == &origin1);
	CHECK(dwarf1.m_location->m_hasShapes.contains(dwarf1));
	// Multi tile.
	Actor& troll1 = Actor::create(troll, origin2);
	CHECK(origin2.m_hasShapes.contains(troll1));
	CHECK(block1.m_hasShapes.contains(troll1));
	CHECK(block2.m_hasShapes.contains(troll1));
	CHECK(block3.m_hasShapes.contains(troll1));
}
