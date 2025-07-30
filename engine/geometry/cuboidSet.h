#pragma once
#include "cuboid.h"
#include "../numericTypes/index.h"

struct CuboidSet;
struct CuboidSetConstView;
struct OffsetCuboidSet;
struct CuboidSetConstIterator
{
	const CuboidSet& m_cuboidSet;
	SmallSet<Cuboid>::const_iterator m_outerIter;
	Cuboid::iterator m_innerIter;
public:
	CuboidSetConstIterator(const CuboidSet& cuboidSet, bool end = false);
	CuboidSetConstIterator& operator++();
	[[nodiscard]] CuboidSetConstIterator operator++(int);
	[[nodiscard]] bool operator==(const CuboidSetConstIterator& other) const { return m_outerIter == other.m_outerIter && m_innerIter == other.m_innerIter; }
	[[nodiscard]] bool operator!=(const CuboidSetConstIterator& other) const { return !(*this == other); }
	[[nodiscard]] Point3D operator*() { assert(m_innerIter != m_outerIter->end()); return *m_innerIter; }
};
struct CuboidSetConstView
{
	const CuboidSet& m_cuboidSet;
	[[nodiscard]] CuboidSetConstIterator begin() { return {m_cuboidSet, false}; }
	[[nodiscard]] CuboidSetConstIterator end() { return {m_cuboidSet, true}; }
};
struct CuboidSet
{
protected:
	SmallSet<Cuboid> m_cuboids;
	// Overriden by AreaHasVisionCuboids.
	virtual void create(const Cuboid& cuboid);
	virtual void destroy(const uint& cuboid);
	// For merging contained cuboids.
	void mergeInternal(const Cuboid& absorbed, const uint& absorber);
public:
	CuboidSet() = default;
	CuboidSet(const CuboidSet& other) = default;
	CuboidSet(CuboidSet&& other) noexcept = default;
	CuboidSet(const Point3D& location, const Facing4& rotation, const OffsetCuboidSet& offsetPairs);
	CuboidSet(const std::initializer_list<Cuboid>& cuboids);
	CuboidSet& operator=(CuboidSet&& other) noexcept = default;
	CuboidSet& operator=(const CuboidSet& other) = default;
	void add(const Point3D& point);
	void remove(const Point3D& point);
	void removeAll(const auto& cuboids) { for(const Cuboid& cuboid : cuboids) remove(cuboid); }
	virtual void add(const Cuboid& cuboid);
	virtual void remove(const Cuboid& cuboid);
	void clear() { m_cuboids.clear(); }
	void shift(const Offset3D offset, const Distance& distance);
	// For merging with other cuboid sets.
	void addSet(const CuboidSet& other);
	void rotateAroundPoint(const Point3D& point, const Facing4& rotation);
	[[nodiscard]] bool empty() const;
	[[nodiscard]] uint size() const;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] bool contains(const Cuboid& cuboid) const;
	[[nodiscard]] CuboidSetConstView getView() const { return {*this}; }
	[[nodiscard]] const SmallSet<Cuboid>& getCuboids() const { return m_cuboids; }
	[[nodiscard]] SmallSet<Point3D> toPointSet() const;
	[[nodiscard]] bool isAdjacent(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid boundry() const;
	[[nodiscard]] Point3D getLowest() const;
	[[nodiscard]] const Cuboid& front() const { return m_cuboids.front(); }
	[[nodiscard]] const Cuboid& back() const { return m_cuboids.back(); }
	[[nodiscard]] auto begin() { return m_cuboids.begin(); }
	[[nodiscard]] auto end() { return m_cuboids.end(); }
	[[nodiscard]] auto begin() const { return m_cuboids.begin(); }
	[[nodiscard]] auto end() const { return m_cuboids.end(); }
	static CuboidSet create(const SmallSet<Point3D>& space);
	friend struct CuboidSetConstIterator;
	friend struct CuboidSetConstView;
};
void to_json(Json& data, const CuboidSet& cuboidSet);
void from_json(const Json& data, CuboidSet& cuboidSet);
struct CuboidSetWithBoundingBoxAdjacent : public CuboidSet
{
	Cuboid m_boundingBox;
public:
	void addAndExtend(const Cuboid& cuboid);
	void addAndExtend(const Point3D& point);
	[[nodiscard]] SmallSet<Cuboid> removeAndReturnNoLongerAdjacentCuboids(const Cuboid& cuboid);
	[[nodiscard]] SmallSet<Cuboid> removeAndReturnNoLongerAdjacentCuboids(const Point3D& point);
	[[nodiscard]] bool contains(const Point3D& point) const { return m_boundingBox.contains(point) ? CuboidSet::contains(point) : false; }
	[[nodiscard]] const Cuboid& getBoundingBox() const { return m_boundingBox; }
	[[nodiscard]] bool isAdjacent(const CuboidSetWithBoundingBoxAdjacent& cuboid) const;
};
