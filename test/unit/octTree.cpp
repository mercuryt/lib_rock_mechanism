#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/locationBuckets.h"
#include "../../engine/area/area.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/actors/actors.h"
TEST_CASE("octTree")
{
	static AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	Simulation simulation;
	SUBCASE("basic")
	{
		Area& area = simulation.m_hasAreas->createArea(20,20,20);
		CHECK(area.m_octTree.getCount() == 1);
		Blocks& blocks = area.getBlocks();
		Actors& actors = area.getActors();
		BlockIndex block = blocks.getIndex_i(5, 5, 5);
		ActorIndex a1 = actors.create({.species=dwarf, .location=block});
		CHECK(area.m_octTree.contains(actors.getReference(a1), blocks.getCoordinates(block)));
		actors.exit(a1);
		CHECK(!area.m_octTree.contains(actors.getReference(a1), blocks.getCoordinates(block)));
	}
	SUBCASE("split and merge")
	{
		Area& area = simulation.m_hasAreas->createArea(10,10,10);
		Blocks& blocks = area.getBlocks();
		Actors& actors = area.getActors();
		uint toSpawn = Config::minimumOccupantsForOctTreeToSplit;
		for(const BlockIndex& block : blocks.getAllIndices())
		{
			if(toSpawn == 0)
				break;
			--toSpawn;
			actors.create({.species=dwarf, .location=block});
		}
		CHECK(area.m_octTree.getActorCount() == Config::minimumOccupantsForOctTreeToSplit);
		CHECK(area.m_octTree.getCount() == 9);
		CHECK(!blocks.actor_empty(BlockIndex::create(0)));
		uint toUnspawn = Config::minimumOccupantsForOctTreeToSplit - Config::maximumOccupantsForOctTreeToMerge;
		for(const BlockIndex& block : blocks.getAllIndices())
		{
			if(toUnspawn == 0)
				break;
			--toUnspawn;
			CHECK(!blocks.actor_empty(block));
			ActorIndex actor = blocks.actor_getAll(block).front();
			CHECK(actor.exists());
			actors.exit(actor);
			CHECK(!area.m_octTree.contains(actors.getReference(actor), blocks.getCoordinates(block)));
		}
		CHECK(area.m_octTree.getActorCount() == Config::maximumOccupantsForOctTreeToMerge);
		CHECK(area.m_octTree.getCount() == 1);
	}
}