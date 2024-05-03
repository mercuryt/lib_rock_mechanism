#pragma once
#include "types.h"
#include <cstdint>
#include <functional>
#include <unordered_set>
class Area;
class Block;
class Actor;
class VisionRequest;

using DistanceInBuckets = uint32_t;

class LocationBuckets
{
	Area& m_area;
	std::vector<std::vector<std::vector<
		std::unordered_set<Actor*>
		>>> m_buckets;
	DistanceInBuckets m_maxX;
	DistanceInBuckets m_maxY;
	DistanceInBuckets m_maxZ;
public:
	LocationBuckets(Area& area);
	void insert(Actor& actor, Block& block);
	void erase(Actor& actor);
	void update(Actor& actor, const Block& oldLocation, const Block& newLocation);
	void processVisionRequest(VisionRequest& visionRequest) const;
	std::unordered_set<Actor*>* getBucketFor(const Block& block);
	bool hasLineOfSight(VisionRequest& visionRequest, const Block& to, const Block& from) const;
	void getByConditionSortedByTaxiDistanceFrom(const Block& block, std::function<bool(const Actor&)> predicate, std::function<bool(const Actor&)> yeild);
	struct InRange
	{
		const LocationBuckets& locationBuckets;
		const Block& origin;
		DistanceInBlocks range;
		InRange(const LocationBuckets& lb, const Block& o, DistanceInBlocks r) : locationBuckets(lb), origin(o), range(r) { }
		struct iterator
		{
			InRange* inRange;
			DistanceInBuckets x;
			DistanceInBuckets y;
			DistanceInBuckets z;
			DistanceInBuckets maxX;
			DistanceInBuckets maxY;
			DistanceInBuckets maxZ;
			DistanceInBuckets minX;
			DistanceInBuckets minY;
			DistanceInBuckets minZ;
			const std::unordered_set<Actor*>* bucket;
			std::unordered_set<Actor*>::const_iterator bucketIterator;

			iterator(InRange& ir);
			iterator();

			using difference_type = std::ptrdiff_t;
			using value_type = Actor;
			using pointer = Actor*;
			using reference = Actor&;

			void getNextBucket();
			void findNextActor();
			iterator& operator++();
			iterator operator++(int) const;
			bool operator==(const iterator other) const;
			bool operator!=(const iterator other) const;
			reference operator*() const;
			pointer operator->() const;
		};
		iterator begin();
		iterator end();
		static_assert(std::forward_iterator<iterator>);
	};
	InRange inRange(const Block& origin, DistanceInBlocks range) const;
};
