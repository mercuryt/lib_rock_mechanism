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

class LocationBucket final
{
	std::vector<Actor*> m_actors;
	std::vector<std::vector<Block*>> m_blocks;
	[[nodiscard]] size_t indexFor(Actor& actor) const;
public:
	void insert(Actor& actor, std::vector<Block*>& blocks);
	void erase(Actor& actor);
	void update(Actor& actor, std::vector<Block*>& blocks);
	[[nodiscard]] size_t size() const { return m_actors.size(); }
	[[nodiscard]] Actor* getActor(size_t index);
	[[nodiscard]] std::vector<Block*>& getBlocks(size_t index);
};

class LocationBuckets
{
	Area& m_area;
	std::vector<LocationBucket> m_buckets;
	DistanceInBuckets m_maxX;
	DistanceInBuckets m_maxY;
	DistanceInBuckets m_maxZ;
	[[nodiscard]] LocationBucket& get(DistanceInBuckets x, DistanceInBuckets y, DistanceInBuckets z);
	[[nodiscard]] const LocationBucket& get(DistanceInBuckets x, DistanceInBuckets y, DistanceInBuckets z) const;
public:
	LocationBuckets(Area& area);
	void add(Actor& actor);
	void remove(Actor& actor);
	void update(Actor& actor, std::unordered_set<Block*>& oldBlocks);
	[[nodiscard]] LocationBucket& getBucketFor(const Block& block);
	friend class VisionFacade;
};
