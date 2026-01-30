#pragma once
#include "../numericTypes/types.h"
#include "../reference.h"
#include "../geometry/point3D.h"
#include "../geometry/pointSet.h"
#include <vector>
#include <memory_resource>

using LocationBucketContentsIndexWidth = int;
class LocationBucketContentsIndex : public StrongInteger<LocationBucketContentsIndex, LocationBucketContentsIndexWidth, INT32_MAX, 0>
{
public:
	LocationBucketContentsIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const LocationBucketContentsIndex& index) const { return index.get(); } };
};

class LocationBucket
{
	Point3DSet m_points;
	Eigen::Array<int, 1, Eigen::Dynamic> m_visionRangeSquared;
	Eigen::Array<Facing4, 1, Eigen::Dynamic> m_facing;
	StrongVector<ActorReference, LocationBucketContentsIndex> m_actors;
	void sort() { /* todo. */}
	void remove(const LocationBucketContentsIndex& index);
	[[nodiscard]] Eigen::Array<bool, 2, Eigen::Dynamic> canSeeAndCanBeSeenByDistanceAndFacingFilter(const Point3D& location, const Facing4& facing, const DistanceSquared& visionRangeSquared) const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> canBeSeenByDistanceAndFacingFilter(const Point3D& location) const;
public:
	void insert(const ActorReference& actor, const Point3D& point, const DistanceSquared& visionRangeSquared, const Facing4& facing);
	void remove(const ActorReference& actor);
	void copyIndex(const LocationBucket& other, const LocationBucketContentsIndex& otherIndex);
	void updateVisionRangeSquared(const ActorReference& actor, const Point3D& point, const DistanceSquared& visionRangeSquared);
	void prefetch() const;
	void reserve(int size);
	// TODO: prevent checking line of sight to multi tile creaters straddling location bucket boundry.
	[[nodiscard]] const std::pair<const std::vector<ActorReference>*, Eigen::Array<bool, 2, Eigen::Dynamic>>
	visionRequestQuery(const Area& area, const Point3D& position, const Facing4& facing, const DistanceSquared& getVisionRangeSquared, const CuboidSet& occupied, const Distance& largestVisionRange) const;
	[[nodiscard]] const std::pair<const StrongVector<ActorReference, LocationBucketContentsIndex>*, Eigen::Array<bool, 1, Eigen::Dynamic>>
	anyCanBeSeenQuery(const Area& area, const Cuboid& cuboid, const Point3DSet& points) const;
	[[nodiscard]] const std::pair<const StrongVector<ActorReference, LocationBucketContentsIndex>*, Eigen::Array<bool, 1, Eigen::Dynamic>>
	anyCanBeSeenQuery(const Area& area, const Cuboid& cuboid, const Point3DSet& points, const Eigen::Array<bool, 1, Eigen::Dynamic>& alreadyConfirmed) const;
	[[nodiscard]] const Point3DSet& getPoints() const { return m_points; }
	[[nodiscard]] int size() const { return m_actors.size(); }
	[[nodiscard]] bool empty() const { return m_actors.empty(); }
	[[nodiscard]] bool contains(const ActorReference& actor, const Point3D& coordinates) const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesWhichIntersectShape(const auto& queryShape) const;
	[[nodiscard]] Point3D getPosition(const LocationBucketContentsIndex& index) const { return m_points[index.get()]; }
	[[nodiscard]] DistanceSquared getVisionRangeSquared(const LocationBucketContentsIndex& index) const { return DistanceSquared::create(m_visionRangeSquared[index.get()]); }
	[[nodiscard]] Facing4 getFacing(const LocationBucketContentsIndex& index) const { return m_facing[index.get()]; }
};
