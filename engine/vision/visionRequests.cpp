#include "vision/visionRequests.h"
#include "area.h"
#include "config.h"
#include "locationBuckets.h"
#include "simulation.h"
#include "types.h"
#include "util.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "strongInteger.hpp"
#include "geometry/sphere.h"
#include <cstddef>

VisionRequests::VisionRequests(Area& area) :
	m_area(area)
{
	m_data.reserve(Config::visionRequestsReservationSize);
}
void VisionRequests::create(const ActorReference& actor)
{
	Actors& actors = m_area.getActors();
	ActorIndex index = actor.getIndex(actors.m_referenceData);
	assert(actors.hasLocation(index));
	assert(actors.vision_canSeeAnything(index));
	const BlockIndex& location = actors.getLocation(index);
	const VisionCuboidId& visionCuboidIndex = m_area.m_visionCuboids.getVisionCuboidIndexForBlock(location);
	m_data.emplace(m_area.getBlocks().getCoordinates(location), location, visionCuboidIndex, actor, actors.vision_getRange(index), actors.getFacing(index), actors.getBlocks(index));
	if(m_data.back().range > m_largestRange)
		m_largestRange = m_data.back().range;
}
void VisionRequests::maybeCreate(const ActorReference& actor)
{
	Actors& actors = m_area.getActors();
	if(actors.vision_canSeeAnything(actor.getIndex(actors.m_referenceData)) && !m_data.containsAny([&](const auto& data){ return data.actor == actor; }))
		create(actor);
}
void VisionRequests::cancelIfExists(const ActorReference& actor)
{
	auto found = m_data.findIf([&](const VisionRequest& request) { return request.actor == actor; });
	if(found == m_data.end())
		return;
	bool largestRange = found->range == m_largestRange;
	m_data.erase(found);
	if(m_data.empty())
		m_largestRange = DistanceInBlocks::create(0);
	else if(largestRange)
		m_largestRange = std::max_element(m_data.begin(), m_data.end(), [](const VisionRequest& a, const VisionRequest& b) { return a.actor < b.actor; })->range;
}
void VisionRequests::readStepSegment(const uint& begin,  const uint& end)
{
	for(auto iter = m_data.begin() + begin; iter != m_data.begin() + end; ++iter)
	{
		VisionRequest& request = *iter;
		const DistanceInBlocks& rangeSquared = request.range * request.range;
		const VisionCuboidId& fromVisionCuboidIndex = request.cuboid;
		const Point3D& fromCoords = request.coordinates;
		assert(fromVisionCuboidIndex.exists());
		SmallSet<ActorReference>& canSee = request.canSee;
		SmallSet<ActorReference>& canBeSeenBy = request.canBeSeenBy;
		// Collect results in a vector rather then a set to prevent cache thrashing.
		ActorReference previousFoundActor;
		Sphere visionSphere{fromCoords, m_largestRange.toFloat()};
		VisionCuboidSetSIMD visionCuboids = m_area.m_visionCuboids.query(visionSphere, m_area.getBlocks());
		const Facing4 facing = request.facing;
		const OccupiedBlocksForHasShape occupied = request.occupied;
		m_area.m_octTree.query(visionSphere, &visionCuboids, [&](const LocationBucket& bucket)
		{
			const auto& [actors, canSeeAndCanBeSeenBy] = bucket.visionRequestQuery(m_area, fromCoords, facing, rangeSquared, fromVisionCuboidIndex, visionCuboids, occupied, m_largestRange);
			for(uint i = 0; i < actors->size(); ++i)
			{
				if(canSeeAndCanBeSeenBy.col(i)[0])
					canSee.insertNonunique((*actors)[i]);
				if(canSeeAndCanBeSeenBy.col(i)[1])
					//TODO: change calls to insert rather then insertNonUnique by preventing duplicates.
					canBeSeenBy.insertNonunique((*actors)[i]);
			}
		});
	}
	// Finalize result by deduplicating.
	// Do this in a seperate loop to avoid thrashing the CPU cache in the primary one.
	for(auto iter = m_data.begin() + begin; iter != m_data.begin() + end; ++iter)
	{
		VisionRequest& request = *iter;
		request.canSee.removeDuplicatesAndValue(request.actor);
		request.canBeSeenBy.makeUnique();
		//TODO: maybe create canNoLongerSee / canNoLongerBeSeen here?
	}
}
void VisionRequests::readStep()
{
	uint index = 0;
	uint size = m_data.size();
	std::vector<std::pair<uint, uint>> ranges;
	while(index != size)
	{
		uint end = std::min(size, index + Config::visionThreadingBatchSize);
		ranges.emplace_back(index, end);
		index = end;
	}
	// TODO: store hilbert number in request?
	m_data.sort([&](const VisionRequest& a, const VisionRequest& b){ return a.coordinates.hilbertNumber() < b.coordinates.hilbertNumber(); });
	m_area.m_octTree.maybeSort();
	//#pragma omp parallel for
	for(auto [start, end] : ranges)
		readStepSegment(start, end);
}
void VisionRequests::writeStep()
{
	Actors& actors = m_area.getActors();
	for(VisionRequest& request : m_data)
	{
		ActorIndex index = request.actor.getIndex(actors.m_referenceData);
		SmallSet<ActorReference> noLongerCanSee = std::move(actors.vision_getCanSee(index));
		noLongerCanSee.eraseAll(request.canSee);
		SmallSet<ActorReference> noLongerCanBeSeenBy = std::move(actors.vision_getCanBeSeenBy(index));
		noLongerCanBeSeenBy.eraseAll(request.canBeSeenBy);
		actors.vision_setCanSee(index, std::move(request.canSee));
		actors.vision_setCanBeSeenBy(index, std::move(request.canBeSeenBy));
		for(const ActorReference& ref : noLongerCanSee)
			actors.vision_maybeSetNoLongerCanSee(index, ref);
		for(const ActorReference& ref : noLongerCanBeSeenBy)
			actors.vision_maybeSetNoLongerCanBeSeenBy(index, ref);
	}
}
void VisionRequests::doStep()
{
	readStep();
	writeStep();
	m_data.clear();
}
void VisionRequests::clear()
{
	while(m_data.size())
		cancelIfExists(m_data.back().actor);
}
void VisionRequests::maybeGenerateRequestsForAllWithLineOfSightTo(const BlockIndex& block)
{
	maybeGenerateRequestsForAllWithLineOfSightToAny({block});
}
void VisionRequests::maybeGenerateRequestsForAllWithLineOfSightToAny(const std::vector<BlockIndex>& blockSet)
{
	Blocks& blocks = m_area.getBlocks();
	struct CandidateData{
		ActorReference actor;
		DistanceInBlocks rangeSquared;
		Point3D coordinates;
		VisionCuboidId cuboid;
		Facing4 facing;
	};
	struct BlockData
	{
		BlockIndex index;
		Point3D coordinates;
	};
	std::vector<CandidateData> candidates;
	SmallSet<ActorReference> results;
	SmallSet<uint> locationBucketIndices;

	std::vector<BlockData> blockDataStore;
	std::ranges::transform(blockSet.begin(), blockSet.end(), std::back_inserter(blockDataStore),
		[&](const BlockIndex& block) -> BlockData {
			return {block, blocks.getCoordinates(block)};
		});
	int minX = std::ranges::min_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.x(); })->coordinates.x().get();
	int maxX = std::ranges::max_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.x(); })->coordinates.x().get();
	int minY = std::ranges::min_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.y(); })->coordinates.y().get();
	int maxY = std::ranges::max_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.y(); })->coordinates.y().get();
	int minZ = std::ranges::min_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.z(); })->coordinates.z().get();
	int maxZ = std::ranges::max_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.z(); })->coordinates.z().get();
	Cuboid cuboid(
		{DistanceInBlocks::create(maxX), DistanceInBlocks::create(maxY), DistanceInBlocks::create(maxZ)},
		{DistanceInBlocks::create(minX), DistanceInBlocks::create(minY), DistanceInBlocks::create(minZ)}
	);
	Point3DSet points = blocks.getCoordinateSet(blockSet);
	m_area.m_octTree.query(cuboid, nullptr, [&](const LocationBucket& bucket){
		const auto& [actorsResult, canBeSeenBy] = bucket.anyCanBeSeenQuery(m_area, cuboid, points);
		for(auto i = LocationBucketContentsIndex::create(0); i < actorsResult->size(); ++i)
			if(canBeSeenBy[i.get()])
				results.insert((*actorsResult)[i]);
	});
	for(ActorReference actor : results)
		maybeCreate(actor);
}
void VisionRequests::maybeUpdateCuboid(const ActorReference& actor, const VisionCuboidId& newCuboid)
{
	auto iter = m_data.findIf([&](const VisionRequest& request) { return request.actor == actor; });
	if(iter != m_data.end())
	{
		iter->cuboid = newCuboid;
	}
}
bool VisionRequests::maybeUpdateRange(const ActorReference& actor, const DistanceInBlocks& range)
{
	auto iter = m_data.findIf([&](const VisionRequest& request) { return request.actor == actor; });
	if(iter == m_data.end())
		return false;
	iter->range = range;
	return true;
}
bool VisionRequests::maybeUpdateLocation(const ActorReference& actor, const BlockIndex& location)
{
	auto iter = m_data.findIf([&](const VisionRequest& request) { return request.actor == actor; });
	Blocks& blocks = m_area.getBlocks();
	if(iter == m_data.end())
		return false;
	iter->coordinates = blocks.getCoordinates(location);
	iter->location = location;
	iter->cuboid = m_area.m_visionCuboids.getVisionCuboidIndexForBlock(location);
	return true;
}