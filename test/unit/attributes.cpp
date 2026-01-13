#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/actors/actors.h"
#include "../../engine/space/space.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/config/config.h"

TEST_CASE("attributes")
{
	AnimalSpeciesId goblin = AnimalSpecies::byName("goblin");
	AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Actors& actors = area.getActors();
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("marble"));

	ActorIndex goblin1 = actors.create(ActorParamaters{
		.species = goblin,
		.percentGrown=Percent::create(100),
		.location = Point3D::create(5,5,1),
	});
	SUBCASE("basic")
	{
		CHECK(actors.getStrength(goblin1) == AnimalSpecies::getStrength(goblin)[1]);
	}
	SUBCASE("move speed")
	{
		const Point3D& location = Point3D::create(5,7,1);
		ActorIndex dwarf1 = actors.create(ActorParamaters{
			.species = dwarf,
			.location = location,
		});
		CHECK(actors.getAgility(goblin1) > actors.getAgility(dwarf1));
		CHECK(actors.move_getSpeed(goblin1) > actors.move_getSpeed(dwarf1));
		Point3D adjacent = Point3D::create(5,6,1);
		Step delayGoblin = actors.move_delayToMoveInto(goblin1, location, adjacent);
		Step delayDwarf = actors.move_delayToMoveInto(dwarf1, location, adjacent);
		CHECK(delayGoblin < delayDwarf);
	}
}
