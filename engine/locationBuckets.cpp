#include "locationBuckets.h"
#include "actor.h"
#include "area.h"
#include "config.h"
#include "types.h"
#include "util.h"
#include <algorithm>
void LocationBucket::insert(Actor& actor, std::vector<Block*>& blocks)
{
	auto iter = std::ranges::find(data, &actor, &std::pair<Actor*, std::vector<Block*>>::first);
	assert(iter == data.end());
	data.emplace_back(&actor, blocks);
}
void LocationBucket::erase(Actor& actor)
{
	auto iter = std::ranges::find(data, &actor, &std::pair<Actor*, std::vector<Block*>>::first);
	assert(iter != data.end());
	std::swap(*iter, data.back());
	data.pop_back();
}
void LocationBucket::update(Actor& actor, std::vector<Block*>& blocks)
{
	auto iter = std::ranges::find(data, &actor, &std::pair<Actor*, std::vector<Block*>>::first);
	assert(iter != data.end());
	iter->second = blocks;
}
LocationBuckets::LocationBuckets(Area& area) : m_area(area)
{
	m_maxX = ((m_area.m_sizeX - 1) / Config::locationBucketSize) + 1;
	m_maxY = ((m_area.m_sizeY - 1) / Config::locationBucketSize) + 1;
	m_maxZ = ((m_area.m_sizeZ - 1) / Config::locationBucketSize) + 1;
	m_buckets.resize(m_maxX * m_maxY * m_maxZ);
}
LocationBucket& LocationBuckets::get(DistanceInBuckets x, DistanceInBuckets y, DistanceInBuckets z)
{
	return m_buckets[x + (y * m_maxX) + (z * m_maxX * m_maxY)];
}
LocationBucket& LocationBuckets::getBucketFor(const Block& block)
{
	DistanceInBuckets x = block.m_x / Config::locationBucketSize;
	DistanceInBuckets y = block.m_y / Config::locationBucketSize;
	DistanceInBuckets z = block.m_z / Config::locationBucketSize;
	return get(x, y, z);
}
void LocationBuckets::add(Actor& actor)
{
	assert(!actor.m_blocks.empty());
	std::unordered_map<LocationBucket*, std::vector<Block*>> blocksCollatedByBucket;
	for(Block* block : actor.m_blocks)
		blocksCollatedByBucket[block->m_locationBucket].push_back(block);
	for(auto& [bucket, blocks] : blocksCollatedByBucket)
		bucket->insert(actor, blocks);
}
void LocationBuckets::remove(Actor& actor)
{
	assert(!actor.m_blocks.empty());
	std::unordered_set<LocationBucket*> buckets;
	for(Block* block : actor.m_blocks)
		buckets.insert(block->m_locationBucket);
	for(LocationBucket* bucket : buckets)
		bucket->erase(actor);
}
void LocationBuckets::update(Actor& actor, std::unordered_set<Block*>& oldBlocks)
{
	std::unordered_set<LocationBucket*> oldSets;
	std::unordered_map<LocationBucket*, std::vector<Block*>> continuedSets;
	std::unordered_map<LocationBucket*, std::vector<Block*>> newSets;
	for(Block* block : oldBlocks)
		oldSets.insert(block->m_locationBucket);
	for(Block* block : actor.m_blocks)
	{
		auto set = block->m_locationBucket;
		if(oldSets.contains(set))
			continuedSets[set].push_back(block);
		else
			newSets[set].push_back(block);
	}
	for(auto& set : oldSets)
		if(!continuedSets.contains(set))
			set->erase(actor);
	for(auto& [set, blocks] : continuedSets)
		set->update(actor, blocks);
	for(auto& [set, blocks] : newSets)
		set->insert(actor, blocks);
}
