#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/locationBuckets.h"
#include "../../engine/area.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/actors/actors.h"
TEST_CASE("octTree")
{
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	SUBCASE("basic")
	{
		Area& area = simulation.m_hasAreas->createArea(10,10,10);
		REQUIRE(area.m_octTree.getCount() == 1);
		Blocks& blocks = area.getBlocks();
		Actors& actors = area.getActors();
		BlockIndex block = blocks.getIndex_i(5, 5, 5);
		ActorIndex a1 = actors.create({.species=dwarf, .location=block});
		REQUIRE(area.m_octTree.contains(actors.getReference(a1), blocks.getCoordinates(block)));
		actors.exit(a1);
		REQUIRE(!area.m_octTree.contains(actors.getReference(a1), blocks.getCoordinates(block)));
	}
	SUBCASE("split and merge")
	{
		Area& area = simulation.m_hasAreas->createArea(10,10,10);
		Blocks& blocks = area.getBlocks();
		Actors& actors = area.getActors();
		uint toSpawn = Config::minimumOccupantsForOctTreeToSplit;
		for(const BlockIndex& block : blocks.getAll())
		{
			if(toSpawn == 0)
				break;
			--toSpawn;
			actors.create({.species=dwarf, .location=block});
		}
		REQUIRE(area.m_octTree.getActorCount() == Config::minimumOccupantsForOctTreeToSplit);
		REQUIRE(area.m_octTree.getCount() == 9);
		REQUIRE(!blocks.actor_empty(BlockIndex::create(0)));
		uint toUnspawn = Config::minimumOccupantsForOctTreeToSplit - Config::minimumOccupantsForOctTreeToUnsplit;
		for(const BlockIndex& block : blocks.getAll())
		{
			if(toUnspawn == 0)
				break;
			--toUnspawn;
			REQUIRE(!blocks.actor_empty(block));
			ActorIndex actor = blocks.actor_getAll(block).front();
			REQUIRE(actor.exists());
			actors.exit(actor);
			REQUIRE(!area.m_octTree.contains(actors.getReference(actor), blocks.getCoordinates(block)));
		}
		REQUIRE(area.m_octTree.getActorCount() == Config::minimumOccupantsForOctTreeToUnsplit);
		REQUIRE(area.m_octTree.getCount() == 1);
	}
}