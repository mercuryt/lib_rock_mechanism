#include "fluidQueue.h"
#include "fluidGroup.h"
#include "../area/area.h"
#include "../space/space.h"
#include "numericTypes/types.h"
#include <assert.h>
template<typename Derived>
void FluidQueue<Derived>::noChange()
{
	m_groupStart = m_groupEnd = m_queue.begin();
}
template<typename Derived>
void FluidQueue<Derived>::findGroupEnd()
{
	m_groupEnd = m_groupStart;
	// Fill Queue may have future flow cuboids with 0 capacity due to them being fully occupied by heavyer fluids.
	// These get sorted to the back and act the same as the end of the queue, where m_groupEnd == m_groupStart.
	if(m_groupStart->capacity == 0)
		return;
	const auto& end = m_queue.end();
	const auto& startPriority = Derived::getPriority(*m_groupStart);
	while(m_groupEnd != end && Derived::getPriority(*m_groupEnd) == startPriority)
		++m_groupEnd;
}
template<typename Derived>
int FluidQueue<Derived>::groupVolume() const
{
	int output = 0;
	for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
		output += iter->cuboid.volume();
	return output;
}
template<typename Derived>
CollisionVolume FluidQueue<Derived>::groupCapacityPerPoint() const
{
	assert(m_groupStart != m_groupEnd);
	return m_groupStart->capacity;
}
template<typename Derived>
CollisionVolume FluidQueue<Derived>::groupFlowTillNextStepPerPoint() const
{
	assert(m_groupStart != m_groupEnd);
	if(m_groupEnd == m_queue.end() || m_groupEnd->cuboid.m_high.z() != m_groupStart->cuboid.m_high.z())
		return CollisionVolume::null();
	assert(m_groupEnd->capacity < m_groupStart->capacity);
	return m_groupStart->capacity - m_groupEnd->capacity;
}
template class FluidQueue<FillQueue>;
template class FluidQueue<DrainQueue>;