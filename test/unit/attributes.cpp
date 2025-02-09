#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/actors/actors.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/config.h"

TEST_CASE("attributes")
{
	AnimalSpeciesId goblin = AnimalSpecies::byName(L"goblin");
	AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName(L"marble"));

	ActorIndex goblin1 = actors.create(ActorParamaters{
		.species = goblin,
		.percentGrown=Percent::create(100),
		.location = blocks.getIndex_i(5,5,1),
	});
	SUBCASE("basic")
	{
		CHECK(actors.getStrength(goblin1) == AnimalSpecies::getStrength(goblin)[1]);
	}
	SUBCASE("move speed")
	{

		ActorIndex dwarf1 = actors.create(ActorParamaters{
			.species = dwarf,
			.location = blocks.getIndex_i(5,7,1),
		});
		CHECK(actors.getAgility(goblin1) > actors.getAgility(dwarf1));
		CHECK(actors.move_getSpeed(goblin1) > actors.move_getSpeed(dwarf1));
		BlockIndex adjacent = blocks.getIndex_i(5,6,1);
		Step delayGoblin = actors.move_delayToMoveInto(goblin1, adjacent);
		Step delayDwarf = actors.move_delayToMoveInto(dwarf1, adjacent);
		CHECK(delayGoblin < delayDwarf);
	}
}
