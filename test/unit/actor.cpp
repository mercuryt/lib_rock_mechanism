#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/materialType.h"
#include "../../engine/itemType.h"
#include "../../engine/animalSpecies.h"
TEST_CASE("actor")
{
	Step step = DateTime(10, 60, 10000).toSteps();
	Simulation simulation(L"", step);
	static AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	static AnimalSpeciesId troll = AnimalSpecies::byName(L"troll");
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	BlockIndex origin1 = blocks.getIndex_i(5, 5, 1);
	BlockIndex origin2 = blocks.getIndex_i(7, 7, 1);
	BlockIndex block1 = blocks.getIndex_i(6, 7, 1);
	BlockIndex block2 = blocks.getIndex_i(7, 8, 1);
	BlockIndex block3 = blocks.getIndex_i(6, 8, 1);
	SUBCASE("single tile")
	{
		int previousEventCount = area.m_eventSchedule.count();
		ActorIndex dwarf1 = actors.create(ActorParamaters{
			.species=dwarf,
			.percentGrown=Percent::create(100),
			.location=origin1,
		});
		CHECK(actors.getName(dwarf1) == L"dwarf1");
		CHECK(actors.grow_getPercent(dwarf1) == 100);
		CHECK(!actors.grow_isGrowing(dwarf1));
		CHECK(Shape::getName(actors.getShape(dwarf1)) == L"oneByOneFull");
		CHECK(area.m_eventSchedule.count() - previousEventCount == 3);
		CHECK(actors.getLocation(dwarf1) == origin1);
		CHECK(blocks.actor_contains(actors.getLocation(dwarf1), dwarf1));
		CHECK(actors.combat_getCombatScore(dwarf1) != 0);
	}
	SUBCASE("multi tile")
	{
		// Multi tile.
		ActorIndex troll1 = actors.create(ActorParamaters{
			.species=troll,
			.percentGrown=Percent::create(100),
			.location=origin2,
		});
		CHECK(blocks.actor_contains(origin2, troll1));
		CHECK(blocks.actor_contains(block1, troll1));
		CHECK(blocks.actor_contains(block2, troll1));
		CHECK(blocks.actor_contains(block3, troll1));
	}
}
