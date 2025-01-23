#include "locationBuckets.h"
#include <algorithm>
void LocationBucket::sort() { std::sort(m_data.begin(), m_data.end()); }
void LocationBucket::insert(const LocationBucketData &data)
{
	m_data.push_back(data);
	sort();
}
LocationBucketData& LocationBucket::insert(const ActorReference& actor, const Point3D& coordinates, const VisionCuboidIndex& cuboid, const DistanceInBlocks& visionRangeSquered, const Facing4& facing)
{
	m_data.emplace_back(coordinates, cuboid, actor, visionRangeSquered, facing);
	sort();
	// TODO: this search could be avoided by a custom sort which tracks the location of the newly created data.
	auto found = std::ranges::find_if(m_data, [&](const LocationBucketData& data){ return data.actor == actor && data.coordinates == coordinates; });
	return *found;
}
void LocationBucket::remove(const ActorReference& actor)
{
	auto found = std::ranges::find(m_data, actor, &LocationBucketData::actor);
	assert(found != m_data.end());
	// Erase maintaining sort order.
	m_data.erase(found);
}
void LocationBucket::updateVisionRangeSquared(const ActorReference& actor, const Point3D& coordinates, const DistanceInBlocks& visionRangeSquared)
{
	auto found = std::ranges::find_if(m_data, [&](const LocationBucketData& data){ return data.actor == actor && data.coordinates == coordinates; });
	assert(found != m_data.end());
	found->visionRangeSquared = visionRangeSquared;
}
void LocationBucket::updateVisionCuboidIndex(const Point3D& coordinates, const VisionCuboidIndex& cuboid)
{
	std::ranges::for_each(m_data, [&](LocationBucketData& data) { if(data.coordinates == coordinates) data.cuboid = cuboid; });
}
bool LocationBucket::contains(const ActorReference& actor, const Point3D& coordinates) const
{
	auto found = std::ranges::find_if(m_data, [&](const LocationBucketData& data){ return data.actor == actor && data.coordinates == coordinates; });
	return found != m_data.end();
}
void LocationBucket::prefetch() const
{
	//TODO: benchmark this.
	__builtin_prefetch(&*m_data.begin());
}