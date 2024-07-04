#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/attributes.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/actors/actors.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/config.h"

TEST_CASE("attributes")
{
	const AnimalSpecies& goblin = AnimalSpecies::byName("goblin");
	const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	SUBCASE("basic")
	{
		Attributes attributes(goblin, 100);
		REQUIRE(attributes.getStrength() == goblin.strength[1]);
	}
	SUBCASE("move speed")
	{
		Simulation simulation;
		Area& area = simulation.m_hasAreas->createArea(10,10,10);
		Blocks& blocks = area.getBlocks();
		Actors& actors = area.getActors();
		areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("marble"));

		ActorIndex goblin1 = actors.create(ActorParamaters{
			.species = goblin,
			.location = blocks.getIndex({5,5,1}),
		});

		ActorIndex dwarf1 = actors.create(ActorParamaters{
			.species = dwarf,
			.location = blocks.getIndex({5,7,1}),
		});
		REQUIRE(actors.getAgility(goblin1) > actors.getAgility(dwarf1));
		REQUIRE(actors.move_getSpeed(goblin1) > actors.move_getSpeed(dwarf1));
		BlockIndex adjacent = blocks.getIndex({5,6,1});
		Step delayGoblin = actors.move_delayToMoveInto(goblin1, adjacent);
		Step delayDwarf = actors.move_delayToMoveInto(dwarf1, adjacent);
		REQUIRE(delayGoblin < delayDwarf);
	}
}
