#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>

#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include "point3D.h"
class CuboidView;
class CuboidSurfaceView;
struct Sphere;
class Offset3D;
struct OffsetCuboid;
struct CuboidSet;
struct ParamaterizedLine;
struct Cuboid
{
	using PointType = Point3D;
	Point3D m_high;
	Point3D m_low;
	Cuboid() = default;
	Cuboid(const Point3D& high, const Point3D& low);
	Cuboid(const Cuboid&) = default;
	Cuboid& operator=(const Cuboid&) = default;
	void merge(const Cuboid& cuboid);
	void setFrom(const Point3D& point);
	void setFrom(const Point3D& high, const Point3D& low);
	void setFrom(const Offset3D& high, const Offset3D& low);
	void clear();
	void shift(const Facing6& direction, const Distance& distance);
	void shift(const Offset3D& offset, const Distance& distance);
	// MaybeShift only shifts if the result is non negitive.
	void maybeShift(const Facing6& direction, const Distance& distance);
	void maybeShift(const Offset3D& offset, const Distance& distance);
	void rotateAroundPoint(const Point3D& point, const Facing4& rotation);
	void setMaxZ(const Distance& distance);
	void maybeExpand(const Cuboid& other);
	void maybeExpand(const Point3D& point);
	void inflate(const Distance& distance);
	[[nodiscard]] Cuboid boundry() const { return *this; }
	[[nodiscard]] SmallSet<Point3D> toSet() const;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] bool contains(const Cuboid& cuboid) const;
	[[nodiscard]] bool contains(const Offset3D& offset) const;
	[[nodiscard]] bool contains(const OffsetCuboid& cuboid) const;
	[[nodiscard]] bool containsAnyPoints(const auto& points) const
	{
		for(const Point3D& point : points)
			if(contains(point))
				return true;
		return false;
	}
	[[nodiscard]] bool canMerge(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid canMergeSteal(const Cuboid& cuboid) const;
	// TODO: this should return an OffsetCuboid.
	[[nodiscard]] Cuboid sum(const Cuboid& cuboid) const;
	[[nodiscard]] OffsetCuboid difference(const Point3D& other) const;
	[[nodiscard]] Cuboid intersection(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid intersection(const OffsetCuboid& cuboid) const;
	[[nodiscard]] Cuboid intersection(const Point3D& point) const;
	[[nodiscard]] Point3D intersectionPoint(const Point3D& point) const;
	[[nodiscard]] Point3D intersectionPoint(const Cuboid& cuboid) const;
	[[nodiscard]] Point3D intersectionPoint(const CuboidSet& cuboid) const;
	[[nodiscard]] std::pair<Point3D, Point3D> intersectionPoints(const ParamaterizedLine& line) const;
	[[nodiscard]] Point3D intersectionPointForFace(const ParamaterizedLine& line, const Facing6& face) const;
	[[nodiscard]] OffsetCuboid above() const;
	[[nodiscard]] Cuboid getFace(const Facing6& faceing) const;
	[[nodiscard]] bool intersects(const Point3D& point) const;
	[[nodiscard]] bool intersects(const Cuboid& cuboid) const;
	[[nodiscard]] bool overlapsWithSphere(const Sphere& sphere) const;
	[[nodiscard]] int volume() const;
	[[nodiscard]] bool empty() const { return m_high.empty(); }
	[[nodiscard]] bool exists() const { return m_high.exists(); }
	[[nodiscard]] bool operator==(const Cuboid& cuboid) const;
	[[nodiscard]] Point3D getCenter() const;
	[[nodiscard]] Distance dimensionForFacing(const Facing6& facing) const;
	[[nodiscard]] Facing6 getFacingTwordsOtherCuboid(const Cuboid& cuboid) const;
	[[nodiscard]] bool isSomeWhatInFrontOf(const Point3D& position, const Facing4& facing) const;
	[[nodiscard]] bool isTouching(const Cuboid& cuboid) const;
	[[nodiscard]] bool isTouching(const Point3D& point) const;
	[[nodiscard]] bool isTouchingFace(const Cuboid& position) const;
	[[nodiscard]] bool isTouchingFaceFromInside(const Cuboid& position) const;
	// TODO: Should this return a CuboidArray<6>?
	[[nodiscard]] SmallSet<Cuboid> getChildrenWhenSplitByCuboid(const Cuboid& cuboid) const;
	// Overloaded methods for getChildrenWhenSplitBy.
	// TODO: Add variants for line and sphere?
	[[nodiscard]] SmallSet<Cuboid> getChildrenWhenSplitBy(const Cuboid& cuboid) const { return getChildrenWhenSplitByCuboid(cuboid); }
	[[nodiscard]] SmallSet<Cuboid> getChildrenWhenSplitBy(const Point3D& point) const { return getChildrenWhenSplitByCuboid({point, point}); }
	[[nodiscard]] std::pair<Cuboid, Cuboid> getChildrenWhenSplitBelowCuboid(const Cuboid& cuboid) const;
	[[nodiscard]] OffsetCuboid translate(const Point3D& previousPivot, const Point3D& nextPivot, const Facing4& previousFacing, const Facing4& nextFacing) const;
	[[nodiscard]] OffsetCuboid offsetTo(const Point3D& point) const;
	[[nodiscard]] SmallSet<Cuboid> sliceAtEachZ() const;
	[[nodiscard]] int countIf(auto&& condition) const
	{
		int output = 0;
		for(const Point3D& point : *this)
			if(condition(point))
				++output;
		return output;
	}
	[[nodiscard]] static Cuboid fromPoint(const Point3D& point);
	[[nodiscard]] static Cuboid fromPointPair(const Point3D& a, const Point3D& b);
	[[nodiscard]] static Cuboid fromPointSet(const SmallSet<Point3D>& set);
	[[nodiscard]] static Cuboid createCube(const Point3D& center, const Distance& width);
	[[nodiscard]] static Cuboid create(const OffsetCuboid& cuboid);
	class ConstIterator
	{
	private:
		Point3D m_start;
		Point3D m_end;
		Point3D m_current;
		void setToEnd();
	public:
		ConstIterator(const Point3D& low, const Point3D& high);
		ConstIterator(const ConstIterator& other) = default;
		ConstIterator& operator=(const ConstIterator& other);
		ConstIterator& operator++();
		[[nodiscard]] ConstIterator operator++(int);
		[[nodiscard]] bool operator==(const ConstIterator& other) const { return m_current == other.m_current; }
		[[nodiscard]] bool operator!=(const ConstIterator& other) const { return !(*this == other); }
		[[nodiscard]] const Point3D operator*() const;
	};
	ConstIterator begin() { return ConstIterator(m_low, m_high); }
	ConstIterator end() { return ConstIterator(Point3D::null(), Point3D::null()); }
	const ConstIterator begin() const { return ConstIterator(m_low, m_high); }
	const ConstIterator end() const { return ConstIterator(Point3D::null(), Point3D::null()); }
	//TODO:
	//static_assert(std::forward_iterator<iterator>);
	CuboidSurfaceView getSurfaceView() const;
	std::string toString() const;
	[[nodiscard]] std::strong_ordering operator<=>(const Cuboid& other) const;
	static Cuboid null() { return Cuboid(); }
	static Cuboid create(const Offset3D& high, const Offset3D& low) { assert(low.z() >= 0); return {Point3D::create(high), Point3D::create(low)}; }
	static Cuboid create(const Point3D& high, const Point3D& low);
	struct Hash{
		[[nodiscard]] size_t operator()(const Cuboid& cuboid) { return Point3D::Hash()(cuboid.m_high); }
	};
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Cuboid, m_high, m_low);
};
struct CuboidSurfaceView : public std::ranges::view_interface<CuboidSurfaceView>
{
	const Cuboid cuboid;
	CuboidSurfaceView() = default;
	CuboidSurfaceView(const Cuboid c) : cuboid(c) { }
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
		std::pair<Point3D, Facing6> operator*();
	};
	Iterator begin() const { return {*this}; }
	Iterator end() const { Iterator output(*this); output.setToEnd(); return output; }
	//static_assert(std::forward_iterator<Iterator>);
};

// Check that lambda has signature (Cuboid) -> bool.
template<typename T>
concept CuboidToBool = requires(T f, Cuboid cuboid) {
	{ f(cuboid) } -> std::same_as<bool>;
};

