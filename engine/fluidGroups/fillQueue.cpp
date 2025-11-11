#include "fillQueue.h"
#include "../space/space.h"
#include "fluidGroup.h"
#include "../area/area.h"
#include "../space/space.h"
#include "numericTypes/types.h"
void FillQueue::initalizeForStep(Area& area, FluidGroup& fluidGroup)
{
	auto& space = area.getSpace();
	for(FutureFlowPoint& futureFlowPoint : m_queue)
	{
		futureFlowPoint.delta = CollisionVolume::create(0);
		futureFlowPoint.capacity = space.fluid_volumeOfTypeCanEnter(futureFlowPoint.point, fluidGroup.m_fluidType);
	}
	std::ranges::sort(m_queue.begin(), m_queue.end(), [&](FutureFlowPoint& a, FutureFlowPoint& b){
		return getPriority(a) < getPriority(b);
	});
	m_groupStart = m_queue.begin();
	findGroupEnd();
	m_futureFull.clear();
	m_futureNoLongerEmpty.clear();
	m_overfull.clear();
	validate();
}
void FillQueue::recordDelta(Area& area, FluidGroup& fluidGroup, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep)
{
	assert((m_groupStart->capacity >= volume));
	assert(volume != 0);
	assert((m_groupStart != m_groupEnd));
	assert(flowTillNextStep.exists());
	validate();
	auto& space = area.getSpace();
	// Record fluid level changes.
	for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0 && !space.fluid_contains(iter->point, fluidGroup.m_fluidType))
			m_futureNoLongerEmpty.maybeAdd(iter->point);
		iter->delta += volume;
		assert(iter->delta <= Config::maxPointVolume);
		assert(iter->capacity >= volume);
		iter->capacity -= volume;
	}
	// Record full space and get next group.
	if(flowCapacity == volume)
	{
		assert((space.fluid_volumeOfTypeContains(m_groupStart->point, fluidGroup.m_fluidType) + m_groupStart->delta) <= Config::maxPointVolume);
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			if((space.fluid_volumeOfTypeContains(iter->point, fluidGroup.m_fluidType) + iter->delta) == Config::maxPointVolume)
				m_futureFull.add(iter->point);
		m_groupStart = m_groupEnd;
		findGroupEnd();
	}
	// Expand group for new higher level.
	// TODO: continue from current position rather then reseting to start + 1.
	else if(flowTillNextStep == volume)
		findGroupEnd();
	validate();
}
void FillQueue::applyDelta(Area& area, FluidGroup& fluidGroup)
{
	validate();
	Space& space = area.getSpace();
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert(
				!space.fluid_contains(iter->point, fluidGroup.m_fluidType) ||
				space.fluid_getGroup(iter->point, fluidGroup.m_fluidType) != nullptr);
		space.fluid_fillInternal(iter->point, iter->delta, fluidGroup);
		/*assert(iter->point->m_hasFluids.m_fluids[m_fluidGroup.m_fluidType].second != &m_fluidGroup ||
			(iter->point->m_hasFluids.m_fluids[m_fluidGroup.m_fluidType].first < Config::maxPointVolume && !m_futureFull.contains(iter->point)) ||
			(iter->point->m_hasFluids.m_fluids[m_fluidGroup.m_fluidType].first == Config::maxPointVolume && m_futureFull.contains(iter->point)));
		*/
		if(space.fluid_getTotalVolume(iter->point) > Config::maxPointVolume)
			m_overfull.add(iter->point);
	}
	validate();
}
CollisionVolume FillQueue::groupLevel(Area& area, FluidGroup& fluidGroup) const
{
	assert((m_groupStart != m_groupEnd));
	//TODO: calculate this durring find end.
	CollisionVolume highestLevel = CollisionVolume::create(0);
	for(auto it = m_groupStart; it != m_groupEnd; ++it)
	{
		CollisionVolume level = it->delta + area.getSpace().fluid_volumeOfTypeContains(it->point, fluidGroup.m_fluidType);
		if(level > highestLevel)
			highestLevel = level;

	}
	return highestLevel;
}
uint32_t FillQueue::getPriority(FutureFlowPoint& futureFlowPoint) const
{
	if(futureFlowPoint.capacity == 0)
		return UINT32_MAX;
	//TODO: What is happening here?
	return (( futureFlowPoint.point.z().get() + 1) * Config::maxPointVolume.get() * 2) - futureFlowPoint.capacity.get();
}
void FillQueue::findGroupEnd()
{
	if(m_groupStart == m_queue.end() || m_groupStart->capacity == 0)
	{
		m_groupEnd = m_groupStart;
		return;
	}
	uint32_t priority = getPriority(*m_groupStart);
	for(m_groupEnd = m_groupStart + 1; m_groupEnd != m_queue.end(); ++m_groupEnd)
	{
		uint32_t otherPriority = getPriority(*m_groupEnd);
		assert(priority <= otherPriority);
		if(otherPriority != priority)
			break;
	}
	validate();
}
void FillQueue::validate() const
{
	assert((m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end()));
	assert((m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end()));
	assert((m_groupEnd == m_queue.end() || m_groupEnd == m_groupStart || m_groupStart->point.z() != m_groupEnd->point.z() || m_groupStart->capacity > m_groupEnd->capacity));
}