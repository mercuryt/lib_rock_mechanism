#include "locationBuckets.h"
#include "actor.h"
#include "area.h"
#include "config.h"
#include "types.h"
#include <algorithm>
LocationBuckets::LocationBuckets(Area& area) : m_area(area)
{
	m_maxX = ((m_area.m_sizeX - 1) / Config::locationBucketSize) + 1;
	m_maxY = ((m_area.m_sizeY - 1) / Config::locationBucketSize) + 1;
	m_maxZ = ((m_area.m_sizeZ - 1) / Config::locationBucketSize) + 1;
	m_buckets.resize(m_maxX * m_maxY * m_maxZ);
}
std::unordered_set<Actor*>& LocationBuckets::get(DistanceInBuckets x, DistanceInBuckets y, DistanceInBuckets z)
{
	return m_buckets[x + (y * m_maxX) + (z * m_maxX * m_maxY)];
}
const std::unordered_set<Actor*>& LocationBuckets::get(DistanceInBuckets x, DistanceInBuckets y, DistanceInBuckets z) const
{
	return m_buckets[x + (y * m_maxX) + (z * m_maxX * m_maxY)];
}
std::unordered_set<Actor*>& LocationBuckets::getBucketFor(const Block& block)
{
	DistanceInBuckets x = block.m_x / Config::locationBucketSize;
	DistanceInBuckets y = block.m_y / Config::locationBucketSize;
	DistanceInBuckets z = block.m_z / Config::locationBucketSize;
	return get(x, y, z);
}
void LocationBuckets::add(Actor& actor)
{
	assert(!actor.m_blocks.empty());
	for(Block* block : actor.m_blocks)
		block->m_locationBucket->insert(&actor);
}
void LocationBuckets::remove(Actor& actor)
{
	assert(!actor.m_blocks.empty());
	for(Block* block : actor.m_blocks)
		block->m_locationBucket->erase(&actor);
}
void LocationBuckets::update(Actor& actor, std::unordered_set<Block*>& oldBlocks)
{
	std::unordered_set<std::unordered_set<Actor*>*> oldSets;
	std::unordered_set<std::unordered_set<Actor*>*> continuedSets;
	for(const Block* block : oldBlocks)
		oldSets.insert(block->m_locationBucket);
	for(const Block* block : actor.m_blocks)
	{
		auto set = block->m_locationBucket;
		if(oldSets.contains(set))
			continuedSets.insert(set);
		else
			set->insert(&actor);
	}
	for(auto& set : oldSets)
		if(!continuedSets.contains(set))
			set->erase(&actor);
}
