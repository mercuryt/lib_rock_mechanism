#include "../../lib/doctest.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/item.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"
#include "../../src/medical.h"
#include <functional>
TEST_CASE("medical")
{
	const MaterialType& marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Faction faction(L"tower of power");
	area.m_hasStockPiles.addFaction(faction);
	Actor& dwarf1 = simulation.createActor(AnimalSpecies::byName("dwarf"), area.getBlock(1, 1, 1));
	dwarf1.setFaction(&faction);
	area.m_hasActors.add(dwarf1);
}
