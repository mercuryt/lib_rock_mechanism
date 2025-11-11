#include "fluidQueue.h"
#include "fluidGroup.h"
#include "../area/area.h"
#include "../space/space.h"
#include "numericTypes/types.h"
#include <assert.h>
void FluidQueue::maybeAddPoint(const Point3D& point)
{
	if(m_set.contains(point))
		return;
	m_set.add(point);
	m_queue.emplace_back(point);
}
void FluidQueue::maybeAddPoints(const CuboidSet& points)
{
	//m_queue.reserve(m_queue.size() + points.volume());
	//m_set.reserve(m_set.size() + points.size());
	auto filtered = points;
	filtered.maybeRemoveAll(m_set);
	if(filtered.empty())
		return;
	for(const Cuboid& cuboid : filtered)
		for(const Point3D point : cuboid)
			m_queue.emplace_back(point);
	m_set.addAll(filtered);
}
void FluidQueue::removePoint(const Point3D& point)
{
	m_set.remove(point);
	auto found = std::ranges::find_if(m_queue, [&](FutureFlowPoint& futureFlowPoint){ return futureFlowPoint.point == point; });
	assert(found != m_queue.end());
	m_queue.erase(found);
}
void FluidQueue::maybeRemovePoint(const Point3D& point)
{
	if(!m_set.contains(point))
		return;
	removePoint(point);
}
void FluidQueue::removePoints(const CuboidSet& points)
{
	m_set.maybeRemoveAll(points);
	std::erase_if(m_queue, [&](FutureFlowPoint& futureFlowPoint){ return points.contains(futureFlowPoint.point); });
}
void FluidQueue::merge(FluidQueue& fluidQueue)
{
	//m_queue.reserve(m_queue.size() + fluidQueue.m_set.volume());
	maybeAddPoints(fluidQueue.m_set);
}
void FluidQueue::noChange()
{
	m_groupStart = m_groupEnd = m_queue.begin();
}
uint32_t FluidQueue::groupSize() const
{
	return m_groupEnd - m_groupStart;
}
CollisionVolume FluidQueue::groupCapacityPerPoint() const
{
	assert(m_groupStart != m_groupEnd);
	return m_groupStart->capacity;
}
CollisionVolume FluidQueue::groupFlowTillNextStepPerPoint() const
{
	assert(m_groupStart != m_groupEnd);
	if(m_groupEnd == m_queue.end() || m_groupEnd->point.z() != m_groupStart->point.z())
		return CollisionVolume::null();
	assert(m_groupEnd->capacity < m_groupStart->capacity);
	return m_groupStart->capacity - m_groupEnd->capacity;
}
bool FluidQueue::groupContains(const Point3D& point) const
{
	return std::ranges::find(m_groupStart, m_groupEnd, point, &FutureFlowPoint::point) != m_groupEnd;
}
