#include "locationBuckets.h"
#include "actor.h"
#include "area.h"
#include "config.h"
#include "types.h"
#include "util.h"
#include <algorithm>
void LocationBucket::insert(Actor& actor, std::vector<Block*>& blocks)
{
	if(actor.m_shape->isMultiTile)
	{
		[[maybe_unused]] auto iter = std::ranges::find(m_actorsMultiTile, &actor);
		assert(iter == m_actorsMultiTile.end());
		m_actorsMultiTile.push_back(&actor);
		m_blocksMultiTileActors.push_back(blocks);
	}
	else 
	{
		assert(blocks.size() == 1);
		[[maybe_unused]] auto iter = std::ranges::find(m_actorsSingleTile, &actor);
		assert(iter == m_actorsSingleTile.end());
		m_actorsSingleTile.push_back(&actor);
		m_blocksSingleTileActors.push_back(blocks.front());
	}
}
void LocationBucket::erase(Actor& actor)
{
	if(actor.m_shape->isMultiTile)
	{
		auto iter = std::ranges::find(m_actorsMultiTile, &actor);
		assert(iter != m_actorsMultiTile.end());
		size_t index = iter - m_actorsMultiTile.begin();
		util::removeFromVectorByIndexUnordered(m_actorsMultiTile, index);
		util::removeFromVectorByIndexUnordered(m_blocksMultiTileActors, index);
	}
	else 
	{
		auto iter = std::ranges::find(m_actorsSingleTile, &actor);
		assert(iter != m_actorsSingleTile.end());
		size_t index = iter - m_actorsSingleTile.begin();
		util::removeFromVectorByIndexUnordered(m_actorsSingleTile, index);
		util::removeFromVectorByIndexUnordered(m_blocksSingleTileActors, index);
	}
}
void LocationBucket::update(Actor& actor, std::vector<Block*>& blocks)
{

	if(actor.m_shape->isMultiTile)
	{
		auto iter = std::ranges::find(m_actorsMultiTile, &actor);
		assert(iter != m_actorsMultiTile.end());
		size_t index = iter - m_actorsMultiTile.begin();
		m_blocksMultiTileActors.at(index) = blocks;
	}
	else 
	{
		assert(blocks.size() == 1);
		auto iter = std::ranges::find(m_actorsSingleTile, &actor);
		assert(iter != m_actorsSingleTile.end());
		size_t index = iter - m_actorsSingleTile.begin();
		m_blocksSingleTileActors.at(index) = blocks.front();
	}
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
