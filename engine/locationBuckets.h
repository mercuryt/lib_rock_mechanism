#pragma once
#include "types.h"
#include <cstdint>
#include <functional>
#include <unordered_set>
class Area;
class Block;
class Actor;
class VisionRequest;
class VisionFacade;

using DistanceInBuckets = uint32_t;

struct LocationBucket final
{
	std::vector<std::vector<BlockIndex>> m_blocksMultiTileActors;
	std::vector<Actor*> m_actorsMultiTile;
	std::vector<BlockIndex> m_blocksSingleTileActors;
	std::vector<Actor*> m_actorsSingleTile;
	void insert(Actor& actor, std::vector<BlockIndex>& blocks);
	void erase(Actor& actor);
	void update(Actor& actor, std::vector<BlockIndex>& blocks);
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
	void add(Actor& actor);
	void remove(Actor& actor);
	void update(Actor& actor, std::unordered_set<Block*>& oldBlocks);
	[[nodiscard]] LocationBucket& getBucketFor(const Block& block);
	friend class VisionFacade;
};
