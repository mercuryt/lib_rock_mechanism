#include "locationBuckets.h"
#include "config.h"
#include "geometry/sphere.h"
#include "vision/lineOfSight.h"
#include "area/area.h"
#include <algorithm>
void sort()
{
	//TODO.
}
void LocationBucket::remove(const LocationBucketContentsIndex& index)
{
	uint lastIndex = m_actors.size() - 1;
	if(index != m_actors.size() - 1)
	{
		m_points[index.get()] = m_points[lastIndex];
		m_actors[index] = m_actors.back();
	}
	m_points.resize(lastIndex);
	m_actors.popBack();
	// Cuboid indices, vision range, and facing are implicitly truncated by m_actors.popBack().
}
void LocationBucket::copyIndex(const LocationBucket& other, const LocationBucketContentsIndex& otherIndex)
{
	LocationBucketContentsIndex index = LocationBucketContentsIndex::create(m_actors.size());
	if(m_actors.size() == m_actors.capacity())
		reserve(std::max(8u, m_actors.capacity() * 2));
	m_points.insert(Coordinates(other.m_points.data.col(otherIndex.get())));
	m_visionCuboidIndices[index.get()] = other.m_visionCuboidIndices[otherIndex.get()];
	m_visionRangeSquared[index.get()] = other.m_visionRangeSquared[otherIndex.get()];
	m_facing[index.get()] = other.m_facing[otherIndex.get()];
	m_actors.add(other.m_actors[otherIndex]);
}
void LocationBucket::insert(const ActorReference& actor, const Point3D& coordinates, const VisionCuboidId& cuboid, const DistanceInBlocks& visionRangeSquared, const Facing4& facing)
{
	LocationBucketContentsIndex index = LocationBucketContentsIndex::create(m_actors.size());
	if(m_actors.size() == m_actors.capacity())
		reserve(std::max(8u, m_actors.capacity() * 2u));
	m_points.insert(coordinates);
	m_visionCuboidIndices[index.get()] = cuboid.get();
	m_visionRangeSquared[index.get()] = visionRangeSquared.get();
	m_facing[index.get()] = facing;
	m_actors.add(actor);
	sort();
}
void LocationBucket::remove(const ActorReference& actor)
{
	for(auto i = LocationBucketContentsIndex::create(0); i < m_actors.size(); ++i)
		while(i < m_actors.size() && m_actors[i] == actor)
			remove(i);
}
void LocationBucket::updateVisionRangeSquared(const ActorReference& actor, const Point3D& coordinates, const DistanceInBlocks& visionRangeSquared)
{
	for(auto i = LocationBucketContentsIndex::create(0); i < m_actors.size(); ++i)
		if(m_actors[i] == actor && m_points[i.get()] == coordinates.data)
		{
			m_visionRangeSquared[i.get()] = visionRangeSquared.get();
			// Vision range is only stored in first block, which is also the head and location (location may be changed later in development).
			break;
		}
}
void LocationBucket::updateVisionCuboidIndex(const Point3D& coordinates, const VisionCuboidId& cuboid)
{
	for(auto i = LocationBucketContentsIndex::create(0); i < m_actors.size(); ++i)
		if(m_points[i.get()] == coordinates.data)
			m_visionCuboidIndices[i.get()] = cuboid.get();
}
bool LocationBucket::contains(const ActorReference& actor, const Point3D& coordinates) const
{
	for(auto i = LocationBucketContentsIndex::create(0); i < m_actors.size(); ++i)
		if(m_actors[i] == actor && (m_points.data.col(i.get()) == coordinates.data).all())
			return true;
	return false;
}
void LocationBucket::prefetch() const
{
	//TODO
}
void LocationBucket::reserve(int size)
{
	m_points.reserve(size);
	m_visionCuboidIndices.resize(1, size);
	m_visionRangeSquared.resize(1, size);
	m_facing.resize(1, size);
	m_actors.reserve(size);
}
const std::pair<const std::vector<ActorReference>*, Eigen::Array<bool, 2, Eigen::Dynamic>>
LocationBucket::visionRequestQuery(const Area& area, const Point3D& position, const Facing4& facing, const DistanceInBlocks& visionRangeSquared, const VisionCuboidId& visionCuboid, const VisionCuboidSetSIMD& visionCuboids, const OccupiedBlocksForHasShape& occupiedBlocks, const DistanceInBlocks& largestVisionRange) const
{
	Sphere sphere(position, largestVisionRange.toFloat());
	// Broad phase culling.
	Eigen::Array<bool, 1, Eigen::Dynamic> intersectsShape = m_points.indicesOfContainedPoints(sphere);
	Eigen::Array<bool, 2, Eigen::Dynamic> canSeeAndCanBeSeenBy = canSeeAndCanBeSeenByDistanceAndFacingFilter(sphere.center, facing, visionRangeSquared);
	Eigen::Array<bool, 1, Eigen::Dynamic> candidates = intersectsShape && ( canSeeAndCanBeSeenBy.row(0) || canSeeAndCanBeSeenBy.row(1) );
	Eigen::Array<bool, 1, Eigen::Dynamic> isInViewerCuboid = m_visionCuboidIndices == visionCuboid.get();
	// Line of sight checks.
	Eigen::Array<bool, 2, Eigen::Dynamic> output;
	output.resize(2, m_actors.size());
	output.fill(false);
	//TODO: multi tile actors have multiple entries in location bucket and might be checked for line of sight multiple times.
	for(uint index = 0; index < m_actors.size(); ++index)
	{
		if(candidates[index])
		{
			if(
				isInViewerCuboid[index] ||
				sphere.center == m_points[index] ||
				(
					visionCuboids.contains(m_visionCuboidIndices[index]) &&
					lineOfSight(area, sphere.center, m_points[index])
				)
			)
				output.col(index) = canSeeAndCanBeSeenBy.col(index);
		}
	}
	// Multi tile actors must check if their non location blocks can be seen.
	if(occupiedBlocks.size() != 1 && !output.row(1).all())
	{
		const Blocks& blocks = area.getBlocks();
		Point3DSet points = Point3DSet::fromBlockSet(blocks, occupiedBlocks);
		Cuboid cuboid = points.boundry().inflateAdd(largestVisionRange);
		auto canSeeOccupiedBlock = anyCanBeSeenQuery(area, cuboid, Point3DSet::fromBlockSet(blocks, occupiedBlocks), output.row(1));
		output.row(1) += canSeeOccupiedBlock.second;
	}
	return std::pair(&m_actors.toVector(), output);
}
const std::pair<const StrongVector<ActorReference, LocationBucketContentsIndex>*, Eigen::Array<bool, 1, Eigen::Dynamic>>
LocationBucket::anyCanBeSeenQuery(const Area& area, const Cuboid& cuboid, const Point3DSet& points) const
{
	//TODO: use a template instead of an empty array.
	static Eigen::Array<bool, 1, Eigen::Dynamic> alreadyEstablished;
	alreadyEstablished.resize(1, m_actors.size());
	alreadyEstablished.fill(false);
	return anyCanBeSeenQuery(area, cuboid, points, alreadyEstablished);
}
const std::pair<const StrongVector<ActorReference, LocationBucketContentsIndex>*, Eigen::Array<bool, 1, Eigen::Dynamic>>
LocationBucket::anyCanBeSeenQuery(const Area& area, const Cuboid& cuboid, const Point3DSet& points, const Eigen::Array<bool, 1, Eigen::Dynamic>& alreadyEstablished) const
{
	Eigen::Array<bool, 1, Eigen::Dynamic> intersectsShape = m_points.indicesOfContainedPoints(cuboid);
	// TODO: if there are more points then viewers iterate the viewers instead.
	Eigen::Array<bool, 1, Eigen::Dynamic> canBeSeenBy;
	canBeSeenBy.resize(1, m_actors.size());
	canBeSeenBy.fill(false);
	for(const Point3D& point : points)
	{
		canBeSeenBy += canBeSeenByDistanceAndFacingFilter(point);
		// TODO: profile this branch.
		if(canBeSeenBy.all())
			break;
	}
	// Check line of sight from each actor which passed broad phase culling to each point.
	// Raycasting can potentially saturate the SIMD registers so there is probably nothing to be gained from further data level parallelism here, despite the nested loops.
	for(uint i = 0; i < m_actors.size(); i++)
		if(canBeSeenBy[i] && !alreadyEstablished[i])
		{
			bool lineOfSightFound = false;
			for(uint j = 0; j < points.size(); ++j)
				// We need to check for equality here but not in visionRequestQuery because we aren't using vision cuboids here.
				if(m_points[i] == points[j] || lineOfSight(area, m_points[i], points[j]))
				{
					lineOfSightFound = true;
					break;
				}
			if(!lineOfSightFound)
				canBeSeenBy[i] = false;
		}
	return {&m_actors, canBeSeenBy};
}
Eigen::Array<bool, 2, Eigen::Dynamic> LocationBucket::canSeeAndCanBeSeenByDistanceAndFacingFilter(const Point3D& location, const Facing4& facing, const DistanceInBlocks& visionRangeSquared) const
{
	Eigen::Array<int, 1, Eigen::Dynamic> distances = m_points.distancesSquare(location);
	Eigen::Array<Facing4, 1, Eigen::Dynamic> truncatedFacings = m_facing.block(0, 0, 1, m_actors.size());
	Eigen::Array<int, 1, Eigen::Dynamic> truncatedVisionRange = m_visionRangeSquared.block(0, 0, 1, m_actors.size());
	Eigen::Array<bool, 1, Eigen::Dynamic> facingTwordsLocation = m_points.indicesFacingTwords(location, truncatedFacings);
	Eigen::Array<bool, 1, Eigen::Dynamic> isFacedTwordsFrom = m_points.indicesOfFacedTwordsPoints(location, facing);
	Eigen::Array<bool, 1, Eigen::Dynamic> canSee = isFacedTwordsFrom && distances <= visionRangeSquared.get();
	Eigen::Array<bool, 1, Eigen::Dynamic> canBeSeenBy = facingTwordsLocation && distances <= truncatedVisionRange;
	Eigen::Array<bool, 2, Eigen::Dynamic> output;
	output.resize(2, m_actors.size());
	output.row(0) = canSee;
	output.row(1) = canBeSeenBy;
	return output;
}
Eigen::Array<bool, 1, Eigen::Dynamic> LocationBucket::canBeSeenByDistanceAndFacingFilter(const Point3D& location) const
{
	Eigen::Array<int, 1, Eigen::Dynamic> distances = m_points.distancesSquare(location);
	Eigen::Array<Facing4, 1, Eigen::Dynamic> truncatedFacings = m_facing.block(0, 0, 1, m_actors.size());
	Eigen::Array<bool, 1, Eigen::Dynamic> facingTwordsLocation = m_points.indicesFacingTwords(location, truncatedFacings);
	Eigen::Array<int, 1, Eigen::Dynamic> truncatedVisionRange = m_visionRangeSquared.block(0, 0, 1, m_actors.size());
	return facingTwordsLocation && distances <= truncatedVisionRange;
}
Eigen::Array<bool, 1, Eigen::Dynamic> LocationBucket::indicesWhichIntersectShape(const auto& queryShape) const
{
	return m_points.indicesOfContainedPoints(queryShape);
}