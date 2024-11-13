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
	m_data.reserve(reserve);
}
void VisionFacade::setArea(Area& area)
{
	//VisionFacade is a member of an array so it can't have arguments passed to it's constructor
	m_area = &area;
}
void VisionFacade::addActor(const ActorIndex& actor)
{
	assert(m_area != nullptr);
	Actors& actorData = m_area->getActors();
	assert(!actorData.vision_hasFacade(actor));
	assert(actorData.hasLocation(actor));
	assert(actorData.vision_canSeeAnything(actor));
	const BlockIndex& location = actorData.getLocation(actor);
	m_data.add(VisionFacadeData{
		.actor = actor,
		.location = location,
		.cuboid = m_area->m_visionCuboids.getIdFor(location),
		.coordinates = m_area->getBlocks().getCoordinates(location),
		.range = actorData.vision_getRange(actor),
		.results = {},
	});
	auto& hasFacade = m_area->getActors().vision_getHasVisionFacade(actor);
	assert(hasFacade.m_visionFacade == this);
	hasFacade.m_index = VisionFacadeIndex::create(m_data.size() - 1);
}
void VisionFacade::removeActor(const ActorIndex& actor)
{
	Actors& actorData = m_area->getActors();
	auto [visionFacade, index] = actorData.vision_getFacadeWithIndex(actor);
	assert(index.exists());
	assert(visionFacade == this);
	remove(index);
}
void VisionFacade::remove(const VisionFacadeIndex& index)
{
	assert(m_data.size() > index);
	m_data.remove(index);
}
ActorIndex VisionFacade::getActor(const VisionFacadeIndex& index) const
{ 
	assert(m_data.size() > index); 
	return m_data[index].actor; 
}
BlockIndex VisionFacade::getLocation(const VisionFacadeIndex& index) const
{
	assert(m_data.size() > index); 
	Actors& actorData = m_area->getActors();
	assert(actorData.getLocation(m_data[index].actor) == m_data[index].location);
	return m_data[index].location; 
}
DistanceInBlocks VisionFacade::getRange(const VisionFacadeIndex& index) const 
{ 
	assert(m_data.size() > index); 
	Actors& actorData = m_area->getActors();
	assert(actorData.vision_getRange(m_data[index].actor) == m_data[index].range);
	return m_data[index].range; 
}
VisionCuboidId VisionFacade::getCuboid(const VisionFacadeIndex& index) const
{ 
	const VisionCuboidId& cuboid = m_area->m_visionCuboids.getIdFor(m_data[index].location);
	assert(m_data[index].cuboid == cuboid);
	return m_data[index].cuboid;
}
Point3D VisionFacade::getFromCoords(const VisionFacadeIndex& index) const
{
	assert(m_area->getBlocks().getCoordinates(m_area->getActors().getLocation(m_data[index].actor)) == m_data[index].coordinates);
	return m_data[index].coordinates;
}
ActorIndices& VisionFacade::getResults(const VisionFacadeIndex& index) const
{ 
	assert(m_data.size() > index); 
	return m_area->getActors().vision_getCanSee(m_data[index].actor);
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
void VisionFacade::updateLocation(const VisionFacadeIndex& index, const BlockIndex& location)
{
	assert(m_area->getActors().getLocation(m_data[index].actor) == location);
	m_data[index].location = location;
	m_data[index].coordinates = m_area->getBlocks().getCoordinates(location);
	// don't use updateVisionCuboid here because we want to allow overwriting with the same value.
	m_data[index].cuboid = m_area->m_visionCuboids.getIdFor(location);
}
void VisionFacade::updateVisionCuboid(const VisionFacadeIndex& index, const VisionCuboidId& cuboid)
{
	// Allow redundant assignments.
	m_data[index].cuboid = cuboid;
}
void VisionFacade::updateRange(const VisionFacadeIndex& index,  const DistanceInBlocks& range)
{
	assert(m_area->getActors().vision_getRange(m_data[index].actor) == range);
	m_data[index].range = range;
}
void VisionFacade::readStepSegment(const VisionFacadeIndex& begin,  const VisionFacadeIndex& end)
{
	assert(m_area != nullptr);
	for(VisionFacadeIndex viewerIndex = begin; viewerIndex < end; ++viewerIndex)
	{
		const DistanceInBlocks& range = getRange(viewerIndex);
		const BlockIndex& fromIndex = getLocation(viewerIndex);
		const VisionCuboidId& fromVisionCuboidId = getCuboid(viewerIndex);
		const Point3D& fromCoords = getFromCoords(viewerIndex);
		assert(fromVisionCuboidId != 0);
		ActorIndices& result = getResults(viewerIndex);
		result.clear();
		// Collect results in a vector rather then a set to prevent cache thrashing.
		const LocationBuckets& locationBuckets = m_area->m_locationBuckets;
		// Define a cuboid of locationBuckets around the watcher.
		const DistanceInBuckets endX = DistanceInBuckets::create(std::min(((fromCoords.x + range).get() / Config::locationBucketSize.get() + 1), locationBuckets.m_maxX.get()));
		const DistanceInBuckets beginX = DistanceInBuckets::create(std::max(0, int32_t(fromCoords.x.get()) - int32_t(range.get()))) / Config::locationBucketSize.get();
		const DistanceInBuckets endY = DistanceInBuckets::create(std::min(((fromCoords.y + range).get() / Config::locationBucketSize.get() + 1), locationBuckets.m_maxY.get()));
		const DistanceInBuckets beginY = DistanceInBuckets::create(std::max(0, int32_t(fromCoords.y.get()) - int32_t(range.get()))) / Config::locationBucketSize.get();
		const DistanceInBuckets endZ = DistanceInBuckets::create(std::min(((fromCoords.z + range).get() / Config::locationBucketSize.get() + 1), locationBuckets.m_maxZ.get()));
		const DistanceInBuckets beginZ = DistanceInBuckets::create(std::max(0, int32_t(fromCoords.z.get()) - int32_t(range.get()))) / Config::locationBucketSize.get();
		// Iterate defined cuboid of buckets.
		for(DistanceInBuckets x = beginX; x != endX; ++x)
			for(DistanceInBuckets y = beginY; y != endY; ++y)
				for(DistanceInBuckets z = beginZ; z != endZ; ++z)
				{
					assert(x * y * z < locationBuckets.m_buckets.size());
					// Iterate actors in the defined cuboid.
					const LocationBucket& bucket = locationBuckets.get(x, y, z);
					for(LocationBucketId i = LocationBucketId::create(0); i < bucket.m_actorsMultiTile.size(); ++i)
					{
						for(auto pair : bucket.m_positionsAndCuboidsMultiTileActors[i])
						{
							const Point3D& toCoords = pair.first;
							const VisionCuboidId toVisionCuboidId = pair.second;
							// Don't bother to filter by already being present in result, it would only catch multi tile shaps which straddle a bucket boundry.
							// Refine bucket cuboid actors into sphere with radius == range.
							if(taxiDistance(fromCoords, toCoords) <= range)
							{
								// Check sightlines.
								if (
									fromVisionCuboidId == toVisionCuboidId ||
									m_area->m_opacityFacade.hasLineOfSight(fromIndex, fromCoords, toCoords)
								)
								{
									result.add(bucket.m_actorsMultiTile[i]);
									break;
								}
							}
						}
					}
					for(LocationBucketId i = LocationBucketId::create(0); i < bucket.m_actorsSingleTile.size(); ++i)
					{
						Point3D toCoords = bucket.m_positionsSingleTileActors[i];
						// Refine bucket cuboid actors into sphere with radius == range.
						if(taxiDistance(toCoords, fromCoords) <= range)
						{
							// Check sightlines.
							VisionCuboidId toVisionCuboidId = bucket.m_visionCuboidsSingleTileActors[i];
							assert(toVisionCuboidId != 0);
							if(
								fromVisionCuboidId == toVisionCuboidId ||
								m_area->m_opacityFacade.hasLineOfSight(fromIndex, fromCoords, toCoords)
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
	VisionFacadeIndex actorsSize = VisionFacadeIndex::create(m_data.size());
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
	while(m_data.size())
		removeActor(m_data.back().actor);
}
// Buckets.
VisionFacadeBuckets::VisionFacadeBuckets(Area& area) : m_area(area)
{
	for(VisionFacade& visionFacade : m_data)
		visionFacade.setArea(area);
}
VisionFacade& VisionFacadeBuckets::getForActor(const ActorIndex& actor)
{
	ActorId id = m_area.getActors().getId(actor);
	return m_data[id.get() % Config::actorDoVisionInterval];
}
void VisionFacadeBuckets::add(const ActorIndex& actor)
{
	getForActor(actor).addActor(actor);
}
void VisionFacadeBuckets::remove(const ActorIndex& actor)
{
	getForActor(actor).removeActor(actor);
}
void VisionFacadeBuckets::clear()
{
	for(VisionFacade& visionFacade : m_data)
		visionFacade.clear();
}
void VisionFacadeBuckets::updateCuboid(const ActorIndex& actor, const VisionCuboidId& visionCuboidId)
{
	auto pair = m_area.getActors().vision_getFacadeWithIndex(actor);
	pair.first->updateVisionCuboid(pair.second, visionCuboidId);
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
void HasVisionFacade::initalize(Area& area,  const ActorIndex& actor)
{
	assert(empty());
	assert(m_visionFacade == nullptr);
	// Record which vision facade bucket this actor belongs to but don't register it yet. We don't know if it can see currently.
	m_visionFacade = &area.m_visionFacadeBuckets.getForActor(actor);
}
void HasVisionFacade::create(Area& area, const ActorIndex& actor)
{
	assert(empty());
	assert(m_visionFacade != nullptr);
	assert(area.getActors().vision_canSeeAnything(actor));
	assert(area.getActors().hasLocation(actor));
	m_visionFacade->addActor(actor);
	assert(!empty());
}
void HasVisionFacade::updateRange(const DistanceInBlocks& range)
{
	assert(m_visionFacade);
	m_visionFacade->updateRange(m_index, range);
}
void HasVisionFacade::updateLocation(const BlockIndex& location)
{
	assert(m_visionFacade != nullptr);
	assert(m_index.exists());
	m_visionFacade->updateLocation(m_index, location);
}
void HasVisionFacade::clear()
{
	if(!empty())
	{
		assert(m_visionFacade != nullptr);
		m_visionFacade->remove(m_index);
		m_index.clear();
	}
}