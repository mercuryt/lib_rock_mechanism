#include "visionRequests.h"
#include "../area/area.h"
#include "../config/config.h"
#include "../dataStructures/locationBuckets.h"
#include "../simulation/simulation.h"
#include "../numericTypes/types.h"
#include "../util.h"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../geometry/sphere.h"
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
	const Point3D& location = actors.getLocation(index);
	const VisionCuboidId& visionCuboidIndex = m_area.m_visionCuboids.getVisionCuboidIndexForPoint(location);
	m_data.emplace(location, visionCuboidIndex, actor, actors.vision_getRange(index), actors.getFacing(index), actors.getOccupied(index));
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
		m_largestRange = Distance::create(0);
	else if(largestRange)
		m_largestRange = std::max_element(m_data.begin(), m_data.end(), [](const VisionRequest& a, const VisionRequest& b) { return a.actor < b.actor; })->range;
}
void VisionRequests::readStepSegment(const int& begin, const int& end)
{
	for(auto iter = m_data.begin() + begin; iter != m_data.begin() + end; ++iter)
	{
		VisionRequest& request = *iter;
		const DistanceSquared& rangeSquared = request.range.squared();
		const VisionCuboidId& fromVisionCuboidIndex = request.cuboid;
		assert(fromVisionCuboidIndex.exists());
		SmallSet<ActorReference>& canSee = request.canSee;
		SmallSet<ActorReference>& canBeSeenBy = request.canBeSeenBy;
		// Collect results in a vector rather then a set to prevent cache thrashing.
		const Sphere visionSphere{request.location, m_largestRange.toFloat()};
		const VisionCuboidSetSIMD visionCuboids = m_area.m_visionCuboids.query(visionSphere);
		const Facing4 facing = request.facing;
		const CuboidSet& occupied = request.occupied;
		m_area.m_octTree.query(visionSphere, &visionCuboids, [&](const LocationBucket& bucket)
		{
			const auto& [actors, canSeeAndCanBeSeenBy] = bucket.visionRequestQuery(m_area, request.location, facing, rangeSquared, fromVisionCuboidIndex, visionCuboids, occupied, m_largestRange);
			for(size_t i = 0; i < actors->size(); ++i)
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
	int index = 0;
	int size = m_data.size();
	std::vector<std::pair<int, int>> ranges;
	while(index != size)
	{
		int end = std::min(size, index + Config::visionThreadingBatchSize);
		ranges.emplace_back(index, end);
		index = end;
	}
	// TODO: store hilbert number in request?
	m_data.sort([&](const VisionRequest& a, const VisionRequest& b){ return a.location.hilbertNumber() < b.location.hilbertNumber(); });
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
		const auto [noLongerCanSee, canNowSee] = actors.vision_getCanSee(index).getDeltaPair(request.canSee);
		const auto [noLongerCanBeSeenBy, canNowBeSeenBy] = actors.vision_getCanBeSeenBy(index).getDeltaPair(request.canBeSeenBy);
		// CanSee / CanBeSeenBy.
		// Update for this actor.
		actors.vision_setCanSee(index, std::move(request.canSee));
		actors.vision_setCanBeSeenBy(index, std::move(request.canBeSeenBy));
		// Update for other actors.
		for(const ActorReference& ref : noLongerCanSee)
		{
			// Actor looses sight of ref.
			const ActorIndex other = ref.getIndex(actors.m_referenceData);
			actors.vision_setNoLongerCanBeSeenBy(other, request.actor);
		}
		const FactionId& faction = actors.getFaction(index);
		SmallSet<FactionId> enemyFactions;
		if(faction.exists())
		 	enemyFactions = m_area.m_simulation.m_hasFactions.getEnemies(faction);
		// Civilians free from enemies on sight, soldiers evaluate the malice delta and do a test of courage first.
		// TODO: Civilians shouldn't flee from enemies who are much weaker then themselves.
		const bool isNotFleeingAndIsNotSoldier = !actors.combat_isFleeing(index) && !actors.soldier_is(index);
		for(const ActorReference& ref : canNowSee)
		{
			// Ref enters actor's line of sight.
			const ActorIndex other = ref.getIndex(actors.m_referenceData);
			actors.vision_setCanBeSeenBy(other, request.actor);
			if(isNotFleeingAndIsNotSoldier && enemyFactions.contains(actors.getFaction(other)))
				actors.combat_flee(index);
		}
		for(const ActorReference& ref : noLongerCanBeSeenBy)
		{
			// Ref looses sight of actor.
			const ActorIndex other = ref.getIndex(actors.m_referenceData);
			actors.vision_setNoLongerCanBeSeenBy(other, request.actor);
		}
		for(const ActorReference& ref : canNowBeSeenBy)
		{
			// Actor enter's ref's line of sight.
			const ActorIndex other = ref.getIndex(actors.m_referenceData);
			actors.vision_setCanBeSeenBy(other, request.actor);
		}
		// -OnSight.
		if(!canNowSee.empty())
			actors.onSight_get(index).execute(m_area, request.actor, canNowSee);
		m_area.m_hasOnSight.maybeExecute(faction, m_area, request.actor, canNowSee);
		// Iterate this twice to be sure that all canSee/canBeSeen relationships are updated before any callbacks are run.
		for(const ActorReference& ref : canNowBeSeenBy)
		{
			const ActorIndex other = ref.getIndex(actors.m_referenceData);
			actors.onSight_get(other).execute(m_area, ref, request.actor);
			const FactionId& otherFaction = actors.getFaction(other);
			m_area.m_hasOnSight.maybeExecute(otherFaction, m_area, ref, request.actor);
		}
		// TODO: AreaHasOnSightForFaction.
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
void VisionRequests::maybeGenerateRequestsForAllWithLineOfSightTo(const Point3D& point)
{
	maybeGenerateRequestsForAllWithLineOfSightToAny({point});
}
void VisionRequests::maybeGenerateRequestsForAllWithLineOfSightToAny(const std::vector<Point3D>& pointSet)
{
	struct CandidateData{
		ActorReference actor;
		DistanceSquared rangeSquared;
		Point3D coordinates;
		VisionCuboidId cuboid;
		Facing4 facing;
	};
	struct PointData
	{
		Point3D index;
		Point3D coordinates;
	};
	std::vector<CandidateData> candidates;
	SmallSet<ActorReference> results;
	SmallSet<int> locationBucketIndices;

	std::vector<PointData> pointDataStore;
	std::ranges::transform(pointSet.begin(), pointSet.end(), std::back_inserter(pointDataStore),
		[&](const Point3D& point) -> PointData {
			return {point, point};
		});
	int minX = std::ranges::min_element(pointDataStore, {}, [&](const PointData& c) { return c.coordinates.x(); })->coordinates.x().get();
	int maxX = std::ranges::max_element(pointDataStore, {}, [&](const PointData& c) { return c.coordinates.x(); })->coordinates.x().get();
	int minY = std::ranges::min_element(pointDataStore, {}, [&](const PointData& c) { return c.coordinates.y(); })->coordinates.y().get();
	int maxY = std::ranges::max_element(pointDataStore, {}, [&](const PointData& c) { return c.coordinates.y(); })->coordinates.y().get();
	int minZ = std::ranges::min_element(pointDataStore, {}, [&](const PointData& c) { return c.coordinates.z(); })->coordinates.z().get();
	int maxZ = std::ranges::max_element(pointDataStore, {}, [&](const PointData& c) { return c.coordinates.z(); })->coordinates.z().get();
	Cuboid cuboid(
		{Distance::create(maxX), Distance::create(maxY), Distance::create(maxZ)},
		{Distance::create(minX), Distance::create(minY), Distance::create(minZ)}
	);
	Point3DSet points = Point3DSet::fromPointSet(pointSet);
	m_area.m_octTree.query(cuboid, nullptr, [&](const LocationBucket& bucket){
		const auto& [actorsResult, canBeSeenBy] = bucket.anyCanBeSeenQuery(m_area, cuboid, points);
		for(auto i = LocationBucketContentsIndex::create(0); i < actorsResult->size(); ++i)
			if(canBeSeenBy[i.get()])
				// Multi tile actors will be recorded more then once in bucket. Only record them once in results.
				results.maybeInsert((*actorsResult)[i]);
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
bool VisionRequests::maybeUpdateRange(const ActorReference& actor, const Distance& range)
{
	auto iter = m_data.findIf([&](const VisionRequest& request) { return request.actor == actor; });
	if(iter == m_data.end())
		return false;
	iter->range = range;
	return true;
}
bool VisionRequests::maybeUpdateLocation(const ActorReference& actor, const Point3D& location)
{
	auto iter = m_data.findIf([&](const VisionRequest& request) { return request.actor == actor; });
	if(iter == m_data.end())
		return false;
	iter->location = location;
	iter->cuboid = m_area.m_visionCuboids.getVisionCuboidIndexForPoint(location);
	return true;
}