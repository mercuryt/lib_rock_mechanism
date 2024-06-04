#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/locationBuckets.h"
#include "../../engine/area.h"
TEST_CASE("locationBuckets")
{
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	SUBCASE("basic")
	{
		assert(Config::locationBucketSize >= 6);
		Area& area = simulation.m_hasAreas->createArea(10,10,10);
		Blocks& blocks = area.getBlocks();
		BlockIndex block = blocks.getIndex({5, 5, 5});
		Actor& a1 = simulation.m_hasActors->createActor(dwarf, block);
		auto&  actorsInBucket = blocks.getLocationBucket(block).m_actorsSingleTile;
		auto found = std::ranges::find(actorsInBucket, &a1);
		REQUIRE(found != actorsInBucket.end());
		for(BlockIndex adjacent : blocks.getDirectlyAdjacent(block))
			if(adjacent != BLOCK_DISTANCE_MAX)
				REQUIRE(&blocks.getLocationBucket(block) == &blocks.getLocationBucket(adjacent));
		/*
		for(Actor& a : area.m_hasActors.m_locationBuckets.inRange(block, 10))
			REQUIRE(&a == &a1);
		*/
	}
	SUBCASE("locationBuckets out of range")
	{
		assert(Config::locationBucketSize <= 25);
		Simulation simulation;
		Area& area = simulation.m_hasAreas->createArea(30,30,30);
		Blocks& blocks = area.getBlocks();
		BlockIndex b1 = blocks.getIndex({0, 0, 1});
		simulation.m_hasActors->createActor(dwarf, b1);
		/*
		BlockIndex b2 = blocks.getIndex({15, 1, 1});
		uint32_t actors = 0;
		for(Actor& actor : area.m_hasActors.m_locationBuckets.inRange(b2, 15))
		{
			(void)actor;
			++actors;
		}
		REQUIRE(actors == 0);
		BlockIndex b3 = blocks.getIndex({14, 1, 1});
		actors = 0;
		for(Actor& actor : area.m_hasActors.m_locationBuckets.inRange(b3, 15))
		{
			(void)actor;
			++actors;
		}
		REQUIRE(actors == 1);
		*/
	}
}
