#include "drainQueue.h"
#include "fluidGroup.h"
#include "../area/area.h"
#include "../config/config.h"
#include "../space/space.h"
#include "numericTypes/types.h"
#include <algorithm>
void DrainQueue::initializeForStep(Area& area, FluidGroup& fluidGroup)
{
	if(!m_set.empty())
	{
		m_queue.clear();
		const Space& space = area.getSpace();
		auto action = [&](const Cuboid& cuboid, const FluidData& data)
		{
			if(data.group == &fluidGroup)
				for(const Cuboid& flatCuboid : cuboid.sliceAtEachZ())
					m_queue.emplace_back(flatCuboid, data.volume, CollisionVolume::create(0));
		};
		space.fluid_forEachWithCuboid(m_set, action);
		[[maybe_unused]] int totalQueueVolume = 0;
		for(const FutureFlowCuboid& ffc : m_queue)
			totalQueueVolume += ffc.cuboid.volume();
		assert(totalQueueVolume == m_set.volume());
		std::sort(m_queue.begin(), m_queue.end(), compare);
		m_groupStart = m_queue.begin();
		findGroupEnd();
	}
	m_futureEmpty.clear();
	m_futureNoLongerFull.clear();
}
void DrainQueue::recordDelta(Area& area, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep)
{
	assert(volume != 0);
	assert((m_groupStart != m_groupEnd));
	assert((m_groupStart->capacity >= volume));
	assert((m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end()));
	assert((m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end()));
	Space& space = area.getSpace();
	// Record no longer full.
	if(space.fluid_getTotalVolume(m_groupStart->cuboid.m_high) == Config::maxPointVolume && !m_futureNoLongerFull.contains((m_groupEnd-1)->cuboid))
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			//TODO: is this maybe correct? Why would they already be added?
			m_futureNoLongerFull.maybeAdd(iter->cuboid);
	// Record fluid level changes.
	for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
	{
		iter->delta += volume;
		assert(iter->delta <= Config::maxPointVolume);
		iter->capacity -= volume;
	}
	// Record empty space and get next group.
	if(volume == flowCapacity)
	{
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			m_futureEmpty.add(iter->cuboid);
		m_groupStart = m_groupEnd;
		findGroupEnd();
	}
	// Expand group for new higher level.
	else if(volume == flowTillNextStep)
		findGroupEnd();
}
void DrainQueue::applyDelta(Area& area, FluidGroup& fluidGroup)
{
	assert((m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end()));
	assert((m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end()));
	CuboidSet drainedFromAndAdjacent;
	Space& space = area.getSpace();
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert(space.fluid_contains(iter->cuboid, fluidGroup.m_fluidType));
		assert(space.fluid_volumeOfTypeContains(iter->cuboid.m_high, fluidGroup.m_fluidType) >= iter->delta);
		assert(iter->cuboid.exists());
		assert(iter->delta.exists());
		assert(space.fluid_getTotalVolume(iter->cuboid.m_high) >= iter->delta);
		space.fluid_drainInternal(iter->cuboid, iter->delta, fluidGroup.m_fluidType);
		// Record space to set fluid groups unstable.
		drainedFromAndAdjacent.maybeAdd(iter->cuboid);
		CuboidSet adjacent;
		adjacent.add(iter->cuboid);
		adjacent.inflate({1});
		space.fluid_removePointsWhichCannotBeEnteredEverFromCuboidSet(adjacent);
		drainedFromAndAdjacent.maybeAddAll(adjacent);
	}
	// Set fluidGroups unstable.
	// TODO: Would it be better to prevent fluid groups from becoming stable while in contact with another group? Either option seems bad.
	space.fluid_setAllUnstableExcept(drainedFromAndAdjacent, fluidGroup.m_fluidType);
}
CollisionVolume DrainQueue::groupLevel(Area& area, FluidGroup& fluidGroup) const
{
	assert((m_groupStart != m_groupEnd));
	Space& space = area.getSpace();
	return space.fluid_volumeOfTypeContains(m_groupStart->cuboid.m_high, fluidGroup.m_fluidType) - m_groupStart->delta;
}
int DrainQueue::getPriority(const FutureFlowCuboid& futureFlowPoint)
{
	//TODO: What is happening here?
	return (futureFlowPoint.cuboid.m_high.z().get() * Config::maxPointVolume.get() * 2) + futureFlowPoint.capacity.get();
}
bool DrainQueue::compare(const FutureFlowCuboid& a, const FutureFlowCuboid& b)
{
	return getPriority(a) > getPriority(b);
}