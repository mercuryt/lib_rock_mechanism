#pragma once
#include "types.h"
#include "index.h"
#include "dataVector.h"
#include <cstdint>
#include <functional>
class Area;
class Actor;
class VisionRequest;
class VisionFacade;

struct LocationBucket final
{
	//TODO: Use HybridSequence.
	DataVector<BlockIndices, LocationBucketId> m_blocksMultiTileActors;
	DataVector<ActorIndex, LocationBucketId> m_actorsMultiTile;
	DataVector<VisionCuboidIdSet, LocationBucketId> m_visionCuboidsMulitiTileActors;
	DataVector<BlockIndex, LocationBucketId> m_blocksSingleTileActors;
	DataVector<ActorIndex, LocationBucketId> m_actorsSingleTile;
	DataVector<VisionCuboidId, LocationBucketId> m_visionCuboidsSingleTileActors;
	void insert(Area& area, const ActorIndex& actor, const BlockIndices& blocks);
	void erase(Area& area, const ActorIndex& actor);
	void update(Area& area, const ActorIndex& actor, const BlockIndices& blocks);
	void updateCuboid(Area& area, const ActorIndex& actor, const VisionCuboidId& oldCuboid, const VisionCuboidId& newCuboid);
	[[nodiscard]] size_t size() const { return m_actorsMultiTile.size() + m_actorsSingleTile.size(); }
};
class LocationBuckets
{
	Area& m_area;
	std::vector<LocationBucket> m_buckets;
	DistanceInBuckets m_maxX;
	DistanceInBuckets m_maxY;
	DistanceInBuckets m_maxZ;
	[[nodiscard]] LocationBucket& get(const DistanceInBuckets& x, const DistanceInBuckets& y, const DistanceInBuckets& z);
	[[nodiscard]] const LocationBucket& get(const DistanceInBuckets& x, const DistanceInBuckets& y, const DistanceInBuckets& z) const;
public:
	LocationBuckets(Area& area) : m_area(area) { }
	void initalize();
	void add(const ActorIndex& actor);
	void remove(const ActorIndex& actor);
	void update(const ActorIndex& actor, const BlockIndices& oldBlocks);
	void updateCuboid(Area& area, const ActorIndex& actor, const BlockIndex& block, const VisionCuboidId& oldCuboid, const VisionCuboidId& newCuboid);
	[[nodiscard]] LocationBucket& getBucketFor(const BlockIndex& block);
	friend class VisionFacade;
};
