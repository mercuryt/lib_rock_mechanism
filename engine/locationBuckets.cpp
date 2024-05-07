#include "locationBuckets.h"
#include "actor.h"
#include "area.h"
#include "config.h"
#include "types.h"
#include "util.h"
#include <algorithm>
size_t LocationBucket::indexFor(Actor& actor) const
{
	auto iter = std::ranges::find(m_actors, &actor);
	assert(iter != m_actors.end());
	return iter - m_actors.begin();
}
void LocationBucket::insert(Actor& actor, std::vector<Block*>&& blocks)
{
	m_actors.push_back(&actor);
	m_blocks.push_back(std::move(blocks));
}
void LocationBucket::erase(Actor& actor)
{
	size_t index = indexFor(actor);
	util::removeFromVectorByIndexUnordered(m_actors, index);
	util::removeFromVectorByIndexUnordered(m_blocks, index);
}
void LocationBucket::update(Actor& actor, std::vector<Block*>&& blocks)
{
	size_t index = indexFor(actor);
	m_blocks.at(index) = std::move(blocks);
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
const LocationBucket& LocationBuckets::get(DistanceInBuckets x, DistanceInBuckets y, DistanceInBuckets z) const
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
Actor* LocationBucket::getActor(size_t index)
{
	assert(m_actors.size() > index);
	assert(m_blocks.size() == m_actors.size());
	return m_actors.at(index);
}
std::vector<Block*>& LocationBucket::getBlocks(size_t index)
{
	assert(m_blocks.size() > index);
	assert(m_blocks.size() == m_actors.size());
	return m_blocks.at(index);
}
const Actor* LocationBucket::getActor(size_t index) const
{
	return const_cast<const Actor*>(const_cast<LocationBucket*>(this)->getActor(index));
}
const std::vector<Block*>& LocationBucket::getBlocks(size_t index) const
{
	return const_cast<const std::vector<Block*>&>(const_cast<LocationBucket*>(this)->getBlocks(index));
}
void LocationBuckets::add(Actor& actor)
{
	assert(!actor.m_blocks.empty());
	std::unordered_map<LocationBucket*, std::vector<Block*>> blocksCollatedByBucket;
	for(Block* block : actor.m_blocks)
		blocksCollatedByBucket[block->m_locationBucket].push_back(block);
	for(auto& [bucket, blocks] : blocksCollatedByBucket)
		bucket->insert(actor, std::move(blocks));
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
	for(const Block* block : oldBlocks)
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
		set->update(actor, std::move(blocks));
	for(auto& [set, blocks] : newSets)
		set->insert(actor, std::move(blocks));
}
