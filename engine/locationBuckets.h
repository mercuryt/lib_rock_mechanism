#pragma once
#include "types.h"
#include "index.h"
#include <cstdint>
#include <functional>
#include <unordered_set>
class Area;
class Actor;
class VisionRequest;
class VisionFacade;

using DistanceInBuckets = uint32_t;

struct LocationBucket final
{
	std::vector<BlockIndices> m_blocksMultiTileActors;
	std::vector<ActorIndex> m_actorsMultiTile;
	std::vector<BlockIndex> m_blocksSingleTileActors;
	std::vector<ActorIndex> m_actorsSingleTile;
	void insert(Area& area, ActorIndex actor, BlockIndices& blocks);
	void erase(Area& area, ActorIndex actor);
	void update(Area& area, ActorIndex actor, BlockIndices& blocks);
	[[nodiscard]] size_t size() const { return m_actorsMultiTile.size() + m_actorsSingleTile.size(); }
};
class LocationBuckets
{
	Area& m_area;
	std::vector<LocationBucket> m_buckets;
	DistanceInBuckets m_maxX;
	DistanceInBuckets m_maxY;
	DistanceInBuckets m_maxZ;
	[[nodiscard]] LocationBucket& get(DistanceInBuckets x, DistanceInBuckets y, DistanceInBuckets z);
public:
	LocationBuckets(Area& area) : m_area(area) { }
	void initalize();
	void add(ActorIndex actor);
	void remove(ActorIndex actor);
	void update(ActorIndex actor, BlockIndices& oldBlocks);
	[[nodiscard]] LocationBucket& getBucketFor(const BlockIndex block);
	friend class VisionFacade;
};
