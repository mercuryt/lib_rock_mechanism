#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"

class Point3D;
struct Cuboid;
struct Sphere;

struct Point3DSet
{
	using Data = Eigen::Array<DistanceWidth, 3, Eigen::Dynamic>;
	Data data;
private:
	int32_t m_size = 0;
public:
	Point3DSet() = default;
	Point3DSet(const int32_t& capacity) { assert(capacity != 0); reserve(capacity); }
	void reserve(const int32_t& capacity) { data.conservativeResize(3, capacity); }
	void resize(const int32_t& capacity) { if(m_size < capacity) reserve(capacity); m_size = capacity; }
	void insert(const Point3D& point);
	void clear() { m_size = 0; };
	[[nodiscard]] Point3D operator[](const int32_t& index) const;
	[[nodiscard]] int32_t size() const { return m_size; }
	[[nodiscard]] int32_t capacity() const { return data.size(); }
	[[nodiscard]] Cuboid boundry() const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfContainedPoints(const Cuboid& cuboid) const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfContainedPoints(const Sphere& sphere) const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfFacedTwordsPoints(const Point3D& location, const Facing4& facing) const;
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesFacingTwords(const Point3D& location, const Eigen::Array<Facing4, 1, Eigen::Dynamic>& facings) const;
	[[nodiscard]] Eigen::Array<int32_t, 1, Eigen::Dynamic> distancesSquare(const Point3D& location) const;
	[[nodiscard]] Eigen::Array<bool, 2, Eigen::Dynamic> canSeeAndCanBeSeenByDistanceAndFacingFilter(const Point3D& location, const Facing4& facing, const DistanceSquared& visionRangeSquared, const Eigen::Array<Facing4, 1, Eigen::Dynamic>& facings, const Eigen::Array<Facing4, 1, Eigen::Dynamic>& visionRangesSquared) const;
	[[nodiscard]] static Point3DSet fromPointSet(const auto& points)
	{
		Point3DSet output;
		for(const Point3D& point : points)
			output.insert(point);
		return output;
	}
	[[nodiscard]] static Point3DSet fromCuboidSet(const auto& cuboids)
	{
		Point3DSet output;
		for(const Cuboid& cuboid : cuboids)
			for(const Point3D& point : cuboid)
				output.insert(point);
		return output;
	}
	class ConstIterator
	{
		const Point3DSet& m_set;
		int32_t m_index;
	public:
		ConstIterator(const Point3DSet& set, const int32_t& index) : m_set(set), m_index(index) { }
		void operator++() { ++m_index; }
		[[nodiscard]] ConstIterator operator++(int32_t) { auto copy = *this; ++(*this); return copy; }
		[[nodiscard]] Point3D operator*() const;
		[[nodiscard]] bool operator==(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index != other.m_index; }
		[[nodiscard]] std::strong_ordering operator<=>(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index <=> other.m_index; }
	};
	ConstIterator begin() const { return ConstIterator(*this, 0); }
	ConstIterator end() const { return ConstIterator(*this, m_size); }
};