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

class LocationBuckets
{
	Area& m_area;
	std::vector<std::unordered_set<Actor*>> m_buckets;
	DistanceInBuckets m_maxX;
	DistanceInBuckets m_maxY;
	DistanceInBuckets m_maxZ;
	std::unordered_set<Actor*>& get(DistanceInBuckets x, DistanceInBuckets y, DistanceInBuckets z);
	const std::unordered_set<Actor*>& get(DistanceInBuckets x, DistanceInBuckets y, DistanceInBuckets z) const;
public:
	LocationBuckets(Area& area);
	void add(Actor& actor);
	void remove(Actor& actor);
	void update(Actor& actor, std::unordered_set<Block*>& oldBlocks);
	std::unordered_set<Actor*>& getBucketFor(const Block& block);
	friend class VisionFacade;
};
