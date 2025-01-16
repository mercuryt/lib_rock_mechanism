#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>

#include "designations.h"
#include "types.h"
#include "index.h"
class Blocks;
class CuboidView;
class Cuboid
{
public:
	BlockIndex m_highest;
	BlockIndex m_lowest;

	Cuboid(Blocks& blocks, const BlockIndex& h, const BlockIndex& l);
	Cuboid() = default;
	void merge(Blocks& blocks, const Cuboid& cuboid);
	void setFrom(const BlockIndex& block);
	void setFrom(Blocks& blocks, const BlockIndex& a, const BlockIndex& b);
	void clear();
	[[nodiscard]] SmallSet<BlockIndex> toSet(Blocks& blocks);
	[[nodiscard]] bool contains(Blocks& blocks, const BlockIndex& block) const;
	[[nodiscard]] bool canMerge(Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid sum(Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid getFace(Blocks& blocks, const Facing& faceing) const;
	[[nodiscard]] bool overlapsWith(Blocks& blocks, const Cuboid& cuboid) const;
	[[nodiscard]] size_t size(Blocks& blocks) const;
	[[nodiscard]] bool empty(Blocks& blocks) const { return size(blocks) == 0; }
	[[nodiscard]] bool operator==(const Cuboid& cuboid) const;
	[[nodiscard]] Point3D getCenter(const Blocks& blocks) const;
	[[nodiscard]] static Cuboid fromBlock(Blocks& blocks, const BlockIndex& block);
	[[nodiscard]] static Cuboid fromBlockPair(Blocks& blocks, const BlockIndex& a, const BlockIndex& b);
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