#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/item.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/medical.h"
#include <functional>
TEST_CASE("medical")
{
	const MaterialType& marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Faction faction(L"tower of power");
	area.m_hasStockPiles.addFaction(faction);
	Actor& dwarf1 = simulation.m_hasActors->createActor(AnimalSpecies::byName("dwarf"), area.getBlock(1, 1, 1));
	dwarf1.setFaction(&faction);
}
