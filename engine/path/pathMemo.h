/*
 * Memo data used for pathing, cleared and reused.
 * Pather is required to call reset before returning.
 */
#pragma once
#include "dataStructures/rtreeData.h"
#include "dataStructures/sortedVectorContainers.h"
#include "numericTypes/index.h"
#include <cstdint>
#include <set>

class PathMemoClosed final
{
	RTreeData<Point3D> m_data;
	SmallSet<Point3D> m_dirty;
public:
	[[nodiscard]] bool contains(const Point3D& index) const { return m_data.queryAny(index); }
	[[nodiscard]] bool empty() const { return m_dirty.empty(); }
	[[nodiscard]] Point3D previous(const Point3D& index) const { assert(contains(index)); return m_data.queryGetOne(index); }
	[[nodiscard]] SmallSet<Point3D> getPath(const Point3D& secondToLast, const Point3D& last) const;
	void add(const Point3D& index, Point3D parent);
	void clear() { for(const Point3D& index : m_dirty) { m_data.maybeRemove(index); } m_dirty.clear(); }
};

class PathMemoBreadthFirst final
{
	// TODO: Profile with single pass vector.
	std::deque<Point3D> m_open;
	PathMemoClosed m_closed;
public:
	void reset();
	void setClosed(const Point3D& point, const Point3D& previous);
	// Area is not used here, it is only provided to match the interface of depth first.
	void setOpen(const Point3D& point, const Area& area);
	[[nodiscard]] bool isClosed(const Point3D& point) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool openEmpty() const { return m_open.empty(); }
	[[nodiscard]] Point3D next();
	[[nodiscard]] SmallSet<Point3D> getPath(const Point3D& secondToLast, const Point3D& last) const;
};
class PathMemoDepthFirst final
{
	MediumMap<Distance, Point3D> m_open;
	PathMemoClosed m_closed;
	Point3D m_huristicDestination;
public:
	void reset();
	void setClosed(const Point3D& point, const Point3D& previous);
	void setOpen(const Point3D& point, const Area& area);
	void setDestination(const Point3D& point) { m_huristicDestination = point; }
	[[nodiscard]] bool isClosed(const Point3D& point) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool openEmpty() const { return m_open.empty(); }
	[[nodiscard]] Point3D next();
	[[nodiscard]] SmallSet<Point3D> getPath(const Point3D& secondToLast, const Point3D& last) const;
};
class SimulationHasPathMemos final
{
	std::deque<PathMemoBreadthFirst> m_breadthFirst;
	std::deque<PathMemoDepthFirst> m_depthFirst;
	std::vector<bool> m_reservedBreadthFirst;
	std::vector<bool> m_reservedDepthFirst;
	std::mutex m_mutex;
public:
	std::pair<PathMemoBreadthFirst*, uint8_t> getBreadthFirst(Area& area);
	std::pair<PathMemoDepthFirst*, uint8_t> getDepthFirst(Area& area);
	void releaseBreadthFirst(uint8_t index);
	void releaseDepthFirst(uint8_t index);
};
