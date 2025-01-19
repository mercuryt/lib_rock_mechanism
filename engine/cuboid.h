#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>

#include "designations.h"
#include "types.h"
#include "index.h"
class Blocks;
class CuboidView;
class CuboidSurfaceView;
class Cuboid
{
public:
	BlockIndex m_highest;
	BlockIndex m_lowest;

	Cuboid(const Blocks& blocks, const BlockIndex& h, const BlockIndex& l);
	Cuboid() = default;
	void merge(const Blocks& blocks, const Cuboid& cuboid);
	void setFrom(const BlockIndex& block);
	void setFrom(const Blocks& blocks, const BlockIndex& a, const BlockIndex& b);
	void clear();
	void shift(const Blocks& blocks, const Facing& direction, const DistanceInBlocks& distance);
	void setMaxZ(const Blocks& blocks, const DistanceInBlocks& distance);
	[[nodiscard]] SmallSet<BlockIndex> toSet(Blocks& blocks);
	[[nodiscard]] bool contains(const Blocks& blocks, const BlockIndex& block) const;
	[[nodiscard]] bool contains(const Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] bool canMerge(const Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid canMergeSteal(const Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid sum(const Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid getFace(const Blocks& blocks, const Facing& faceing) const;
	[[nodiscard]] bool overlapsWith(const Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] size_t size(const Blocks& blocks) const;
	[[nodiscard]] bool empty(const Blocks& blocks) const { return size(blocks) == 0; }
	[[nodiscard]] bool operator==(const Cuboid& cuboid) const;
	[[nodiscard]] Point3D getCenter(const Blocks& blocks) const;
	[[nodiscard]] DistanceInBlocks dimensionForFacing(const Blocks& blocks, const Facing& facing) const;
	[[nodiscard]] Facing getFacingTwordsOtherCuboid(const Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] static Cuboid fromBlock(Blocks& blocks, const BlockIndex& block);
	[[nodiscard]] static Cuboid fromBlockPair(const Blocks& blocks, const BlockIndex& a, const BlockIndex& b);
	class iterator
	{
	private:
		Point3D m_start;
		Point3D m_end;
		Point3D m_current;
		Blocks& m_blocks;
		void setToEnd();
	public:
		iterator(Blocks& blocks, Point3D low, Point3D high) : m_start(low), m_end(high), m_current(low), m_blocks(blocks)  { }
		iterator(Blocks& blocks, const BlockIndex& lowest, const BlockIndex& highest);
		iterator& operator++();
		iterator operator++(int);
		bool operator==(const iterator& other) const { return m_current == other.m_current; }
		bool operator!=(const iterator& other) const { return !(*this == other); }
		BlockIndex operator*();
	};
	iterator begin(Blocks& blocks) { return iterator(blocks, m_lowest, m_highest); }
	iterator end(Blocks& blocks) { return iterator(blocks, BlockIndex::null(), BlockIndex::null()); }
	const iterator begin(Blocks& blocks) const { return iterator(blocks, m_lowest, m_highest); }
	const iterator end(Blocks& blocks) const { return iterator(blocks, BlockIndex::null(), BlockIndex::null()); }
	//TODO:
	//static_assert(std::forward_iterator<iterator>);
	CuboidView getView(Blocks& blocks) const;
	CuboidSurfaceView getSurfaceView(Blocks& blocks) const;
	std::wstring toString(const Blocks& blocks) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Cuboid, m_highest, m_lowest);
};
struct CuboidView : public std::ranges::view_interface<CuboidView>
{
	const Cuboid cuboid;
	Blocks& blocks;
	CuboidView() = default;
	CuboidView(Blocks& b, const Cuboid c) : cuboid(c), blocks(b) { }
	Cuboid::iterator begin() const { return cuboid.begin(blocks); }
	Cuboid::iterator end() const { return cuboid.end(blocks); }
};
struct CuboidSurfaceView : public std::ranges::view_interface<CuboidSurfaceView>
{
	const Cuboid cuboid;
	Blocks& blocks;
	CuboidSurfaceView() = default;
	CuboidSurfaceView(Blocks& b, const Cuboid c) : cuboid(c), blocks(b) { }
	struct Iterator
	{
		const CuboidSurfaceView& view;
		Cuboid face;
		Point3D current;
		Facing facing = Facing::create(0);
		void setFace();
		void setToEnd();
		Iterator(const CuboidSurfaceView& v) : view(v) { setFace(); }
		Iterator& operator++();
		Iterator operator++(int);
		bool operator==(const Iterator& other) const { return current == other.current; }
		bool operator!=(const Iterator& other) const { return !(*this == other); }
		std::pair<BlockIndex, Facing> operator*();
	};
	Iterator begin() const { return {*this}; }
	Iterator end() const { Iterator output(*this); output.setToEnd(); return output; }
	//static_assert(std::forward_iterator<Iterator>);
};