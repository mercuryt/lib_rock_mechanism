#pragma once
#include "types.h"
#include "index.h"
#include "dataVector.h"
#include "reference.h"
#include <cstdint>
#include <functional>
class Area;
class Actor;
class VisionRequest;
class VisionFacade;

struct LocationBucketSingleTileActorData
{
	Point3D position;
	VisionCuboidId cuboidId;
	ActorReference actor;
	DistanceInBlocks rangeSquared;
	struct hash{ [[nodiscard]] static size_t operator()(const LocationBucketSingleTileActorData& data) { return data.actor.getReferenceIndex().get(); }};
	[[nodiscard]] bool operator==(const LocationBucketSingleTileActorData& other) const { return actor == other.actor; }
	[[nodiscard]] bool operator!=(const LocationBucketSingleTileActorData& other) const { return actor != other.actor; }
};
struct LocationBucketMultiTileActorData
{
	SmallMap<Point3D, VisionCuboidId> positionsAndCuboidIds;
	ActorReference actor;
	DistanceInBlocks rangeSquared;
	struct hash{ static size_t operator()(const LocationBucketMultiTileActorData& data) { return data.actor.getReferenceIndex().get(); }};
	[[nodiscard]] bool operator==(const LocationBucketMultiTileActorData& other) const { return actor == other.actor; }
	[[nodiscard]] bool operator!=(const LocationBucketMultiTileActorData& other) const { return actor != other.actor; }
};
struct LocationBucket final
{
	//TODO: Use HybridSequence.
	SmallSet<LocationBucketSingleTileActorData> m_actorsSingleTile;
	SmallSet<LocationBucketMultiTileActorData> m_actorsMultiTile;
	void insert(Area& area, const ActorIndex& actor, const BlockIndices& blocks);
	void erase(Area& area, const ActorIndex& actor);
	void update(Area& area, const ActorIndex& actor, const BlockIndices& blocks);
	// When a block changes it's cuboid id and stored in the location bucket data must be updated
	void updateCuboid(Area& area, const ActorIndex& actor, const BlockIndex& block, const VisionCuboidId& oldCuboid, const VisionCuboidId& newCuboid);
	void prefetch() const;
	[[nodiscard]] size_t size() const { return m_actorsMultiTile.size() + m_actorsSingleTile.size(); }
	[[nodiscard]] LocationBucketSingleTileActorData& getSingleTileData(const ActorReference& ref);
	[[nodiscard]] LocationBucketMultiTileActorData& getMultiTileData(const ActorReference& ref);
	[[nodiscard]] bool empty() const { return m_actorsMultiTile.empty() && m_actorsSingleTile.empty(); }
};
class LocationBuckets
{
	Area& m_area;
	std::vector<LocationBucket> m_buckets;
	DistanceInBuckets m_maxX;
	DistanceInBuckets m_maxY;
	DistanceInBuckets m_maxZ;
	[[nodiscard]] uint getIndex(const DistanceInBuckets& x, const DistanceInBuckets& y, const DistanceInBuckets& z) const;
	[[nodiscard]] LocationBucket& get(uint);
	[[nodiscard]] const LocationBucket& get(uint) const;
public:
	LocationBuckets(Area& area) : m_area(area) { }
	void initalize();
	void add(const ActorIndex& actor);
	void remove(const ActorIndex& actor);
	void update(const ActorIndex& actor, const BlockIndices& oldBlocks);
	void updateCuboid(Area& area, const ActorIndex& actor, const BlockIndex& block, const VisionCuboidId& oldCuboid, const VisionCuboidId& newCuboid);
	void prefetch(uint index) const;
	[[nodiscard]] uint getBucketIndexFor(const BlockIndex& block);
	[[nodiscard]] LocationBucket& getBucketFor(const BlockIndex& block) { return m_buckets[getBucketIndexFor(block)]; }
	friend class VisionRequests;
};
