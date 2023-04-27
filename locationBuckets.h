#pragma once
class VisionRequest;
class LocationBuckets
{
	AREA& m_area;
	std::vector<std::vector<std::vector<
		std::unordered_set<ACTOR*>
		>>> m_buckets;
	uint32_t m_maxX;
	uint32_t m_maxY;
	uint32_t m_maxZ;
public:
	LocationBuckets(AREA& area);
	void insert(ACTOR& actor);
	void erase(ACTOR& actor);
	void update(ACTOR& actor, const BLOCK& oldLocation, const BLOCK& newLocation);
	void processVisionRequest(VisionRequest& visionRequest) const;
	std::unordered_set<ACTOR*>* getBucketFor(const BLOCK& block);
	bool hasLineOfSight(VisionRequest& visionRequest, const BLOCK& to, const BLOCK& from) const;
	struct InRange
	{
		const LocationBuckets& locationBuckets;
		const BLOCK& origin;
		uint32_t range;
		struct iterator
		{
			InRange* inRange;
			uint32_t x;
			uint32_t y;
			uint32_t z;
			uint32_t maxX;
			uint32_t maxY;
			uint32_t maxZ;
			uint32_t minX;
			uint32_t minY;
			uint32_t minZ;
			const std::unordered_set<ACTOR*>* bucket;
			std::unordered_set<ACTOR*>::const_iterator bucketIterator;

			iterator(InRange& ir);
			iterator();

			using difference_type = std::ptrdiff_t;
			using value_type = ACTOR;
			using pointer = ACTOR*;
			using reference = ACTOR&;

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
	InRange inRange(const BLOCK& origin, uint32_t range) const;
};
