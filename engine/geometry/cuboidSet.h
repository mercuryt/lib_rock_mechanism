#pragma once
#include "../geometry/cuboid.h"
#include "../index.h"

class Blocks;

class CuboidSet;
struct CuboidSetConstView;
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
	SmallSet<Cuboid> m_cuboids;
	//
	void create(const Cuboid& cuboid);
	void merge(const Cuboid& absorbed, const Cuboid& absorber);
public:
	void add(const Point3D& point);
	void remove(const Point3D& point);
	void add(const Blocks& blocks, const BlockIndex& block);
	void remove(const Blocks& blocks, const BlockIndex& block);
	void add(const Cuboid& cuboid);
	void remove(const Cuboid& cuboid);
	void clear() { m_cuboids.clear(); }
	[[nodiscard]] bool empty() const;
	[[nodiscard]] uint size() const;
	[[nodiscard]] bool contains(const Blocks& blocks, const BlockIndex& block) const;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] CuboidSetConstView getView(const Blocks& blocks) const { return {blocks, *this}; }
	[[nodiscard]] const SmallSet<Cuboid>& getCuboids() const { return m_cuboids; }
	[[nodiscard]] SmallSet<BlockIndex> toBlockSet(const Blocks& blocks) const;
	friend class CuboidSetConstIterator;
	friend struct CuboidSetConstView;
};
class CuboidSetWithBoundingBox : public CuboidSet
{
	Cuboid m_boundingBox;
public:
	void add(const Cuboid& cuboid);
	void remove(const Cuboid& cuboid);
	[[nodiscard]] bool contains(const Point3D& point) const { return m_boundingBox.contains(point) ? CuboidSet::contains(point) : false; }
	[[nodiscard]] const Cuboid& getBoundingBox() const { return m_boundingBox; }
};