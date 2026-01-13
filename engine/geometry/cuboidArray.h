#pragma once
#include "cuboid.h"
#include "../dataStructures/smallSet.h"

struct Sphere;
struct ParamaterizedLine;
struct CuboidSet;

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
	void insert(const int& index, const Cuboid& cuboid);
	void erase(const int& index);
	void clear();
	[[nodiscard]] bool operator==(const CuboidArray&) const;
	[[nodiscard]] int getCapacity() const;
	[[nodiscard]] const Cuboid operator[](const int& index) const;
	[[nodiscard]] Cuboid boundry() const;
	[[nodiscard]] bool intersects(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const Point3D& point) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const CuboidSet& cuboids) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const Sphere& sphere) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const ParamaterizedLine& line) const;
	[[nodiscard]] BoolArray indicesOfContainedCuboids(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfContainedCuboids(const CuboidSet& cuboids) const;
	[[nodiscard]] BoolArray indicesOfContainedCuboids(const Sphere& sphere) const;
	[[nodiscard]] BoolArray indicesOfCuboidsContaining(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfCuboidsContaining(const CuboidSet& cuboids) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboidsWhereThereIsADifferenceInEntranceAndExitZ(const ParamaterizedLine& line) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboidsLowZOnly(const Point3D& point) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboidsLowZOnly(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboidsLowZOnly(const Sphere& sphere) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboidsLowZOnly(const ParamaterizedLine& line) const;
	[[nodiscard]] BoolArray indicesOfIntersectingCuboidsLowZOnly(const CuboidSet& cuboids) const;
	[[nodiscard]] BoolArray indicesOfMergeableCuboids(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfTouchingCuboids(const Cuboid& cuboid) const;
	[[nodiscard]] BoolArray indicesOfTouchingCuboids(const CuboidSet& cuboids) const;
	[[nodiscard]] int indexOfCuboid(const Cuboid& cuboid) const;
	[[nodiscard]] bool anyOverlap() const;
	[[nodiscard]] __attribute__((noinline)) std::string toString() const;
	[[nodiscard]] __attribute__((noinline)) Cuboid at(int i) const;
	[[nodiscard]] __attribute__((noinline)) bool contains(const Cuboid& cuboid) const;
	static CuboidArray<capacity> create(const auto& source)
	{
		CuboidArray<capacity> output;
		int i = 0;
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
		int m_index;
	public:
		ConstIterator(const CuboidArray& set, const int& index) : m_set(set), m_index(index) { }
		ConstIterator operator++() { ++m_index; return *this; }
		[[nodiscard]] ConstIterator operator++(int32_t) { auto output = *this; ++(*this); return output; }
		ConstIterator operator--() { assert(m_index > 0); --m_index; return *this; }
		[[nodiscard]] ConstIterator operator--(int32_t) { auto output = *this; --(*this); return output; }
		[[nodiscard]] Cuboid operator*() const { return m_set[m_index]; }
		[[nodiscard]] bool operator==(const ConstIterator& other) const { assert(&m_set == &other.m_set); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator& other) const { assert(&m_set == &other.m_set); return m_index != other.m_index; }
		[[nodiscard]] ConstIterator operator-(const ConstIterator& other) const { assert(&m_set == &other.m_set); return (*this) - other.m_index; }
		[[nodiscard]] ConstIterator operator+(const ConstIterator& other) const { assert(&m_set == &other.m_set); return (*this) + other.m_index; }
		[[nodiscard]] ConstIterator operator-(const int& value) const { assert(m_index >= value); auto copy = *this; copy.m_index -= value; return copy; }
		[[nodiscard]] ConstIterator operator+(const int& value) const { auto copy = *this; copy.m_index += value; return copy; }
		[[nodiscard]] std::strong_ordering operator<=>(const ConstIterator& other) const { assert(&m_set == &other.m_set); return m_index <=> other.m_index; }
	};
	ConstIterator begin() const { return ConstIterator(*this, 0); }
	ConstIterator end() const { return ConstIterator(*this, capacity); }
};
template<int capacity>
inline void to_json(Json& data, const CuboidArray<capacity>& array) { std::vector<Cuboid> vector; vector.reserve(capacity); for(const Cuboid& cuboid : array) vector.push_back(cuboid); data = vector; }
template<int capacity>
inline void from_json(const Json& data, CuboidArray<capacity>& array) { auto vector = data.get<std::vector<Cuboid>>(); array = CuboidArray<capacity>::create(vector); }