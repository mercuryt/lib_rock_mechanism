#include "../../lib/doctest.h"
#include "../../src/locationBuckets.h"
#include "../../src/area.h"
#include "../../src/block.h"
TEST_CASE("locationBuckets")
{
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	SUBCASE("basic")
	{
		assert(Config::locationBucketSize >= 6);
		Area& area = simulation.createArea(10,10,10);
		Block& block = area.m_blocks[5][5][5];
		CHECK(block.m_locationBucket != nullptr);
		Actor& a1 = simulation.createActor(dwarf, block);
		CHECK(block.m_locationBucket->contains(&a1));
		for(Block* adjacent : block.m_adjacentsVector)
			CHECK(block.m_locationBucket == adjacent->m_locationBucket);
		for(Actor& a : area.m_hasActors.m_locationBuckets.inRange(block, 10))
			CHECK(&a == &a1);
	}
	SUBCASE("locationBuckets out of range")
	{
		assert(Config::locationBucketSize <= 25);
		Simulation simulation;
		Area& area = simulation.createArea(30,30,30);
		Block& b1 = area.m_blocks[0][0][1];
		simulation.createActor(dwarf, b1);
		Block& b2 = area.m_blocks[15][1][1];
		uint32_t actors = 0;
		for(Actor& actor : area.m_hasActors.m_locationBuckets.inRange(b2, 15))
		{
			(void)actor;
			++actors;
		}
		CHECK(actors == 0);
		Block& b3 = area.m_blocks[14][1][1];
		actors = 0;
		for(Actor& actor : area.m_hasActors.m_locationBuckets.inRange(b3, 15))
		{
			(void)actor;
			++actors;
		}
		CHECK(actors == 1);
	}
}
