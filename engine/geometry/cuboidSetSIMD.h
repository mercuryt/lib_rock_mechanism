#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop
#include "../types.h"
#include "cuboid.h"
#include "sphere.h"

class CuboidSetSIMD
{
	using Data = Eigen::Array<DistanceInBlocksWidth, 3, Eigen::Dynamic>;
	Data m_high;
	Data m_low;
	Cuboid m_boundingBox;
	uint m_size = 0;
	uint m_capacity = 0;
public:
	CuboidSetSIMD(uint capacity) { assert(capacity != 0); reserve(capacity); }
	void reserve(uint capacity);
	void insert(const Cuboid& cuboid);
	void clear();
	[[nodiscard]] Cuboid operator[](const uint& index) const { return {Coordinates(m_high.col(index)), Coordinates(m_low.col(index)) }; }
	[[nodiscard]] bool intersects(const Cuboid& cuboid) const;
	[[nodiscard]] uint size() const { return m_size; }
	[[nodiscard]] uint capacity() const { return m_capacity; }
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfIntersectingCuboids(const Cuboid& cuboid);
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfContainedCuboids(const Cuboid& cuboid);
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfIntersectingCuboids(const Sphere& sphere);
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfContainedCuboids(const Sphere& sphere);
	[[nodiscard]] const Cuboid& getBoundingBox() const { return m_boundingBox; }
};

template<int size>
class CuboidArray
{
	using Data = Eigen::Array<DistanceInBlocksWidth, 3, size>;
	Data m_high;
	Data m_low;
public:
	void insert(const uint& index, const Cuboid& cuboid)
	{
		m_high.col(index) = cuboid.m_highest.data;
		m_low.col(index) = cuboid.m_lowest.data;
	}
	[[nodiscard]] Cuboid operator[](const uint& index) const { return {Coordinates(m_high.col(index)), Coordinates(m_low.col(index)) }; }
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfIntersectingCuboids(const Cuboid& cuboid) const
	{
		return (
			(m_high >= cuboid.m_lowest.data.replicate(1, size)).colwise().all() &&
			(m_low <= cuboid.m_highest.data.replicate(1, size)).colwise().all()
		);
	}
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfContainedCuboids(const Cuboid& cuboid) const
	{
		return (
			(m_high <= cuboid.m_highest.data.replicate(1, size)).colwise().all() &&
			(m_low >= cuboid.m_lowest.data.replicate(1, size)).colwise().all()
		);
	}
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfIntersectingCuboids(const Sphere& sphere) const
	{
		int radius = sphere.radius.get();
		const Eigen::Array<int, 3, 1>& center = sphere.center.data.template cast<int>();
		return !(
			((m_high.template cast<int>() + radius) < center.replicate(1, size)).colwise().any() ||
			((m_low.template cast<int>() - radius) > center.replicate(1, size)).colwise().any()
		);
	}
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfContainedCuboids(const Sphere& sphere) const
	{
		int radius = sphere.radius.get();
		const Eigen::Array<int, 3, 1>& center = sphere.center.data.cast<int>();
		return !(
			((m_high.template cast<int>() - radius) >= center.replicate(1, size)).colwise().any() &&
			((m_low.template cast<int>() + radius) <= center.replicate(1, size)).colwise().any()
		);
	}
	class ConstIterator
	{
		const CuboidSetSIMD& m_set;
		uint m_index;
	public:
		ConstIterator(const CuboidSetSIMD& set, const uint& index) : m_set(set), m_index(index) { }
		void operator++() { ++m_index; }
		[[nodiscard]] ConstIterator operator++(int) { auto copy = *this; ++(*this); return copy; }
		[[nodiscard]] Cuboid operator*() const { return m_set[m_index]; }
		[[nodiscard]] bool operator==(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index != other.m_index; }
		[[nodiscard]] std::strong_ordering operator<=>(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index <=> other.m_index; }
	};
	ConstIterator begin() const { return ConstIterator(*this, 0); }
	ConstIterator end() const { return ConstIterator(*this, size); }
};