#pragma once
#include "types.h"
#include "reference.h"
#include <vector>
#include <memory_resource>

using Allocator = std::pmr::unsynchronized_pool_resource;

// LocationBucket::m_data is kept sorted by actor reference, so if we see one tile of a multi tile actor we can skip ahead in the forEach callback.
struct LocationBucketData
{
	Point3D coordinates;
	VisionCuboidIndex cuboid;
	ActorReference actor;
	DistanceInBlocks visionRangeSquared;
	Facing4 facing;
	struct hash{ [[nodiscard]] static size_t operator()(const LocationBucketData& data) { return data.actor.getReferenceIndex().get(); }};
	[[nodiscard]] std::strong_ordering operator<=>(const LocationBucketData& other) { return actor <=> other.actor; }
};
class LocationBucket
{
	std::pmr::vector<LocationBucketData> m_data;
	void sort();
public:
	LocationBucket(Allocator& allocator) : m_data(&allocator) { }
	void insert(const LocationBucketData& data);
	LocationBucketData& insert(const ActorReference& actor, const Point3D& point, const VisionCuboidIndex& cuboid, const DistanceInBlocks& visionRangeSquered, const Facing4& facing);
	void remove(const ActorReference& actor);
	void updateVisionRangeSquared(const ActorReference& actor, const Point3D& point, const DistanceInBlocks& visionRangeSquaed);
	void updateVisionCuboidIndex(const Point3D& point, const VisionCuboidIndex& cuboid);
	void prefetch() const;
	template<typename Action>
	void forEach(Action&& action) const
	{
		for (const LocationBucketData &data : m_data)
			action(data);
	}
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] const std::pmr::vector<LocationBucketData>& get() const { return m_data; }
	[[nodiscard]] bool contains(const ActorReference& actor, const Point3D& coordinates) const;
};