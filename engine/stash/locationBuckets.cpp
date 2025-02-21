#include "locationBuckets.h"
#include "area/area.h"
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
	ActorReference ref = actors.getReference(actor);
	if(Shape::getIsMultiTile(actors.getShape(actor)))
	{
		assert(!m_actorsMultiTile.containsAny([&](const LocationBucketMultiTileActorData& data) { return data.actor == ref; }));
		decltype(LocationBucketMultiTileActorData::positionsAndCuboidIds) positionsAndCuboidIds;
		for(BlockIndex block : actors.getBlocks(actor))
			positionsAndCuboidIds.insert(blocksData.getCoordinates(block), area.m_visionCuboids.getIdFor(block));
		m_actorsMultiTile.emplace(std::move(positionsAndCuboidIds), ref, actors.vision_getRangeSquared(actor));
	}
	else
	{
		assert(blocks.size() == 1);
		assert(!m_actorsSingleTile.containsAny([&](const LocationBucketSingleTileActorData& data) { return data.actor == ref; }));
		BlockIndex block = blocks.front();
		m_actorsSingleTile.emplace(blocksData.getCoordinates(block), area.m_visionCuboids.getIdFor(block), ref, actors.vision_getRangeSquared(actor));
	}
}
void LocationBucket::erase(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	ActorReference ref = actors.getReference(actor);
	if(Shape::getIsMultiTile(actors.getShape(actor)))
		m_actorsMultiTile.eraseIf([&](const LocationBucketMultiTileActorData& data) { return data.actor == ref; });
	else
		m_actorsSingleTile.eraseIf([&](const LocationBucketSingleTileActorData& data) { return data.actor == ref; });
}
void LocationBucket::update(Area& area, const ActorIndex& actor, const BlockIndices& blocks)
{
	Actors& actors = area.getActors();
	Blocks& blocksData = area.getBlocks();
	ActorReference ref = actors.getReference(actor);
	if(Shape::getIsMultiTile(actors.getShape(actor)))
	{
		LocationBucketMultiTileActorData& data = getMultiTileData(ref);
		data.positionsAndCuboidIds.clear();
		for(BlockIndex block : actors.getBlocks(actor))
			data.positionsAndCuboidIds.insert(blocksData.getCoordinates(block), area.m_visionCuboids.getIdFor(block));
	}
	else
	{
		assert(blocks.size() == 1);
		BlockIndex block = blocks.front();
		LocationBucketSingleTileActorData& data = getSingleTileData(ref);
		data.position = blocksData.getCoordinates(block);
		data.cuboidId = area.m_visionCuboids.getIdFor(actors.getLocation(actor));
	}
}
void LocationBucket::updateCuboid(Area& area, const ActorIndex& actor, const BlockIndex& block, const VisionCuboidId& oldCuboid, const VisionCuboidId& newCuboid)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	ActorReference ref = actors.getReference(actor);
	if(Shape::getIsMultiTile(actors.getShape(actor)))
	{
		LocationBucketMultiTileActorData& data = getMultiTileData(ref);
		Point3D position = blocks.getCoordinates(block);
		assert(data.positionsAndCuboidIds[position] == oldCuboid);
		data.positionsAndCuboidIds[position] = newCuboid;
	}
	else
	{
		LocationBucketSingleTileActorData& data = getSingleTileData(ref);
		assert(data.cuboidId == oldCuboid);
		data.cuboidId = newCuboid;
	}
}
void LocationBucket::prefetch() const
{
	//TODO: benchmark this.
	__builtin_prefetch(&*m_actorsSingleTile.begin());
	__builtin_prefetch(&*m_actorsMultiTile.begin());
}
LocationBucketSingleTileActorData& LocationBucket::getSingleTileData(const ActorReference& ref)
{
	auto found = m_actorsSingleTile.findIf([&](const LocationBucketSingleTileActorData& data){ return data.actor == ref; });
	assert(found != m_actorsSingleTile.end());
	return *found;
}
LocationBucketMultiTileActorData& LocationBucket::getMultiTileData(const ActorReference& ref)
{
	auto found = m_actorsMultiTile.findIf([&](const LocationBucketMultiTileActorData& data){ return data.actor == ref; });
	assert(found != m_actorsMultiTile.end());
	return *found;
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
uint LocationBuckets::getBucketIndexFor(const BlockIndex& block)
{
	Point3D coordinates = m_area.getBlocks().getCoordinates(block);
	DistanceInBuckets x = DistanceInBuckets::create((coordinates.x / Config::locationBucketSize).get());
	DistanceInBuckets y = DistanceInBuckets::create((coordinates.y / Config::locationBucketSize).get());
	DistanceInBuckets z = DistanceInBuckets::create((coordinates.z / Config::locationBucketSize).get());
	return getIndex(x, y, z);
}
void LocationBuckets::add(const ActorIndex& actor)
{
	auto& actorBlocks = m_area.getActors().getBlocks(actor);
	assert(!actorBlocks.empty());
	SmallMap<uint, BlockIndices> blocksCollatedByBucketIndex;
	for(const BlockIndex& block : actorBlocks)
		blocksCollatedByBucketIndex.getOrCreate(getBucketIndexFor(block)).add(block);
	for(const auto& [bucketIndex, blocks] : blocksCollatedByBucketIndex)
	{
		m_buckets[bucketIndex].insert(m_area, actor, blocks);
	}
}
void LocationBuckets::remove(const ActorIndex& actor)
{
	SmallSet<uint> bucketIndices;
	Actors& actors = m_area.getActors();
	assert(!actors.getBlocks(actor).empty());
	for(BlockIndex block : actors.getBlocks(actor))
		bucketIndices.insert(getBucketIndexFor(block));
	bucketIndices.makeUnique();
	for(int bucketIndex : bucketIndices)
	{
		m_buckets[bucketIndex].erase(m_area, actor);
	}
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
	m_buckets[getBucketIndexFor(block)].updateCuboid(area, actor, block, oldCuboid, newCuboid);
}