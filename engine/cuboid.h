#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>

#include "designations.h"
#include "types.h"
#include "index.h"
class Blocks;
class Cuboid
{
	// TODO: remove this!
	Blocks* m_blocks;
public:
	BlockIndex m_highest;
	BlockIndex m_lowest;

	Cuboid(Blocks& blocks, const BlockIndex& h, const BlockIndex& l);
	Cuboid() = default;
	void merge(const Cuboid& cuboid);
	void setFrom(const BlockIndex& block);
	void setFrom(Blocks& blocks, const BlockIndex& a, const BlockIndex& b);
	void clear();
	[[nodiscard]] BlockIndices toSet();
	[[nodiscard]] bool contains(const BlockIndex& block) const;
	[[nodiscard]] bool canMerge(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid sum(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid getFace(const Facing& faceing) const;
	[[nodiscard]] bool overlapsWith(const Cuboid& cuboid) const;
	[[nodiscard]] size_t size() const;
	[[nodiscard]] bool empty() const { return size() == 0; }
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
	iterator begin() { return iterator(*m_blocks, m_lowest, m_highest); }
	iterator end() { return iterator(*m_blocks, BlockIndex::null(), BlockIndex::null()); }
	const iterator begin() const { return iterator(*m_blocks, m_lowest, m_highest); }
	const iterator end() const { return iterator(*m_blocks, BlockIndex::null(), BlockIndex::null()); }
	//TODO:
	//static_assert(std::forward_iterator<iterator>);
};
