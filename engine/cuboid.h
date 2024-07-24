#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>

#include "designations.h"
#include "types.h"
#include "blockIndices.h"
class Blocks;
class Cuboid
{
	Blocks* m_blocks;
public:
	BlockIndex m_highest = BLOCK_INDEX_MAX;
	BlockIndex m_lowest = BLOCK_INDEX_MAX;

	Cuboid(Blocks& blocks, BlockIndex h, BlockIndex l);
	Cuboid() = default;
	void merge(const Cuboid& cuboid);
	void setFrom(BlockIndex block);
	void setFrom(Blocks& blocks, BlockIndex a, BlockIndex b);
	void clear();
	[[nodiscard]] BlockIndices toSet();
	[[nodiscard]] bool contains(const BlockIndex block) const;
	[[nodiscard]] bool canMerge(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid sum(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid getFace(Facing faceing) const;
	[[nodiscard]] bool overlapsWith(const Cuboid& cuboid) const;
	[[nodiscard]] size_t size() const;
	[[nodiscard]] bool empty() const { return size() == 0; }
	[[nodiscard]] bool operator==(const Cuboid& cuboid) const;
	[[nodiscard]] static Cuboid fromBlock(Blocks& blocks, BlockIndex block);
	[[nodiscard]] static Cuboid fromBlockPair(Blocks& blocks, BlockIndex a, BlockIndex b);
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
		iterator(Blocks& blocks, BlockIndex lowest, BlockIndex highest);
		iterator& operator++();
		iterator operator++(int);
		bool operator==(const iterator& other) const { return m_current == other.m_current; }
		bool operator!=(const iterator& other) const { return !(*this == other); }
		BlockIndex operator*();
	};
	iterator begin() { return iterator(*m_blocks, m_lowest, m_highest); }
	iterator end() { return iterator(*m_blocks, BLOCK_INDEX_MAX, BLOCK_INDEX_MAX); }
	const iterator begin() const { return iterator(*m_blocks, m_lowest, m_highest); }
	const iterator end() const { return iterator(*m_blocks, BLOCK_INDEX_MAX, BLOCK_INDEX_MAX); }
	//TODO:
	//static_assert(std::forward_iterator<iterator>);
};
