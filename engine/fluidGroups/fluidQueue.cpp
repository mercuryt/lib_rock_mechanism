#include "fluidQueue.h"
#include "fluidGroup.h"
#include "../area/area.h"
#include "../space/space.h"
#include "numericTypes/types.h"
#include <assert.h>
void FluidQueue::setPoints(SmallSet<Point3D>& points)
{
	std::erase_if(m_queue, [&](FutureFlowPoint& futureFlowPoint){ return !points.contains(futureFlowPoint.point); });
	for(Point3D point : points)
		if(!m_set.contains(point))
			m_queue.emplace_back(point);
	m_set.swap(points);
}
void FluidQueue::maybeAddPoint(const Point3D& point)
{
	if(m_set.contains(point))
		return;
	m_set.insert(point);
	m_queue.emplace_back(point);
}
void FluidQueue::maybeAddPoints(SmallSet<Point3D>& points)
{
	//m_queue.reserve(m_queue.size() + points.size());
	for(Point3D point : points)
		if(!m_set.contains(point))
			m_queue.emplace_back(point);
	m_set.maybeInsertAll(points);
}
void FluidQueue::removePoint(const Point3D& point)
{
	m_set.erase(point);
	std::erase_if(m_queue, [&](FutureFlowPoint& futureFlowPoint){ return futureFlowPoint.point == point; });
}
void FluidQueue::maybeRemovePoint(const Point3D& point)
{
	if(m_set.contains(point))
		removePoint(point);
}
void FluidQueue::removePoints(SmallSet<Point3D>& points)
{
	m_set.eraseIf([&](const Point3D& point){ return points.contains(point); });
	std::erase_if(m_queue, [&](FutureFlowPoint& futureFlowPoint){ return points.contains(futureFlowPoint.point); });
}
void FluidQueue::merge(FluidQueue& fluidQueue)
{
	//m_queue.reserve(m_queue.size() + fluidQueue.m_set.size());
	for(Point3D point : fluidQueue.m_set)
		maybeAddPoint(point);
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
