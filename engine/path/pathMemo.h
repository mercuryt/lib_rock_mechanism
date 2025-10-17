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

struct CuboidSet;

class PathMemoClosed final
{
	RTreeData<DistanceFractional> m_data;
public:
	[[nodiscard]] bool contains(const Point3D& index) const { return m_data.queryAny(index); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] Point3D previous(const Point3D& index) const;
	[[nodiscard]] SmallSet<Point3D> getPath(const Point3D& secondToLast, const Point3D& last, const Point3D& first) const;
	[[nodiscard]] CuboidSet getIntersecting(const auto& shape) const { return m_data.queryGetAllCuboids(shape); }
	void add(const Point3D& index, const Point3D& start);
	void clear() { m_data.clear(); }
	void removeClosed(CuboidSet& cuboids) const;
};

class PathMemoBreadthFirst final
{
	// TODO: Profile with single pass vector.
	std::deque<Point3D> m_open;
	PathMemoClosed m_closed;
public:
	void reset();
	void setClosed(const Point3D& point, const Point3D& start);
	void setOpen(const Point3D& point);
	void removeClosed(CuboidSet& cuboids) { m_closed.removeClosed(cuboids); }
	[[nodiscard]] bool isClosed(const Point3D& point) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool openEmpty() const { return m_open.empty(); }
	[[nodiscard]] Point3D next();
	[[nodiscard]] SmallSet<Point3D> getPath(const Point3D& secondToLast, const Point3D& last, const Point3D& first) const;
	[[nodiscard]] CuboidSet getClosedIntersecting(const auto& shape) const { return m_closed.getIntersecting(shape); }
};
class PathMemoDepthFirst final
{
	MediumMap<Distance, Point3D> m_open;
	PathMemoClosed m_closed;
	Point3D m_huristicDestination;
public:
	void reset();
	void setClosed(const Point3D& point, const Point3D& start);
	void setOpen(const Point3D& point);
	void removeClosed(CuboidSet& cuboids) { m_closed.removeClosed(cuboids); }
	void setDestination(const Point3D& point) { m_huristicDestination = point; }
	[[nodiscard]] bool isClosed(const Point3D& point) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool openEmpty() const { return m_open.empty(); }
	[[nodiscard]] Point3D next();
	[[nodiscard]] SmallSet<Point3D> getPath(const Point3D& secondToLast, const Point3D& last, const Point3D& first) const;
	[[nodiscard]] CuboidSet getClosedIntersecting(const auto& shape) const { return m_closed.getIntersecting(shape); }
};
class SimulationHasPathMemos final
{
	std::deque<PathMemoBreadthFirst> m_breadthFirst;
	std::deque<PathMemoDepthFirst> m_depthFirst;
	std::vector<bool> m_reservedBreadthFirst;
	std::vector<bool> m_reservedDepthFirst;
	std::mutex m_mutex;
public:
	std::pair<PathMemoBreadthFirst*, uint8_t> getBreadthFirst();
	std::pair<PathMemoDepthFirst*, uint8_t> getDepthFirst();
	void releaseBreadthFirst(uint8_t index);
	void releaseDepthFirst(uint8_t index);
};
