#pragma once
#include "types.h"
#include "reference.h"
#include "geometry/point3D.h"
#include "geometry/pointSet.h"
#include "vision/visionCuboid.h"
#include <vector>
#include <memory_resource>

using LocationBucketContentsIndexWidth = uint32_t;
class LocationBucketContentsIndex : public StrongInteger<LocationBucketContentsIndex, LocationBucketContentsIndexWidth>
{
public:
	LocationBucketContentsIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const LocationBucketContentsIndex& index) const { return index.get(); } };
};

class LocationBucket
{
	Point3DSet m_points;
	Eigen::Array<VisionCuboidIndexWidth, 1, Eigen::Dynamic> m_visionCuboidIndices;
	Eigen::Array<int, 1, Eigen::Dynamic> m_visionRangeSquared;
	Eigen::Array<Facing4, 1, Eigen::Dynamic> m_facing;
	StrongVector<ActorReference, LocationBucketContentsIndex> m_actors;
	void sort() { /* todo. */}
	void remove(const LocationBucketContentsIndex& index);
	[[nodiscard]] Eigen::Array<bool, 2, Eigen::Dynamic> canSeeAndCanBeSeenByDistanceAndFacingFilter(const Point3D& location, const Facing4& facing, const DistanceInBlocks& visionRangeSquared) const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> canBeSeenByDistanceAndFacingFilter(const Point3D& location) const;
public:
	void insert(const ActorReference& actor, const Point3D& point, const VisionCuboidId& cuboid, const DistanceInBlocks& visionRangeSquared, const Facing4& facing);
	void remove(const ActorReference& actor);
	void copyIndex(const LocationBucket& other, const LocationBucketContentsIndex& otherIndex);
	void updateVisionRangeSquared(const ActorReference& actor, const Point3D& point, const DistanceInBlocks& visionRangeSquaed);
	void updateVisionCuboidIndex(const Point3D& point, const VisionCuboidId& cuboid);
	void prefetch() const;
	void reserve(int size);
	// TODO: prevent checking line of sight to multi tile creaters straddling location bucket boundry.
	[[nodiscard]] const std::pair<const std::vector<ActorReference>*, Eigen::Array<bool, 2, Eigen::Dynamic>>
	visionRequestQuery(const Area& area, const Point3D& position, const Facing4& facing, const DistanceInBlocks& getVisionRangeSquared, const VisionCuboidId& visionCuboid, const VisionCuboidSetSIMD& visionCuboids, const OccupiedBlocksForHasShape& occupiedBlocks, const DistanceInBlocks& largestVisionRange) const;
	[[nodiscard]] const std::pair<const StrongVector<ActorReference, LocationBucketContentsIndex>*, Eigen::Array<bool, 1, Eigen::Dynamic>>
	anyCanBeSeenQuery(const Area& area, const Cuboid& cuboid, const Point3DSet& points) const;
	[[nodiscard]] const std::pair<const StrongVector<ActorReference, LocationBucketContentsIndex>*, Eigen::Array<bool, 1, Eigen::Dynamic>>
	anyCanBeSeenQuery(const Area& area, const Cuboid& cuboid, const Point3DSet& points, const Eigen::Array<bool, 1, Eigen::Dynamic>& alreadyConfirmed) const;
	[[nodiscard]] const Point3DSet& getPoints() const { return m_points; }
	[[nodiscard]] uint size() const { return m_actors.size(); }
	[[nodiscard]] bool empty() const { return m_actors.empty(); }
	[[nodiscard]] bool contains(const ActorReference& actor, const Point3D& coordinates) const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indiciesWhichIntersectShape(const auto& queryShape) const;
	[[nodiscard]] Point3D getPosition(const LocationBucketContentsIndex& index) const { return m_points[index.get()]; }
	[[nodiscard]] VisionCuboidId getCuboidIndex(const LocationBucketContentsIndex& index) const { return VisionCuboidId::create(m_visionCuboidIndices[index.get()]); }
	[[nodiscard]] DistanceInBlocks getVisionRangeSquared(const LocationBucketContentsIndex& index) const { return DistanceInBlocks::create(m_visionRangeSquared[index.get()]); }
	[[nodiscard]] Facing4 getFacing(const LocationBucketContentsIndex& index) const { return m_facing[index.get()]; }
};
