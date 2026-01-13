#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/dataStructures/locationBuckets.h"
#include "../../engine/area/area.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/space/space.h"
#include "../../engine/actors/actors.h"
#include "../../engine/areaBuilderUtil.h"
TEST_CASE("octTree")
{
	const AnimalSpeciesId& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	const MaterialTypeId& granite = MaterialType::byName("granite");
	SUBCASE("basic")
	{
		Area& area = simulation.m_hasAreas->createArea(1,1,2);
		CHECK(area.m_octTree.getCount() == 1);
		Space& space = area.getSpace();
		Actors& actors = area.getActors();
		Point3D point = Point3D::create(0, 0, 1);
		space.solid_set(point.below(), granite, false);
		ActorIndex a1 = actors.create({.species=dwarf, .location=point});
		CHECK(area.m_octTree.contains(actors.getReference(a1), point));
		actors.location_clear(a1);
		CHECK(!area.m_octTree.contains(actors.getReference(a1), point));
	}
	SUBCASE("split and merge")
	{
		Area& area = simulation.m_hasAreas->createArea(10,10,10);
		Space& space = area.getSpace();
		Actors& actors = area.getActors();
		int toSpawn = Config::minimumOccupantsForOctTreeToSplit;
		areaBuilderUtil::setSolidLayer(area, 0, granite);
		Cuboid level = space.getZLevel(Distance::create(1));
		for(const Point3D& point : level)
		{
			if(toSpawn == 0)
				break;
			--toSpawn;
			actors.create({.species=dwarf, .location=point});
		}
		CHECK(area.m_octTree.getActorCount() == Config::minimumOccupantsForOctTreeToSplit);
		CHECK(area.m_octTree.getCount() == 9);
		CHECK(!space.actor_empty(Point3D::create(0, 0, 1)));
		int toUnspawn = Config::minimumOccupantsForOctTreeToSplit - Config::maximumOccupantsForOctTreeToMerge;
		for(const Point3D& point : level)
		{
			if(toUnspawn == 0)
				break;
			--toUnspawn;
			CHECK(!space.actor_empty(point));
			ActorIndex actor = space.actor_getAll(point).front();
			CHECK(actor.exists());
			actors.location_clear(actor);
			CHECK(!area.m_octTree.contains(actors.getReference(actor), point));
		}
		CHECK(area.m_octTree.getActorCount() == Config::maximumOccupantsForOctTreeToMerge);
		CHECK(area.m_octTree.getCount() == 1);
	}
}