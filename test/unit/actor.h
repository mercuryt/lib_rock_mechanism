#include "../../lib/doctest.h"
#include "../../src/simulation.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/actor.h"
TEST_CASE("actor")
{
	simulation::init();
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const MaterialType& marble = MaterialType::byName("marble");
	Area area(10,10,10,280);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Actor& actor = Actor::create(dwarf, area.m_blocks[5][5][1]);
	CHECK(actor.m_name == L"dwarf1");
}
