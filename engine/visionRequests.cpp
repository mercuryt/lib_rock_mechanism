#include "visionRequests.h"
#include "area.h"
#include "config.h"
#include "locationBuckets.h"
#include "simulation.h"
#include "types.h"
#include "util.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "strongInteger.hpp"
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
	m_data.emplace(m_area.getBlocks().getCoordinates(location), location, m_area.m_visionCuboids.getIdFor(location), actor, actors.vision_getRange(index));
	if(m_data.back().range > m_largestRange)
		m_largestRange = m_data.back().range;
}
void VisionRequests::maybeCreate(const ActorReference& actor)
{
	if(!m_data.containsAny([&](const auto& data){ return data.actor == actor; }))
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
		const BlockIndex& fromIndex = request.location;
		const VisionCuboidId& fromVisionCuboidId = request.cuboid;
		const Point3D& fromCoords = request.coordinates;
		assert(fromVisionCuboidId != 0);
		SmallSet<ActorReference>& canSee = request.canSee;
		SmallSet<ActorReference>& canBeSeenBy = request.canBeSeenBy;
		// Collect results in a vector rather then a set to prevent cache thrashing.
		ActorReference previousFoundActor;
		m_area.m_octTree.query({fromCoords, m_largestRange}, [&](const LocationBucketData& data){
			if(previousFoundActor.exists() && data.actor == previousFoundActor)
				// Multi tile actor has already been found.
				return;
			const Point3D& toCoords = data.coordinates;
			const VisionCuboidId& toVisionCuboidId = data.cuboid;
			DistanceInBlocks distanceSquared = fromCoords.distanceSquared(toCoords);
			bool inRangeToSee = distanceSquared <= rangeSquared;
			// A bit of logical asymetry here: If a multi tile object moves it's vision is counted only from it's location, but if another actor moves into a position where it can see a tile of the multi tile which is not it's location and the other actor is in the multi tile actors range it will be marked as seen, even though it would not be seen at the same position if it had not moved due to either distance or occulsion from the multi tile actor's primary location.
			bool inRangeToBeSeen = distanceSquared <= data.visionRangeSquared;
			if(inRangeToBeSeen || inRangeToSee)
				if (
					fromVisionCuboidId == toVisionCuboidId ||
					// Check sightlines.
					m_area.m_opacityFacade.hasLineOfSight(fromIndex, fromCoords, toCoords))
				{
					if(inRangeToSee)
					{
						if(inRangeToBeSeen)
							previousFoundActor = data.actor;
						canSee.insertNonunique(data.actor);
					}
					if(inRangeToBeSeen)
						canBeSeenBy.insertNonunique(data.actor);
				}
		});
	}
	// Finalize result.
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
	uint actorsSize = m_data.size();
	std::vector<std::pair<uint, uint>> ranges;
	while(index != actorsSize)
	{
		uint end = std::min(actorsSize, index + Config::visionThreadingBatchSize);
		ranges.emplace_back(index, end);
		index = end;
	}
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
		BlockIndex location;
	};
	struct BlockData
	{
		BlockIndex index;
		Point3D coordinates;
		VisionCuboidId cuboid;
	};
	std::vector<CandidateData> candidates;
	SmallSet<ActorReference> results;
	SmallSet<uint> locationBucketIndices;
	
	std::vector<BlockData> blockDataStore;
	std::ranges::transform(blockSet.begin(), blockSet.end(), std::back_inserter(blockDataStore),
		[&](const BlockIndex& block) -> BlockData {
			return {block, blocks.getCoordinates(block), m_area.m_visionCuboids.getIdFor(block)};
		});
	int minX = std::ranges::min_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.x; })->coordinates.x.get();
	int maxX = std::ranges::max_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.x; })->coordinates.x.get();
	int minY = std::ranges::min_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.y; })->coordinates.y.get();
	int maxY = std::ranges::max_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.y; })->coordinates.y.get();
	int minZ = std::ranges::min_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.z; })->coordinates.z.get();
	int maxZ = std::ranges::max_element(blockDataStore, {}, [&](const BlockData& c) { return c.coordinates.z; })->coordinates.z.get();
	Cuboid cuboid(blocks, blocks.getIndex_i(maxX, maxY, maxZ), blocks.getIndex_i(minX, minY, minZ));
	ActorReference lastSeen;
	m_area.m_octTree.query(m_area, cuboid, [&](const LocationBucketData& data){ 
		// Skip multi tile actor tiles after the first. The first is assumed to be the source of vision.
		// The data is kept sorted by actor so multi tile records should always be contiguous.
		if(lastSeen.exists() && lastSeen == data.actor)
			return;
		// Skip any already recorded.
		auto found = std::ranges::find(candidates, data.actor, &CandidateData::actor);
		if(found != candidates.end())
			return;
		BlockIndex location = blocks.getIndex(data.coordinates);
		candidates.emplace_back(data.actor, data.visionRangeSquared, data.coordinates, data.cuboid, location);
		lastSeen = data.actor;
	});
	for(const CandidateData& candidate : candidates)
	{
		for(const BlockData& blockData : blockDataStore)
		{
			DistanceInBlocks distanceSquared = candidate.coordinates.distanceSquared(blockData.coordinates);
			// Exclude actors too far away to see.
			if(distanceSquared > candidate.rangeSquared)
				continue;
			if(
				// At some threashold it is not worth it to check line of sight for so many changed blocks, better to just create the vision requests instead.
				blockDataStore.size() >= Config::minimumSizeOfGroupOfMovingBlocksWhichSkipLineOfSightChecksForMakingVisionRequests ||
				blockData.cuboid == candidate.cuboid ||
				m_area.m_opacityFacade.hasLineOfSight(blockData.index, blockData.coordinates, candidate.coordinates)
			)
			{
				results.insert(candidate.actor);
				break;
			}
		}
	}
	for(ActorReference actor : results)
		maybeCreate(actor);
}
void VisionRequests::maybeUpdateCuboid(const ActorReference& actor, const VisionCuboidId& oldCuboid, const VisionCuboidId& newCuboid)
{
	auto iter = m_data.findIf([&](const VisionRequest& request) { return request.actor == actor; });
	if(iter != m_data.end())
	{
		assert(iter->cuboid == oldCuboid || iter->cuboid == newCuboid);
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
	iter->cuboid = m_area.m_visionCuboids.getIdFor(location); 
	return true;
}