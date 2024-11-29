#include "locationBuckets.h"
#include "area.h"
#include "config.h"
#include "types.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "util.h"
#include <algorithm>
void LocationBucket::insert(Area& area, const ActorIndex& actor, const BlockIndices& blocks)
{
	Actors& actors = area.getActors();
	Blocks& blocksData = area.getBlocks();
	if(Shape::getIsMultiTile(actors.getShape(actor)))
	{
		[[maybe_unused]] auto iter = std::ranges::find(m_actorsMultiTile, actor);
		assert(!m_actorsMultiTile.contains(actor));
		m_actorsMultiTile.add(actor);
		m_positionsAndCuboidsMultiTileActors.add({});
		for(const BlockIndex& block : blocks)
			m_positionsAndCuboidsMultiTileActors.back().emplace(
				blocksData.getCoordinates(block),
				area.m_visionCuboids.getIdFor(block)
			);
	}
	else
	{
		assert(blocks.size() == 1);
		assert(!m_actorsSingleTile.contains(actor));
		m_actorsSingleTile.add(actor);
		m_positionsSingleTileActors.add(blocksData.getCoordinates(blocks.front()));
		m_visionCuboidsSingleTileActors.add(area.m_visionCuboids.getIdFor(blocks.front()));
	}
}
void LocationBucket::erase(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(Shape::getIsMultiTile(actors.getShape(actor)))
	{
		auto iter = std::ranges::find(m_actorsMultiTile, actor);
		assert(iter != m_actorsMultiTile.end());
		LocationBucketId index = LocationBucketId::create(iter - m_actorsSingleTile.begin());
		m_actorsMultiTile.remove(index);
		m_positionsAndCuboidsMultiTileActors.remove(index);
	}
	else
	{
		auto iter = std::ranges::find(m_actorsSingleTile, actor);
		assert(iter != m_actorsSingleTile.end());
		LocationBucketId index = LocationBucketId::create(iter - m_actorsSingleTile.begin());
		m_actorsSingleTile.remove(index);
		m_positionsSingleTileActors.remove(index);
		m_visionCuboidsSingleTileActors.remove(index);
	}
}
void LocationBucket::update(Area& area, const ActorIndex& actor, const BlockIndices& blocks)
{
	Actors& actors = area.getActors();
	Blocks& blocksData = area.getBlocks();
	if(Shape::getIsMultiTile(actors.getShape(actor)))
	{
		auto iter = std::ranges::find(m_actorsMultiTile, actor);
		assert(iter != m_actorsMultiTile.end());
		LocationBucketId index = LocationBucketId::create(iter - m_actorsMultiTile.begin());
		m_positionsAndCuboidsMultiTileActors[index].clear();
		for(const BlockIndex& block : blocks)
			m_positionsAndCuboidsMultiTileActors[index].emplace(
				blocksData.getCoordinates(block),
				area.m_visionCuboids.getIdFor(block)
			);
	}
	else
	{
		assert(blocks.size() == 1);
		auto iter = std::ranges::find(m_actorsSingleTile, actor);
		assert(iter != m_actorsSingleTile.end());
		LocationBucketId index = LocationBucketId::create(iter - m_actorsSingleTile.begin());
		m_positionsSingleTileActors[index] = blocksData.getCoordinates(blocks.front());
		m_visionCuboidsSingleTileActors[index] = area.m_visionCuboids.getIdFor(actors.getLocation(actor));
	}
}
void LocationBucket::updateCuboid(Area& area, const ActorIndex& actor, const BlockIndex& block, const VisionCuboidId& oldCuboid, const VisionCuboidId& newCuboid)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	if(Shape::getIsMultiTile(actors.getShape(actor)))
	{
		Point3D position = blocks.getCoordinates(block);
		LocationBucketId index = m_actorsMultiTile.indexFor(actor);
		assert(m_positionsAndCuboidsMultiTileActors[index][position] == oldCuboid);
		m_positionsAndCuboidsMultiTileActors[index][position] = newCuboid;
	}
	else
	{
		LocationBucketId index = m_actorsSingleTile.indexFor(actor);
		assert(m_visionCuboidsSingleTileActors[index] == oldCuboid);
		m_visionCuboidsSingleTileActors[index] = newCuboid;
	}
}
void LocationBucket::prefetch() const
{
	// TODO: benchmark this, consider combining vectors.
	__builtin_prefetch(&*m_positionsSingleTileActors.begin());
	__builtin_prefetch(&*m_visionCuboidsSingleTileActors.begin());
}
void LocationBuckets::prefetch(uint index) const
{
	m_buckets[index].prefetch();
}
void LocationBuckets::initalize()
{
	m_maxX = DistanceInBuckets::create((((m_area.getBlocks().m_sizeX - 1) / Config::locationBucketSize) + 1).get());
	m_maxY = DistanceInBuckets::create((((m_area.getBlocks().m_sizeY - 1) / Config::locationBucketSize) + 1).get());
	m_maxZ = DistanceInBuckets::create((((m_area.getBlocks().m_sizeZ - 1) / Config::locationBucketSize) + 1).get());
	m_buckets.resize(m_maxX.get() * m_maxY.get() * m_maxZ.get());
}
uint LocationBuckets::getIndex(const DistanceInBuckets& x, const DistanceInBuckets& y, const DistanceInBuckets& z) const
{
	return (x + (y * m_maxX) + (z * m_maxX * m_maxY)).get();
}
LocationBucket& LocationBuckets::get(uint index)
{
	return m_buckets[index];
}
const LocationBucket& LocationBuckets::get(uint index) const 
{
	return const_cast<LocationBuckets&>(*this).get(index);
}
LocationBucket& LocationBuckets::getBucketFor(const BlockIndex& block)
{
	Point3D coordinates = m_area.getBlocks().getCoordinates(block);
	DistanceInBuckets x = DistanceInBuckets::create((coordinates.x / Config::locationBucketSize).get());
	DistanceInBuckets y = DistanceInBuckets::create((coordinates.y / Config::locationBucketSize).get());
	DistanceInBuckets z = DistanceInBuckets::create((coordinates.z / Config::locationBucketSize).get());
	return get(getIndex(x, y, z));
}
void LocationBuckets::add(const ActorIndex& actor)
{
	auto& actorBlocks = m_area.getActors().getBlocks(actor);
	assert(!actorBlocks.empty());
	SmallMap<LocationBucket*, BlockIndices> blocksCollatedByBucket;
	Blocks& blockData = m_area.getBlocks();
	for(BlockIndex block : actorBlocks)
	{
		LocationBucket* bucket = &blockData.getLocationBucket(block);
		blocksCollatedByBucket.getOrCreate(bucket).add(block);
	}
	for(auto [bucket, blocks] : blocksCollatedByBucket)
	{
		assert(bucket != nullptr);
		bucket->insert(m_area, actor, blocks);
	}
}
void LocationBuckets::remove(const ActorIndex& actor)
{
	SmallSet<LocationBucket*> buckets;
	Blocks& blocks = m_area.getBlocks();
	Actors& actors = m_area.getActors();
	assert(!actors.getBlocks(actor).empty());
	for(BlockIndex block : actors.getBlocks(actor))
		buckets.insert(&blocks.getLocationBucket(block));
	for(LocationBucket* bucket : buckets)
		bucket->erase(m_area, actor);
}
void LocationBuckets::update(const ActorIndex& actor, const BlockIndices& oldBlocks)
{
	SmallSet<LocationBucket*> oldSets;
	SmallMap<LocationBucket*, BlockIndices> continuedSets;
	SmallMap<LocationBucket*, BlockIndices> newSets;
	Blocks& blocks = m_area.getBlocks();
	Actors& actors = m_area.getActors();
	for(BlockIndex block : oldBlocks)
		oldSets.insert(&blocks.getLocationBucket(block));
	for(BlockIndex block : actors.getBlocks(actor))
	{
		auto set = &blocks.getLocationBucket(block);
		if(oldSets.contains(set))
			continuedSets[set].add(block);
		else
			newSets[set].add(block);
	}
	for(auto& set : oldSets)
		if(!continuedSets.contains(set))
			set->erase(m_area, actor);
	for(auto [set, blocks] : continuedSets)
		set->update(m_area, actor, blocks);
	for(auto [set, blocks] : newSets)
		set->insert(m_area, actor, blocks);
}
void LocationBuckets::updateCuboid(Area& area, const ActorIndex& actor, const BlockIndex& block, const VisionCuboidId& oldCuboid, const VisionCuboidId& newCuboid)
{
	getBucketFor(block).updateCuboid(area, actor, block, oldCuboid, newCuboid);
}