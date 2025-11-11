#include "drainQueue.h"
#include "fluidGroup.h"
#include "../area/area.h"
#include "../config.h"
#include "../space/space.h"
#include "numericTypes/types.h"
#include <algorithm>
void DrainQueue::initalizeForStep(Area& area, FluidGroup& fluidGroup)
{
	Space& space = area.getSpace();
	for(FutureFlowPoint& futureFlowPoint : m_queue)
	{
		assert((space.fluid_contains(futureFlowPoint.point, fluidGroup.m_fluidType)));
		assert(space.fluid_getTotalVolume(futureFlowPoint.point) <= Config::maxPointVolume);
		futureFlowPoint.delta = CollisionVolume::create(0);
		futureFlowPoint.capacity = space.fluid_volumeOfTypeContains(futureFlowPoint.point, fluidGroup.m_fluidType);
	}
	std::ranges::sort(m_queue.begin(), m_queue.end(), [&](FutureFlowPoint& a, FutureFlowPoint& b){
		return getPriority(a) > getPriority(b);
	});
	m_groupStart = m_queue.begin();
	findGroupEnd();
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
	if(space.fluid_getTotalVolume(m_groupStart->point) == Config::maxPointVolume && !m_futureNoLongerFull.contains((m_groupEnd-1)->point))
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			//TODO: is this maybe correct? Why would they already be added?
			m_futureNoLongerFull.maybeAdd(iter->point);
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
			m_futureEmpty.add(iter->point);
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
	SmallSet<Point3D> drainedFromAndAdjacent;
	Space& space = area.getSpace();
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert(space.fluid_contains(iter->point, fluidGroup.m_fluidType));
		assert(space.fluid_volumeOfTypeContains(iter->point, fluidGroup.m_fluidType) >= iter->delta);
		assert(iter->point.exists());
		assert(iter->delta.exists());
		assert(space.fluid_getTotalVolume(iter->point) >= iter->delta);
		space.fluid_drainInternal(iter->point, iter->delta, fluidGroup.m_fluidType);
		// Record space to set fluid groups unstable.
		drainedFromAndAdjacent.maybeInsert(iter->point);
		for(const Point3D& adjacent : space.getDirectlyAdjacent(iter->point))
			if(space.fluid_canEnterEver(adjacent))
				drainedFromAndAdjacent.maybeInsert(adjacent);
		fluidGroup.validate(area);
	}
	// Set fluidGroups unstable.
	// TODO: Would it be better to prevent fluid groups from becoming stable while in contact with another group? Either option seems bad.
	for(Point3D point : drainedFromAndAdjacent)
		space.fluid_setAllUnstableExcept(point, fluidGroup.m_fluidType);
}
CollisionVolume DrainQueue::groupLevel(Area& area, FluidGroup& fluidGroup) const
{
	assert((m_groupStart != m_groupEnd));
	Space& space = area.getSpace();
	return space.fluid_volumeOfTypeContains(m_groupStart->point, fluidGroup.m_fluidType) - m_groupStart->delta;
}
uint32_t DrainQueue::getPriority(FutureFlowPoint& futureFlowPoint) const
{
	//TODO: What is happening here?
	return futureFlowPoint.point.z().get() * Config::maxPointVolume.get() * 2 + futureFlowPoint.capacity.get();
}
void DrainQueue::findGroupEnd()
{
	if(m_groupStart == m_queue.end())
	{
		m_groupEnd = m_groupStart;
		return;
	}
	uint32_t priority = getPriority(*m_groupStart);
	for(m_groupEnd = m_groupStart + 1; m_groupEnd != m_queue.end(); ++m_groupEnd)
		if(getPriority(*m_groupEnd) != priority)
			break;
}