#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>

#include "../types.h"
#include "../index.h"
#include "point3D.h"
class Blocks;
class CuboidView;
class CuboidSurfaceView;
class Sphere;
class Cuboid
{
public:
	Point3D m_highest;
	Point3D m_lowest;

	Cuboid(const Blocks& blocks, const BlockIndex& h, const BlockIndex& l);
	Cuboid(const Point3D& highest, const Point3D& lowest);

	Cuboid() = default;
	void merge(const Cuboid& cuboid);
	void setFrom(const Point3D& block);
	void setFrom(const Blocks& blocks, const BlockIndex& block);
	void setFrom(const Blocks& blocks, const BlockIndex& a, const BlockIndex& b);
	void clear();
	void shift(const Facing6& direction, const DistanceInBlocks& distance);
	void setMaxZ(const DistanceInBlocks& distance);
	void maybeExpand(const Cuboid& other);
	[[nodiscard]] SmallSet<BlockIndex> toSet(const Blocks& blocks) const;
	[[nodiscard]] bool contains(const Blocks& blocks, const BlockIndex& block) const;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] bool contains(const Cuboid& cuboid) const;
	[[nodiscard]] bool canMerge(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid canMergeSteal(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid sum(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid getFace(const Facing6& faceing) const;
	[[nodiscard]] bool overlapsWith(const Cuboid& cuboid) const;
	[[nodiscard]] bool overlapsWithSphere(const Sphere& sphere) const;
	[[nodiscard]] size_t size() const;
	[[nodiscard]] bool empty() const { return m_highest.empty(); }
	[[nodiscard]] bool operator==(const Cuboid& cuboid) const;
	[[nodiscard]] Point3D getCenter() const;
	[[nodiscard]] DistanceInBlocks dimensionForFacing(const Facing6& facing) const;
	[[nodiscard]] Facing6 getFacingTwordsOtherCuboid(const Cuboid& cuboid) const;
	[[nodiscard]] bool isSomeWhatInFrontOf(const Point3D& position, const Facing4& facing) const;
	[[nodiscard]] SmallSet<Cuboid> getChildrenWhenSplitByCuboid(const Cuboid& cuboid) const;
	[[nodiscard]] bool isTouching(const Cuboid& cuboid) const;
	[[nodiscard]] static Cuboid fromBlock(const Blocks& blocks, const BlockIndex& block);
	[[nodiscard]] static Cuboid fromBlockPair(const Blocks& blocks, const BlockIndex& a, const BlockIndex& b);
	class iterator
	{
	private:
		Point3D m_start;
		Point3D m_end;
		Point3D m_current;
		const Blocks* m_blocks;
		void setToEnd();
	public:
		iterator(const Blocks& blocks, Point3D low, Point3D high) : m_start(low), m_end(high), m_current(low), m_blocks(&blocks)  { }
		iterator(const Blocks& blocks, const BlockIndex& lowest, const BlockIndex& highest);
		iterator(const iterator& other) = default;
		iterator& operator=(const iterator& other);
		iterator& operator++();
		[[nodiscard]] iterator operator++(int);
		[[nodiscard]] bool operator==(const iterator& other) const { return m_current == other.m_current; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return !(*this == other); }
		[[nodiscard]] BlockIndex operator*();
	};
	iterator begin(const Blocks& blocks) { return iterator(blocks, m_lowest, m_highest); }
	iterator end(const Blocks& blocks) { return iterator(blocks, BlockIndex::null(), BlockIndex::null()); }
	const iterator begin(const Blocks& blocks) const { return iterator(blocks, m_lowest, m_highest); }
	const iterator end(const Blocks& blocks) const { return iterator(blocks, BlockIndex::null(), BlockIndex::null()); }
	//TODO:
	//static_assert(std::forward_iterator<iterator>);
	CuboidView getView(const Blocks& blocks) const;
	CuboidSurfaceView getSurfaceView(const Blocks& blocks) const;
	std::wstring toString() const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Cuboid, m_highest, m_lowest);
};
struct CuboidView : public std::ranges::view_interface<CuboidView>
{
	const Cuboid cuboid;
	const Blocks& blocks;
	CuboidView() = default;
	CuboidView(const Blocks& b, const Cuboid c) : cuboid(c), blocks(b) { }
	Cuboid::iterator begin() const { return cuboid.begin(blocks); }
	Cuboid::iterator end() const { return cuboid.end(blocks); }
};
struct CuboidSurfaceView : public std::ranges::view_interface<CuboidSurfaceView>
{
	const Cuboid cuboid;
	const Blocks& blocks;
	CuboidSurfaceView() = default;
	CuboidSurfaceView(const Blocks& b, const Cuboid c) : cuboid(c), blocks(b) { }
	struct Iterator
	{
		const CuboidSurfaceView& view;
		Cuboid face;
		Point3D current;
		Facing6 facing = Facing6::Below;
		void setFace();
		void setToEnd();
		Iterator(const CuboidSurfaceView& v);
		Iterator& operator++();
		Iterator operator++(int);
		bool operator==(const Iterator& other) const;
		bool operator!=(const Iterator& other) const { return !(*this == other); }
		std::pair<BlockIndex, Facing6> operator*();
	};
	Iterator begin() const { return {*this}; }
	Iterator end() const { Iterator output(*this); output.setToEnd(); return output; }
	//static_assert(std::forward_iterator<Iterator>);
};