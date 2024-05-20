#include "visionFacade.h"
#include "actor.h"
#include "area.h"
#include "config.h"
#include "locationBuckets.h"
#include "simulation.h"
#include "types.h"
#include "visionUtil.h"
#include <cstddef>

VisionFacade::VisionFacade( )
{
	VisionFacadeIndex reserve = Config::visionFacadeReservationSize;
	m_actors.reserve(reserve);
	m_ranges.reserve(reserve);
	m_locations.reserve(reserve);
	m_results.reserve(reserve);
}
void VisionFacade::setArea(Area& area)
{
	m_area = &area;
}
void VisionFacade::add(Actor& actor)
{
	assert(actor.m_canSee.m_hasVisionFacade.m_index == VISION_FACADE_INDEX_MAX);
	assert(actor.m_canSee.m_hasVisionFacade.m_visionFacade == nullptr);
	VisionFacadeIndex index = m_actors.size();
	actor.m_canSee.m_hasVisionFacade.m_index = index;
	actor.m_canSee.m_hasVisionFacade.m_visionFacade = this;
	assert(m_results.size() == m_actors.size() && m_ranges.size() == m_actors.size() && m_locations.size() == m_actors.size());
	m_actors.push_back(&actor);
	m_ranges.push_back(actor.m_canSee.getRange());
	m_locations.push_back(actor.m_location);
	m_results.push_back({});
}
void VisionFacade::remove(Actor& actor)
{
	assert(actor.m_canSee.m_hasVisionFacade.m_index != VISION_FACADE_INDEX_MAX);
	assert(actor.m_canSee.m_hasVisionFacade.m_visionFacade == this);
	VisionFacadeIndex index = actor.m_canSee.m_hasVisionFacade.m_index;
	remove(index);
}
void VisionFacade::remove(VisionFacadeIndex index)
{
	assert(m_actors.size() > index);
	m_actors.at(index)->m_canSee.m_hasVisionFacade.m_index = VISION_FACADE_INDEX_MAX;
	m_actors.at(index)->m_canSee.m_hasVisionFacade.m_visionFacade = nullptr;
	util::removeFromVectorByIndexUnordered(m_actors, index);
	util::removeFromVectorByIndexUnordered(m_ranges, index);
	util::removeFromVectorByIndexUnordered(m_locations, index);
	util::removeFromVectorByIndexUnordered(m_results, index);
	if(m_actors.size() > index)
		m_actors.at(index)->m_canSee.m_hasVisionFacade.m_index = index;
}
Actor& VisionFacade::getActor(VisionFacadeIndex index)  
{ 
	assert(m_actors.size() > index); 
	return *m_actors.at(index); 
}
Block& VisionFacade::getLocation(VisionFacadeIndex index)  
{
       	assert(m_actors.size() > index); 
	assert(m_actors.at(index)->m_location == m_locations.at(index));
	return *m_locations.at(index); 
}
DistanceInBlocks VisionFacade::getRange(VisionFacadeIndex index) const 
{ 
	assert(m_actors.size() > index); 
	assert(m_actors.at(index)->m_canSee.getRange() == m_ranges.at(index));
	return m_ranges.at(index); 
}
// Static.
DistanceInBlocks VisionFacade::taxiDistance(Point3D a, Point3D b)
{
	return 
		std::abs((int)a.x - (int)b.x) + 
		std::abs((int)a.y - (int)b.y) +
		std::abs((int)a.z - (int)b.z);
}
std::unordered_set<Actor*>& VisionFacade::getResults(VisionFacadeIndex index)
{ 
	assert(m_actors.size() > index); 
	assert(m_results.size() == m_actors.size()); 
	return m_results.at(index); 
}
void VisionFacade::updateLocation(VisionFacadeIndex index, Block& location)
{
	assert(m_actors.at(index)->m_location == &location);
	m_locations.at(index) = &location;
}
void VisionFacade::updateRange(VisionFacadeIndex index, DistanceInBlocks range)
{
	assert(m_actors.at(index)->m_canSee.getRange() == range);
	m_ranges.at(index) = range;
}
void VisionFacade::readStepSegment(VisionFacadeIndex begin, VisionFacadeIndex end)
{
	for(VisionFacadeIndex viewerIndex = begin; viewerIndex < end; ++viewerIndex)
	{
		DistanceInBlocks range = getRange(viewerIndex);
		Block& from = getLocation(viewerIndex);
		BlockIndex fromIndex = m_area->getBlockIndex(from);
		Point3D fromCoords = m_area->getCoordinatesForIndex(fromIndex);
		LocationBuckets& locationBuckets = m_area->m_hasActors.m_locationBuckets;
		std::unordered_set<Actor*>& result = getResults(viewerIndex);
		result.clear();
		// Define a cuboid of locationBuckets around the watcher.
		DistanceInBuckets endX = std::min(((fromCoords.x + range) / Config::locationBucketSize + 1), locationBuckets.m_maxX);
		DistanceInBuckets beginX = std::max(0, int32_t(fromCoords.x - range)) / Config::locationBucketSize;
		DistanceInBuckets endY = std::min(((fromCoords.y + range) / Config::locationBucketSize + 1), locationBuckets.m_maxY);
		DistanceInBuckets beginY = std::max(0, int32_t(fromCoords.y - range)) / Config::locationBucketSize;
		DistanceInBuckets endZ = std::min(((fromCoords.z + range) / Config::locationBucketSize + 1), locationBuckets.m_maxZ);
		DistanceInBuckets beginZ = std::max(0, int32_t(fromCoords.z - range)) / Config::locationBucketSize;
		// Iterate defined cuboid of buckets.
		for(DistanceInBuckets x = beginX; x != endX; ++x)
			for(DistanceInBuckets y = beginY; y != endY; ++y)
				for(DistanceInBuckets z = beginZ; z != endZ; ++z)
				{
					assert(x * y * z < locationBuckets.m_buckets.size());
					// Iterate actors in the defined cuboid.
					LocationBucket& bucket = locationBuckets.get(x, y, z);
					for(uint16_t i = 0; i < bucket.m_actorsMultiTile.size(); i++)
					{
						for(Block* to : bucket.m_blocksMultiTileActors.at(i))
						{
							BlockIndex toIndex = m_area->getBlockIndex(*to);
							Point3D toCoords = m_area->getCoordinatesForIndex(toIndex);
							// Refine bucket cuboid actors into sphere with radius == range.
							if(taxiDistance(fromCoords, toCoords) <= range)
							{
								// Check sightlines.
								// TODO: this if should not be here. Use a template?
								if(from.m_area->m_visionCuboidsActive)
								{
									if(to->m_visionCuboid == from.m_visionCuboid || m_area->m_hasActors.m_opacityFacade.hasLineOfSight(fromIndex, fromCoords, toIndex, toCoords))
									{
										result.insert(bucket.m_actorsMultiTile.at(i));
										break;
									}
								}
								else if(m_area->m_hasActors.m_opacityFacade.hasLineOfSight(fromIndex, fromCoords, toIndex, toCoords))
								{
									result.insert(bucket.m_actorsMultiTile.at(i));
									break;
								}
							}
						}
					}
					for(uint16_t i = 0; i < bucket.m_actorsSingleTile.size(); i++)
					{
						Block* to = bucket.m_blocksSingleTileActors.at(i);
						BlockIndex toIndex = m_area->getBlockIndex(*to);
						Point3D toCoords = m_area->getCoordinatesForIndex(toIndex);
						// Refine bucket cuboid actors into sphere with radius == range.
						if(to->taxiDistance(from) <= range)
						{
							// Check sightlines.
							// TODO: this if should not be here. Use a template?
							if(from.m_area->m_visionCuboidsActive)
							{
								if(to->m_visionCuboid == from.m_visionCuboid || m_area->m_hasActors.m_opacityFacade.hasLineOfSight(fromIndex, fromCoords, toIndex, toCoords))
									result.insert(bucket.m_actorsSingleTile.at(i));
							}
							else if(m_area->m_hasActors.m_opacityFacade.hasLineOfSight(fromIndex, fromCoords, toIndex, toCoords))
								result.insert(bucket.m_actorsSingleTile.at(i));
						}
					}
				}
		// Actors don't see themselves.
		result.erase(&getActor(viewerIndex));
	}
}
void VisionFacade::readStep()
{
	VisionFacadeIndex index = 0;
	while(index != m_actors.size())
	{
		VisionFacadeIndex actorsSize = m_actors.size();
		VisionFacadeIndex end = std::min(actorsSize, index + Config::visionThreadingBatchSize);
		m_area->m_simulation.m_taskFutures.push_back(m_area->m_simulation.m_pool.submit([this, index, end]{ 
			readStepSegment(index, end);
		}));
		index = end;
	}
}
void VisionFacade::writeStep()
{
	for(VisionFacadeIndex index = 0; index < m_actors.size(); ++index)
		getActor(index).m_canSee.m_currently.swap(getResults(index));
}
void VisionFacade::clear()
{
	while(m_actors.size())
		remove(m_actors.size() - 1);
}
// Buckets.
VisionFacadeBuckets::VisionFacadeBuckets(Area& area)
{
	for(VisionFacade& visionFacade : m_data)
		visionFacade.setArea(area);
}
VisionFacade& VisionFacadeBuckets::facadeForActor(Actor& actor)
{
	return m_data[actor.m_id % Config::actorDoVisionInterval];
}
void VisionFacadeBuckets::add(Actor& actor)
{
	facadeForActor(actor).add(actor);
}
void VisionFacadeBuckets::remove(Actor& actor)
{
	facadeForActor(actor).remove(actor);
}
void VisionFacadeBuckets::clear()
{
	for(VisionFacade& visionFacade : m_data)
		visionFacade.clear();
}
VisionFacade& VisionFacadeBuckets::getForStep(Step step)
{
	return m_data[step % Config::actorDoVisionInterval];
}
/// Handle.
void HasVisionFacade::clearVisionFacade()
{
	assert(m_visionFacade);
	if(!empty())
		clear();
	m_visionFacade = nullptr;
}
void HasVisionFacade::create(Actor& actor)
{
	assert(empty());
	assert(!m_visionFacade);
	actor.m_location->m_area->m_hasActors.m_visionFacadeBuckets.add(actor);
	assert(!empty());
	assert(m_visionFacade);
}
void HasVisionFacade::updateRange(DistanceInBlocks range)
{
	assert(m_visionFacade);
	m_visionFacade->updateRange(m_index, range);
}
void HasVisionFacade::updateLocation(Block& location)
{
	assert(m_visionFacade);
	m_visionFacade->updateLocation(m_index, location);
}
void HasVisionFacade::clear()
{
	if(!empty())
	{
		assert(m_visionFacade);
		m_visionFacade->remove(m_index);
	}
}
