#pragma once
#include "types.h"
#include "reference.h"

// LocationBucket::m_data is kept sorted by actor reference, so if we see one tile of a multi tile actor we can skip ahead in the forEach callback.
struct LocationBucketData
{
	Point3D coordinates;
	VisionCuboidId cuboid;
	ActorReference actor;
	DistanceInBlocks visionRangeSquared;
	struct hash{ [[nodiscard]] static size_t operator()(const LocationBucketData& data) { return data.actor.getReferenceIndex().get(); }};
	[[nodiscard]] std::strong_ordering operator<=>(const LocationBucketData& other) { return actor <=> other.actor; }
};

class LocationBucket
{
	std::vector<LocationBucketData> m_data;
	void sort();
public:
	void insert(const LocationBucketData& data);
	LocationBucketData& insert(const ActorReference& actor, const Point3D& point, const VisionCuboidId& cuboid, const DistanceInBlocks& visionRangeSquered);
	void remove(const ActorReference& actor);
	void updateVisionRangeSquared(const ActorReference& actor, const Point3D& point, const DistanceInBlocks& visionRangeSquaed);
	void updateVisionCuboidId(const Point3D& point, const VisionCuboidId& cuboid);
	template<typename Action>
	void forEach(Action&& action) const
	{
		for (const LocationBucketData &data : m_data)
			action(data);
	}
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] const std::vector<LocationBucketData>& get() const { return m_data; }
	[[nodiscard]] bool contains(const ActorReference& actor, const Point3D& coordinates) const;
};