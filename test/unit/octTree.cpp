#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/dataStructures/locationBuckets.h"
#include "../../engine/area/area.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/blocks/blocks.h"
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
		Blocks& blocks = area.getSpace();
		Actors& actors = area.getActors();
		BlockIndex block = blocks.getIndex_i(0, 0, 1);
		blocks.solid_set(blocks.getBlockBelow(block), granite, false);
		ActorIndex a1 = actors.create({.species=dwarf, .location=block});
		CHECK(area.m_octTree.contains(actors.getReference(a1), blocks.getCoordinates(block)));
		actors.location_clear(a1);
		CHECK(!area.m_octTree.contains(actors.getReference(a1), blocks.getCoordinates(block)));
	}
	SUBCASE("split and merge")
	{
		Area& area = simulation.m_hasAreas->createArea(10,10,10);
		Blocks& blocks = area.getSpace();
		Actors& actors = area.getActors();
		uint toSpawn = Config::minimumOccupantsForOctTreeToSplit;
		areaBuilderUtil::setSolidLayer(area, 0, granite);
		Cuboid level = blocks.getZLevel(DistanceInBlocks::create(1));
		for(const BlockIndex& block : level.getView(blocks))
		{
			if(toSpawn == 0)
				break;
			--toSpawn;
			actors.create({.species=dwarf, .location=block});
		}
		CHECK(area.m_octTree.getActorCount() == Config::minimumOccupantsForOctTreeToSplit);
		CHECK(area.m_octTree.getCount() == 9);
		CHECK(!blocks.actor_empty(blocks.getIndex_i(0, 0, 1)));
		uint toUnspawn = Config::minimumOccupantsForOctTreeToSplit - Config::maximumOccupantsForOctTreeToMerge;
		for(const BlockIndex& block : level.getView(blocks))
		{
			if(toUnspawn == 0)
				break;
			--toUnspawn;
			CHECK(!blocks.actor_empty(block));
			ActorIndex actor = blocks.actor_getAll(block).front();
			CHECK(actor.exists());
			actors.location_clear(actor);
			CHECK(!area.m_octTree.contains(actors.getReference(actor), blocks.getCoordinates(block)));
		}
		CHECK(area.m_octTree.getActorCount() == Config::maximumOccupantsForOctTreeToMerge);
		CHECK(area.m_octTree.getCount() == 1);
	}
}