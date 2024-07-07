#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/locationBuckets.h"
#include "../../engine/area.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/actors/actors.h"
TEST_CASE("locationBuckets")
{
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	SUBCASE("basic")
	{
		assert(Config::locationBucketSize >= 6);
		Area& area = simulation.m_hasAreas->createArea(10,10,10);
		Blocks& blocks = area.getBlocks();
		Actors& actors = area.getActors();
		BlockIndex block = blocks.getIndex(5, 5, 5);
		ActorIndex a1 = actors.create({.species=dwarf, .location=block});
		auto&  actorsInBucket = blocks.getLocationBucket(block).m_actorsSingleTile;
		auto found = std::ranges::find(actorsInBucket, a1);
		REQUIRE(found != actorsInBucket.end());
		for(BlockIndex adjacent : blocks.getDirectlyAdjacent(block))
			if(adjacent != BLOCK_DISTANCE_MAX)
				REQUIRE(&blocks.getLocationBucket(block) == &blocks.getLocationBucket(adjacent));
		/*
		for(ActorIndex a : area.m_hasActors.m_locationBuckets.inRange(block, 10))
			REQUIRE(&a == &a1);
		*/
	}
	SUBCASE("locationBuckets out of range")
	{
		//TODO: fix or delete this test.
		/*
		assert(Config::locationBucketSize <= 25);
		Simulation simulation;
		Area& area = simulation.m_hasAreas->createArea(30,30,30);
		Blocks& blocks = area.getBlocks();
		BlockIndex b1 = blocks.getIndex(0, 0, 1);
		Actors& actors = area.getActors();
		ActorIndex a1 = actors.create({.species=dwarf, .location=b1});
		BlockIndex b2 = blocks.getIndex(15, 1, 1);
		uint32_t actorCount = 0;
		for(ActorIndex actor : area.m_locationBuckets.inRange(b2, 15))
		{
			(void)actor;
			++actorCount;
		}
		REQUIRE(actorCount == 0);
		BlockIndex b3 = blocks.getIndex(14, 1, 1);
		actorCount = 0;
		for(ActorIndex actor : area.m_locationBuckets.inRange(b3, 15))
		{
			(void)actor;
			++actorCount;
		}
		REQUIRE(actorCount == 1);
		*/
	}
}
