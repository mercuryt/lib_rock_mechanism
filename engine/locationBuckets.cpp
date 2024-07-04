#include "locationBuckets.h"
#include "area.h"
#include "config.h"
#include "types.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "util.h"
#include <algorithm>
void LocationBucket::insert(Area& area, ActorIndex actor, std::vector<BlockIndex>& blocks)
{
	Actors& actors = area.getActors();
	if(actors.getShape(actor).isMultiTile)
	{
		[[maybe_unused]] auto iter = std::ranges::find(m_actorsMultiTile, actor);
		assert(iter == m_actorsMultiTile.end());
		m_actorsMultiTile.push_back(actor);
		m_blocksMultiTileActors.push_back(blocks);
	}
	else
	{
		assert(blocks.size() == 1);
		[[maybe_unused]] auto iter = std::ranges::find(m_actorsSingleTile, actor);
		assert(iter == m_actorsSingleTile.end());
		m_actorsSingleTile.push_back(actor);
		m_blocksSingleTileActors.push_back(blocks.front());
	}
}
void LocationBucket::erase(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(actors.getShape(actor).isMultiTile)
	{
		auto iter = std::ranges::find(m_actorsMultiTile, actor);
		assert(iter != m_actorsMultiTile.end());
		size_t index = iter - m_actorsMultiTile.begin();
		util::removeFromVectorByIndexUnordered(m_actorsMultiTile, index);
		util::removeFromVectorByIndexUnordered(m_blocksMultiTileActors, index);
	}
	else
	{
		auto iter = std::ranges::find(m_actorsSingleTile, actor);
		assert(iter != m_actorsSingleTile.end());
		size_t index = iter - m_actorsSingleTile.begin();
		util::removeFromVectorByIndexUnordered(m_actorsSingleTile, index);
		util::removeFromVectorByIndexUnordered(m_blocksSingleTileActors, index);
	}
}
void LocationBucket::update(Area& area, ActorIndex actor, std::vector<BlockIndex>& blockIndices)
{
	Actors& actors = area.getActors();
	if(actors.getShape(actor).isMultiTile)
	{
		auto iter = std::ranges::find(m_actorsMultiTile, actor);
		assert(iter != m_actorsMultiTile.end());
		size_t index = iter - m_actorsMultiTile.begin();
		m_blocksMultiTileActors.at(index) = blockIndices;
	}
	else
	{
		assert(blockIndices.size() == 1);
		auto iter = std::ranges::find(m_actorsSingleTile, actor);
		assert(iter != m_actorsSingleTile.end());
		size_t index = iter - m_actorsSingleTile.begin();
		m_blocksSingleTileActors.at(index) = blockIndices.front();
	}
}
void LocationBuckets::initalize()
{
	m_maxX = ((m_area.getBlocks().m_sizeX - 1) / Config::locationBucketSize) + 1;
	m_maxY = ((m_area.getBlocks().m_sizeY - 1) / Config::locationBucketSize) + 1;
	m_maxZ = ((m_area.getBlocks().m_sizeZ - 1) / Config::locationBucketSize) + 1;
	m_buckets.resize(m_maxX * m_maxY * m_maxZ);
}
LocationBucket& LocationBuckets::get(DistanceInBuckets x, DistanceInBuckets y, DistanceInBuckets z)
{
	return m_buckets[x + (y * m_maxX) + (z * m_maxX * m_maxY)];
}
LocationBucket& LocationBuckets::getBucketFor(const BlockIndex block)
{
	Point3D coordinates = m_area.getBlocks().getCoordinates(block);
	DistanceInBuckets x = coordinates.x / Config::locationBucketSize;
	DistanceInBuckets y = coordinates.y / Config::locationBucketSize;
	DistanceInBuckets z = coordinates.z / Config::locationBucketSize;
	return get(x, y, z);
}
void LocationBuckets::add(ActorIndex actor)
{
	auto& actorBlocks = m_area.getActors().getBlocks(actor);
	assert(!actorBlocks.empty());
	std::unordered_map<LocationBucket*, std::vector<BlockIndex>> blocksCollatedByBucket;
	Blocks& blockData = m_area.getBlocks();
	for(BlockIndex block : actorBlocks)
		blocksCollatedByBucket[&blockData.getLocationBucket(block)].push_back(block);
	for(auto& [bucket, blocks] : blocksCollatedByBucket)
	{
		assert(bucket);
		bucket->insert(m_area, actor, blocks);
	}
}
void LocationBuckets::remove(ActorIndex actor)
{
	std::unordered_set<LocationBucket*> buckets;
	Blocks& blocks = m_area.getBlocks();
	Actors& actors = m_area.getActors();
	assert(!actors.getBlocks(actor).empty());
	for(BlockIndex block : actors.getBlocks(actor))
		buckets.insert(&blocks.getLocationBucket(block));
	for(LocationBucket* bucket : buckets)
		bucket->erase(m_area, actor);
}
void LocationBuckets::update(ActorIndex actor, std::unordered_set<BlockIndex>& oldBlocks)
{
	std::unordered_set<LocationBucket*> oldSets;
	std::unordered_map<LocationBucket*, std::vector<BlockIndex>> continuedSets;
	std::unordered_map<LocationBucket*, std::vector<BlockIndex>> newSets;
	Blocks& blocks = m_area.getBlocks();
	Actors& actors = m_area.getActors();
	for(BlockIndex block : oldBlocks)
		oldSets.insert(&blocks.getLocationBucket(block));
	for(BlockIndex block : actors.getBlocks(actor))
	{
		auto set = &blocks.getLocationBucket(block);
		if(oldSets.contains(set))
			continuedSets[set].push_back(block);
		else
			newSets[set].push_back(block);
	}
	for(auto& set : oldSets)
		if(!continuedSets.contains(set))
			set->erase(m_area, actor);
	for(auto& [set, blocks] : continuedSets)
		set->update(m_area, actor, blocks);
	for(auto& [set, blocks] : newSets)
		set->insert(m_area, actor, blocks);
}
