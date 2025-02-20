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
	// TODO: change name to boundry.
	Cuboid m_boundingBox;
	uint m_size = 0;
	uint m_capacity = 0;
public:
	CuboidSetSIMD(uint capacity = 8) { assert(capacity != 0); reserve(capacity); }
	CuboidSetSIMD(const CuboidSetSIMD& other) { m_high = other.m_high; m_low = other.m_low; m_boundingBox = other.m_boundingBox; m_size = other.m_size; m_capacity = other.m_capacity; }
	CuboidSetSIMD(CuboidSetSIMD&& other) noexcept { m_high = std::move(other.m_high); m_low = std::move(other.m_low); m_boundingBox = other.m_boundingBox; m_size = other.m_size; m_capacity = other.m_capacity; }
	template<class CuboidSetT>
	CuboidSetSIMD(const CuboidSetT& contents) { load(contents); }
	void operator=(const CuboidSetSIMD& other) { m_high = other.m_high; m_low = other.m_low; m_boundingBox = other.m_boundingBox; m_size = other.m_size; m_capacity = other.m_capacity; }
	void operator=(CuboidSetSIMD&& other) { m_high = std::move(other.m_high); m_low = std::move(other.m_low); m_boundingBox = other.m_boundingBox; m_size = other.m_size; m_capacity = other.m_capacity; }
	void reserve(uint capacity);
	void insert(const Cuboid& cuboid);
	void update(const uint index, const Cuboid& cuboid);
	void erase(const Cuboid& cuboid);
	void erase(uint index);
	void clear();
	template<class CuboidSetT>
	void load(const CuboidSetT& contents) { assert(empty()); reserve(contents.size()); for(const Cuboid& cuboid : contents) insert(cuboid); }
	[[nodiscard]] Cuboid operator[](const uint& index) const { return {Coordinates(m_high.col(index)), Coordinates(m_low.col(index)) }; }
	[[nodiscard]] bool intersects(const Cuboid& cuboid) const;
	[[nodiscard]] bool containsAsMember(const Cuboid& cuboid) const;
	[[nodiscard]] bool empty() const { return m_size == 0; }
	[[nodiscard]] uint size() const { return m_size; }
	[[nodiscard]] uint capacity() const { return m_capacity; }
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfIntersectingCuboids(const Cuboid& cuboid) const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfContainedCuboids(const Cuboid& cuboid) const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfIntersectingCuboids(const Sphere& sphere) const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfContainedCuboids(const Sphere& sphere) const;
	[[nodiscard]] const Cuboid& boundry() const { return m_boundingBox; }
	[[nodiscard]] std::vector<Cuboid> toVector() const;
	class ConstIterator
	{
		const CuboidSetSIMD& m_set;
		uint m_index = 0;
	public:
		ConstIterator(const CuboidSetSIMD& set, uint index) : m_set(set), m_index(index) { }
		[[nodiscard]] Cuboid operator*() const { return m_set[m_index]; }
		void operator++(int) { ++m_index; }
		ConstIterator operator++() { auto copy = *this; ++m_index; return copy; }
		[[nodiscard]] bool operator==(const ConstIterator& other) const { assert(&m_set == &other.m_set); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator& other) const { return !((*this) == other); }
	};
	ConstIterator begin() const { return {*this, 0}; }
	ConstIterator end() const { return {*this, m_size}; }
};

inline void to_json(Json& data, const CuboidSetSIMD& set) { data = set.toVector(); }
inline void from_json(const Json& data, CuboidSetSIMD& set) { set.load(data.get<std::vector<Cuboid>>()); }

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