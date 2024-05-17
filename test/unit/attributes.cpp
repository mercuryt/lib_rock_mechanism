#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/attributes.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/actor.h"
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
		areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("marble"));

		Actor& goblin1 = simulation.m_hasActors->createActor(ActorParamaters{
			.species = goblin,
			.location = &area.getBlock(5,5,1),
		});

		Actor& dwarf1 = simulation.m_hasActors->createActor(ActorParamaters{
			.species = dwarf,
			.location = &area.getBlock(5,7,1),
		});
		REQUIRE(goblin1.m_attributes.getAgility() > dwarf1.m_attributes.getAgility());
		REQUIRE(goblin1.m_attributes.getMoveSpeed() > dwarf1.m_attributes.getMoveSpeed());
		Block& adjacent = area.getBlock(5,6,1);
		Step delayGoblin = goblin1.m_canMove.delayToMoveInto(adjacent);
		Step delayDwarf = dwarf1.m_canMove.delayToMoveInto(adjacent);
		REQUIRE(delayGoblin < delayDwarf);
	}
}
