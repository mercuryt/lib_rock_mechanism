#pragma once
#include "cuboid.h"
#include "../dataStructures/smallSet.h"

struct Sphere;
struct ParamaterizedLine;

template<int capacity>
struct CuboidArray
{
public:
	using BoolArray = Eigen::Array<bool, 1, capacity>;
	using Bool3DArray = Eigen::Array<bool, 3, capacity>;
	using FloatArray = Eigen::Array<float, 1, capacity>;
	using Float3DArray = Eigen::Array<float, 3, capacity>;
	using PointArray = Eigen::Array<DistanceWidth, 3, capacity>;
	using Offset3DArray = Eigen::Array<OffsetWidth, 3, capacity>;
	using OffsetArray = Eigen::Array<OffsetWidth, 1, capacity>;
private:
	PointArray m_high;
	PointArray m_low;
public:
	CuboidArray();
	CuboidArray(const CuboidArray&) = default;
	CuboidArray(CuboidArray&&) = default;
	CuboidArray& operator=(const CuboidArray&) = default;
	CuboidArray& operator=(CuboidArray&&) = default;
	void insert(const uint16_t& index, const Cuboid& cuboid);
	void erase(const uint16_t& index);
	void clear();
	[[nodiscard]] __attribute__((noinline)) bool contains(const Cuboid& cuboid) const;
	[[nodiscard]] int getCapacity() const;
	[[nodiscard]] const Cuboid operator[](const uint& index) const;
	[[nodiscard]] Cuboid boundry() const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const Point3D& point) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfContainedCuboids(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfCuboidsContaining(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const Sphere& sphere) const;
	[[nodiscard]] BoolArray indicesOfContainedCuboids(const Sphere& sphere) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const ParamaterizedLine& line) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboidsLowZOnly(const Point3D& point) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboidsLowZOnly(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboidsLowZOnly(const Sphere& sphere) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboidsLowZOnly(const ParamaterizedLine& line) const;
	[[nodiscard]] BoolArray indicesOfMergeableCuboids(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfTouchingCuboids(const Cuboid& cuboid) const;
	[[nodiscard]] __attribute__((noinline)) std::string toString() const;
	[[nodiscard]] __attribute__((noinline)) Cuboid at(uint i) const;
	static CuboidArray<capacity> create(const auto& source)
	{
		CuboidArray<capacity> output;
		uint i = 0;
		for(const Cuboid& cuboid : source)
		{
			output.insert(i, cuboid);
			++i;
		}
		return output;
	}
	class ConstIterator
	{
		const CuboidArray& m_set;
		uint m_index;
	public:
		ConstIterator(const CuboidArray& set, const uint& index) : m_set(set), m_index(index) { }
		void operator++() { ++m_index; }
		[[nodiscard]] ConstIterator operator++(int) { auto copy = *this; ++(*this); return copy; }
		[[nodiscard]] Cuboid operator*() const { return m_set[m_index]; }
		[[nodiscard]] bool operator==(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index != other.m_index; }
		[[nodiscard]] std::strong_ordering operator<=>(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index <=> other.m_index; }
	};
	ConstIterator begin() const { return ConstIterator(*this, 0); }
	ConstIterator end() const { return ConstIterator(*this, capacity); }
};
template<int size>
inline void to_json(Json& data, const CuboidArray<size>& set) { data = SmallSet<Cuboid>::create(set); }
template<int size>
inline void from_json(const Json& data, CuboidArray<size>& set) { auto smallSet = data.get<SmallSet<Cuboid>>(); set = CuboidArray<size>::create(smallSet); }