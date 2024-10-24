#include "visionFacade.h"
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

VisionFacade::VisionFacade( )
{
	VisionFacadeIndex reserve = VisionFacadeIndex::create(Config::visionFacadeReservationSize);
	m_actors.reserve(reserve);
	m_ranges.reserve(reserve);
	m_locations.reserve(reserve);
	m_results.reserve(reserve);
	// Four results fit on a  cache line, by ensuring that the number of tasks per thread is a multiple of 4 we prevent false shareing.
	assert(Config::visionThreadingBatchSize % 4 == 0);
}
void VisionFacade::setArea(Area& area)
{
	//VisionFacade is a member of an array so it can't have arguments passed to it's constructor
	m_area = &area;
}
void VisionFacade::addActor( const ActorIndex& actor)
{
	assert(m_area != nullptr);
	Actors& actorData = m_area->getActors();
	assert(!actorData.vision_hasFacade(actor));
	assert(m_ranges.size() == m_actors.size() && m_locations.size() == m_actors.size());
	m_actors.add(actor);
	m_ranges.add(actorData.vision_getRange(actor));
	m_locations.add(actorData.getLocation(actor));
	m_results.add();
	auto& hasFacade = m_area->getActors().vision_getHasVisionFacade(actor);
	hasFacade.m_visionFacade = this;
	hasFacade.m_index = VisionFacadeIndex::create(m_actors.size() - 1);
}
void VisionFacade::removeActor( const ActorIndex& actor)
{
	Actors& actorData = m_area->getActors();
	auto [visionFacade, index] = actorData.vision_getFacadeWithIndex(actor);
	assert(index.exists());
	assert(visionFacade == this);
	remove(index);
}
void VisionFacade::remove( const VisionFacadeIndex& index)
{
	assert(m_actors.size() > index);
	m_actors.remove(index);
	m_ranges.remove(index);
	m_locations.remove(index);
	m_results.remove(index);
}
ActorIndex VisionFacade::getActor( const VisionFacadeIndex& index)  
{ 
	assert(m_actors.size() > index); 
	return m_actors[index]; 
}
BlockIndex VisionFacade::getLocation( const VisionFacadeIndex& index)  
{
       	assert(m_actors.size() > index); 
	Actors& actorData = m_area->getActors();
	assert(actorData.getLocation(m_actors[index]) == m_locations[index]);
	return m_locations[index]; 
}
DistanceInBlocks VisionFacade::getRange( const VisionFacadeIndex& index) const 
{ 
	assert(m_actors.size() > index); 
	Actors& actorData = m_area->getActors();
	assert(actorData.vision_getRange(m_actors[index]) == m_ranges[index]);
	return m_ranges[index]; 
}
ActorIndices& VisionFacade::getResults( const VisionFacadeIndex& index)
{ 
	assert(m_actors.size() > index); 
	return m_area->getActors().vision_getCanSee(m_actors[index]);
}
// Static.
DistanceInBlocks VisionFacade::taxiDistance(Point3D a, Point3D b)
{
	return DistanceInBlocks::create(
		std::abs((int)a.x.get() - (int)b.x.get()) + 
		std::abs((int)a.y.get() - (int)b.y.get()) +
		std::abs((int)a.z.get() - (int)b.z.get())
	);
}
void VisionFacade::updateLocation( const VisionFacadeIndex& index, const BlockIndex& location)
{
	assert(m_area->getActors().getLocation(m_actors[index]) == location);
	m_locations[index] = location;
}
void VisionFacade::updateRange( const VisionFacadeIndex& index,  const DistanceInBlocks& range)
{
	assert(m_area->getActors().vision_getRange(m_actors[index]) == range);
	m_ranges[index] = range;
}
void VisionFacade::readStepSegment( const VisionFacadeIndex& begin,  const VisionFacadeIndex& end)
{
	assert(m_area != nullptr);
	Blocks& blocks = m_area->getBlocks();
	for(VisionFacadeIndex viewerIndex = begin; viewerIndex < end; ++viewerIndex)
	{
		DistanceInBlocks range = getRange(viewerIndex);
		BlockIndex fromIndex = getLocation(viewerIndex);
		ActorIndices& result = getResults(viewerIndex);
		result.clear();
		// Collect results in a vector rather then a set to prevent cache thrashing.
		Point3D fromCoords = blocks.getCoordinates(fromIndex);
		LocationBuckets& locationBuckets = m_area->m_locationBuckets;
		VisionCuboidId fromVisionCuboidId = m_area->m_visionCuboids.getIdFor(fromIndex);
		assert(fromVisionCuboidId != 0);
		// Define a cuboid of locationBuckets around the watcher.
		DistanceInBuckets endX = DistanceInBuckets::create(std::min(((fromCoords.x + range).get() / Config::locationBucketSize.get() + 1), locationBuckets.m_maxX.get()));
		DistanceInBuckets beginX = DistanceInBuckets::create(std::max(0, int32_t(fromCoords.x.get()) - int32_t(range.get()))) / Config::locationBucketSize.get();
		DistanceInBuckets endY = DistanceInBuckets::create(std::min(((fromCoords.y + range).get() / Config::locationBucketSize.get() + 1), locationBuckets.m_maxY.get()));
		DistanceInBuckets beginY = DistanceInBuckets::create(std::max(0, int32_t(fromCoords.y.get()) - int32_t(range.get()))) / Config::locationBucketSize.get();
		DistanceInBuckets endZ = DistanceInBuckets::create(std::min(((fromCoords.z + range).get() / Config::locationBucketSize.get() + 1), locationBuckets.m_maxZ.get()));
		DistanceInBuckets beginZ = DistanceInBuckets::create(std::max(0, int32_t(fromCoords.z.get()) - int32_t(range.get()))) / Config::locationBucketSize.get();
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
						for(BlockIndex toIndex : bucket.m_blocksMultiTileActors[i])
						{
							Point3D toCoords = blocks.getCoordinates(toIndex);
							// Refine bucket cuboid actors into sphere with radius == range.
							if(taxiDistance(fromCoords, toCoords) <= range)
							{
								// Check sightlines.
								VisionCuboidId toVisionCuboidId = m_area->m_visionCuboids.getIdFor(fromIndex);
								assert(toVisionCuboidId != 0);
								if(
									fromVisionCuboidId == toVisionCuboidId ||
									m_area->m_opacityFacade.hasLineOfSight(fromIndex, fromCoords, toIndex, toCoords)
								  )
								{
									result.add(bucket.m_actorsMultiTile[i]);
									break;
								}
							}
						}
					}
					for(uint16_t i = 0; i < bucket.m_actorsSingleTile.size(); i++)
					{
						BlockIndex toIndex = bucket.m_blocksSingleTileActors[i];
						Point3D toCoords = blocks.getCoordinates(toIndex);
						// Refine bucket cuboid actors into sphere with radius == range.
						if(taxiDistance(toCoords, fromCoords) <= range)
						{
							// Check sightlines.
							VisionCuboidId toVisionCuboidId = m_area->m_visionCuboids.getIdFor(toIndex);
							assert(toVisionCuboidId != 0);
							if(
								fromVisionCuboidId == toVisionCuboidId ||
								m_area->m_opacityFacade.hasLineOfSight(fromIndex, fromCoords, toIndex, toCoords)
							  )
								result.add(bucket.m_actorsSingleTile[i]);
						}
					}
				}
	}
	// Finalize result.
	// Do this in a seperate loop to avoid thrashing the CPU cache in the primary one.
	for(VisionFacadeIndex viewerIndex = begin; viewerIndex < end; ++viewerIndex)
		getResults(viewerIndex).removeDuplicatesAndValue(getActor(viewerIndex));
}
void VisionFacade::doStep()
{
	VisionFacadeIndex index = VisionFacadeIndex::create(0);
	VisionFacadeIndex actorsSize = VisionFacadeIndex::create(m_actors.size());
	std::vector<std::pair<VisionFacadeIndex, VisionFacadeIndex>> ranges;
	while(index != actorsSize)
	{
		VisionFacadeIndex end = std::min(actorsSize, index + Config::visionThreadingBatchSize);
		ranges.emplace_back(index, end);
		index = end;
	}
	//#pragma omp parallel for
	for(auto [start, end] : ranges)
		readStepSegment(start, end);
	for(VisionFacadeIndex index = VisionFacadeIndex::create(0); index < actorsSize; ++index)
		m_area->getActors().vision_swap(getActor(index), getResults(index));
}
void VisionFacade::clear()
{
	while(m_actors.size())
		removeActor(ActorIndex::create(m_actors.size() - 1));
}
// Buckets.
VisionFacadeBuckets::VisionFacadeBuckets(Area& area) : m_area(area)
{
	for(VisionFacade& visionFacade : m_data)
		visionFacade.setArea(area);
}
VisionFacade& VisionFacadeBuckets::facadeForActor( const ActorIndex& actor)
{
	ActorId id = m_area.getActors().getId(actor);
	return m_data[id.get() % Config::actorDoVisionInterval];
}
void VisionFacadeBuckets::add( const ActorIndex& actor)
{
	facadeForActor(actor).addActor(actor);
}
void VisionFacadeBuckets::remove( const ActorIndex& actor)
{
	facadeForActor(actor).removeActor(actor);
}
void VisionFacadeBuckets::clear()
{
	for(VisionFacade& visionFacade : m_data)
		visionFacade.clear();
}
VisionFacade& VisionFacadeBuckets::getForStep(Step step)
{
	return m_data[step.get() % Config::actorDoVisionInterval];
}
/// Handle.
void HasVisionFacade::clearVisionFacade()
{
	assert(m_visionFacade);
	if(!empty())
		clear();
	m_visionFacade = nullptr;
}
void HasVisionFacade::create(Area& area,  const ActorIndex& actor)
{
	assert(empty());
	assert(!m_visionFacade);
	area.m_visionFacadeBuckets.add(actor);
	assert(!empty());
	assert(m_visionFacade);
}
void HasVisionFacade::updateRange( const DistanceInBlocks& range)
{
	assert(m_visionFacade);
	m_visionFacade->updateRange(m_index, range);
}
void HasVisionFacade::updateLocation(const BlockIndex& location)
{
	assert(m_visionFacade);
	m_visionFacade->updateLocation(m_index, location);
}
void HasVisionFacade::clear()
{
	if(!empty())
	{
		assert(m_visionFacade != nullptr);
		m_visionFacade->remove(m_index);
		m_visionFacade = nullptr;
		m_index.clear();
	}
}
