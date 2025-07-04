#pragma once
#include "cuboid.h"
#include "../numericTypes/index.h"

class Blocks;

class CuboidSet;
struct CuboidSetConstView;
class OffsetCuboidSet;
class CuboidSetConstIterator
{
	const Blocks& m_blocks;
	const CuboidSet& m_cuboidSet;
	SmallSet<Cuboid>::const_iterator m_outerIter;
	Cuboid::iterator m_innerIter;
public:
	CuboidSetConstIterator(const Blocks& blocks, const CuboidSet& cuboidSet, bool end = false);
	CuboidSetConstIterator& operator++();
	[[nodiscard]] CuboidSetConstIterator operator++(int);
	[[nodiscard]] bool operator==(const CuboidSetConstIterator& other) const { return m_outerIter == other.m_outerIter && m_innerIter == other.m_innerIter; }
	[[nodiscard]] bool operator!=(const CuboidSetConstIterator& other) const { return !(*this == other); }
	[[nodiscard]] BlockIndex operator*() { assert(m_innerIter != m_outerIter->end(m_blocks)); return *m_innerIter; }
};
struct CuboidSetConstView
{
	const Blocks& m_blocks;
	const CuboidSet& m_cuboidSet;
	[[nodiscard]] CuboidSetConstIterator begin() { return {m_blocks, m_cuboidSet, false}; }
	[[nodiscard]] CuboidSetConstIterator end() { return {m_blocks, m_cuboidSet, true}; }
};
class CuboidSet
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
	CuboidSet(const Blocks& blocks, const BlockIndex& location, const Facing4& rotation, const OffsetCuboidSet& offsetPairs);
	CuboidSet& operator=(CuboidSet&& other) noexcept = default;
	CuboidSet& operator=(const CuboidSet& other) = default;
	void add(const Point3D& point);
	void remove(const Point3D& point);
	void add(const Blocks& blocks, const BlockIndex& block);
	void remove(const Blocks& blocks, const BlockIndex& block);
	virtual void add(const Cuboid& cuboid);
	virtual void remove(const Cuboid& cuboid);
	void clear() { m_cuboids.clear(); }
	void shift(const Offset3D offset, const DistanceInBlocks& distance);
	// For merging with other cuboid sets.
	void addSet(const CuboidSet& other);
	void rotateAroundPoint(Blocks& blocks, const BlockIndex& point, const Facing4& rotation);
	[[nodiscard]] bool empty() const;
	[[nodiscard]] uint size() const;
	[[nodiscard]] bool contains(const Blocks& blocks, const BlockIndex& block) const;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] CuboidSetConstView getView(const Blocks& blocks) const { return {blocks, *this}; }
	[[nodiscard]] const SmallSet<Cuboid>& getCuboids() const { return m_cuboids; }
	[[nodiscard]] SmallSet<BlockIndex> toBlockSet(const Blocks& blocks) const;
	[[nodiscard]] bool isAdjacent(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid boundry(const BlockIndex& block) const;
	[[nodiscard]] auto begin() { return m_cuboids.begin(); }
	[[nodiscard]] auto end() { return m_cuboids.end(); }
	[[nodiscard]] auto begin() const { return m_cuboids.begin(); }
	[[nodiscard]] auto end() const { return m_cuboids.end(); }
	static CuboidSet create(const SmallSet<Point3D>& blocks);
	friend class CuboidSetConstIterator;
	friend struct CuboidSetConstView;
};
void to_json(Json& data, const CuboidSet& cuboidSet);
void from_json(const Json& data, CuboidSet& cuboidSet);
class CuboidSetWithBoundingBoxAdjacent : public CuboidSet
{
	Cuboid m_boundingBox;
public:
	void addAndExtend(const Cuboid& cuboid);
	void addAndExtend(const Blocks& blocks, const BlockIndex& block);
	[[nodiscard]] SmallSet<Cuboid> removeAndReturnNoLongerAdjacentCuboids(const Cuboid& cuboid);
	[[nodiscard]] SmallSet<Cuboid> removeAndReturnNoLongerAdjacentCuboids(const Blocks& blocks, const BlockIndex& block);
	[[nodiscard]] bool contains(const Point3D& point) const { return m_boundingBox.contains(point) ? CuboidSet::contains(point) : false; }
	[[nodiscard]] const Cuboid& getBoundingBox() const { return m_boundingBox; }
	[[nodiscard]] bool isAdjacent(const CuboidSetWithBoundingBoxAdjacent& cuboid) const;
};
