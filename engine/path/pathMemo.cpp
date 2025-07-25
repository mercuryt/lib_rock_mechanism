#include "pathMemo.h"
#include "area/area.h"
#include "space/space.h"
#include "numericTypes/index.h"
void PathMemoClosed::add(const Point3D& index, Point3D parent)
{
	assert(!contains(index));
	assert(index.exists());
	// Max is used to indicate the start of the path.
	if(parent.z() != Distance::max())
		assert(contains(parent));
	m_data.maybeInsertOrOverwrite(index, parent);
	m_dirty.insert(index);
}
SmallSet<Point3D> PathMemoClosed::getPath(const Point3D& secondToLast, const Point3D& last) const
{
	SmallSet<Point3D> output;
	// Last may not exist if we are using the one point shape pathing adjacent to condition optimization.
	if(last.exists())
		output.insert(last);
	Point3D current = secondToLast;
	// Max is used to indicade the start of the path.
	while(previous(current).z() != Distance::max())
	{
		output.insert(current);
		current = previous(current);
	}
	return output;
}
// Breadth First.
void PathMemoBreadthFirst::reset()
{
	m_closed.clear();
	m_open.clear();
}
void PathMemoBreadthFirst::setClosed(const Point3D& point, const Point3D& previous)
{
	assert(!m_closed.contains(point));
	m_closed.add(point, previous);
}
void PathMemoBreadthFirst::setOpen(const Point3D& point, const Area&)
{
	assert(!m_closed.contains(point));
	m_open.push_back(point);
}
bool PathMemoBreadthFirst::isClosed(const Point3D& point) const
{
	return m_closed.contains(point);
}
bool PathMemoBreadthFirst::empty() const
{
	return m_closed.empty() && m_open.empty();
}
Point3D PathMemoBreadthFirst::next()
{
	Point3D output = m_open.front();
	m_open.pop_front();
	return output;
}
SmallSet<Point3D> PathMemoBreadthFirst::getPath(const Point3D& secondToLast, const Point3D& last) const
{
	return m_closed.getPath(secondToLast, last);
}
// Depth First.
void PathMemoDepthFirst::reset()
{
	m_closed.clear();
	m_open.clear();
	m_huristicDestination.clear();
}
void PathMemoDepthFirst::setClosed(const Point3D& point, const Point3D& previous)
{
	assert(!m_closed.contains(point));
	m_closed.add(point, previous);
}
void PathMemoDepthFirst::setOpen(const Point3D& point, const Area& area)
{
	assert(m_huristicDestination.exists());
	[[maybe_unused]] bool contains = m_closed.contains(point);
	assert(!contains);
	// Subtract from max rather then provide MediumMap with the ability to sort backwards.
	Distance distance = Distance::max() - point.distanceToSquared(m_huristicDestination);
	m_open.insertNonUnique(distance, point);
}
bool PathMemoDepthFirst::isClosed(const Point3D& point) const
{
	return m_closed.contains(point);
}
bool PathMemoDepthFirst::empty() const
{
	return m_closed.empty() && m_open.empty();
}
Point3D PathMemoDepthFirst::next()
{
	assert(!m_open.empty());
	Point3D output = m_open.back().second;
	m_open.pop_back();
	return output;
}
SmallSet<Point3D> PathMemoDepthFirst::getPath(const Point3D& secondToLast, const Point3D& last) const
{
	return m_closed.getPath(secondToLast, last);
}
// SimulaitonHasPathMemos.
std::pair<PathMemoBreadthFirst*, uint8_t> SimulationHasPathMemos::getBreadthFirst(Area& area)
{
	std::lock_guard lock(m_mutex);
	// Find an unreserved memo to use.
	auto found = std::ranges::find(m_reservedBreadthFirst, false);
	if(found == m_reservedBreadthFirst.end())
	{
		uint oldSize = m_breadthFirst.size();
		m_breadthFirst.emplace_back(area);
		m_reservedBreadthFirst.resize(m_breadthFirst.size());
		found = m_reservedBreadthFirst.begin() + oldSize;
	}
	uint offset = std::distance(m_reservedBreadthFirst.begin(), found);
	m_reservedBreadthFirst[offset] = true;
	return {&m_breadthFirst[offset], offset};
}
std::pair<PathMemoDepthFirst*, uint8_t> SimulationHasPathMemos::getDepthFirst(Area& area)
{
	std::lock_guard lock(m_mutex);
	// Find an unreserved memo to use.
	auto found = std::ranges::find(m_reservedDepthFirst, false);
	if(found == m_reservedDepthFirst.end())
	{
		uint oldSize = m_depthFirst.size();
		m_depthFirst.emplace_back(area);
		m_reservedDepthFirst.resize(m_depthFirst.size());
		found = m_reservedDepthFirst.begin() + oldSize;
	}
	uint offset = std::distance(m_reservedDepthFirst.begin(), found);
	m_reservedDepthFirst[offset] = true;
	return {&m_depthFirst[offset], offset};
}
void SimulationHasPathMemos::releaseBreadthFirst(uint8_t index)
{
	std::lock_guard lock(m_mutex);
	m_reservedBreadthFirst[index] = false;
	assert(m_breadthFirst[index].empty());
}
void SimulationHasPathMemos::releaseDepthFirst(uint8_t index)
{
	std::lock_guard lock(m_mutex);
	m_reservedDepthFirst[index] = false;
	assert(m_depthFirst[index].empty());
}
