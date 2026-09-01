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
struct CuboidSet;
struct Cuboid
{
	using PointType = Point3D;
	struct Primitive
	{
		std::array<DistanceWidth, 6> data;
		bool operator<=>(const Primitive&) const = default;
		bool operator==(const Primitive&) const = default;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Primitive, data);
	};
	Point3D m_high;
	Point3D m_low;
	Cuboid() = default;
	Cuboid(Point3D high, Point3D low);
	Cuboid(const Cuboid&) = default;
	Cuboid& operator=(const Cuboid&) = default;
	void merge(Cuboid cuboid);
	void setFrom(Point3D point);
	void setFrom(Point3D high, Point3D low);
	void setFrom(Offset3D high, Offset3D low);
	void clear();
	void shift(Facing6 direction, Distance distance = {1});
	void shift(Offset3D offset, Distance distance = {1});
	// MaybeShift only shifts if the result is non negitive.
	void maybeShift(Facing6 direction, Distance distance = {1});
	void maybeShift(Offset3D offset, Distance distance = {1});
	void rotateAroundPoint(Point3D point, Facing4 rotation);
	void setMaxZ(Distance distance);
	void maybeExpand(Cuboid other);
	void maybeExpand(Point3D point);
	void inflate(Distance distance = {1});
	void inflateHorizontal(Distance distance = {1});
	void inflateHorizontalAndBelow(Distance distance = {1});
	void inflateVertical(Distance distance = {1});
	void inflateDirection(Facing6 direction, Distance distance = {1});
	void deflate(Distance distance = {1});
	void deflateDirection(Facing6 direction, Distance distance = {1});
	void maximizeDirection(Facing6 direction);
	[[nodiscard]] Cuboid maybeExpanded(Point3D point) const;
	[[nodiscard]] Cuboid shifted(Facing6 facing, Distance distance = {1}) const;
	[[nodiscard]] Cuboid maybeShifted(Facing6 facing, Distance distance = {1}) const;
	[[nodiscard]] Cuboid inflated(Distance distance = {1}) const;
	[[nodiscard]] Cuboid inflatedHorizontal(Distance distance = {1}) const;
	[[nodiscard]] Cuboid inflatedHorizontalAndBelow(Distance distance = {1}) const;
	[[nodiscard]] Cuboid inflatedDirection(Facing6 facing, Distance distance = {1}) const;
	[[nodiscard]] Cuboid deflated(Distance distance = {1}) const;
	[[nodiscard]] constexpr Primitive get() const { return {.data={m_high.x().get(), m_high.y().get(), m_high.z().get(), m_low.x().get(), m_low.y().get(), m_low.z().get()}}; }
	[[nodiscard]] constexpr static Primitive nullPrimitive() { auto d = Distance::null().get(); return {.data={d, d, d, d, d, d}}; }
	[[nodiscard]] Cuboid boundry() const { return *this; }
	[[nodiscard]] CuboidSet toSet() const;
	[[nodiscard]] bool contains(Point3D point) const;
	[[nodiscard]] bool contains(Cuboid cuboid) const;
	[[nodiscard]] bool contains(const CuboidSet& cuboids) const;
	[[nodiscard]] bool contains(Offset3D offset) const;
	[[nodiscard]] bool contains(OffsetCuboid cuboid) const;
	[[nodiscard]] bool containsAnyPoints(const auto& points) const
	{
		for(Point3D point : points)
			if(contains(point))
				return true;
		return false;
	}
	[[nodiscard]] bool canMerge(Cuboid cuboid) const;
	[[nodiscard]] Cuboid canMergeSteal(Cuboid cuboid) const;
	// TODO: this should return an OffsetCuboid.
	[[nodiscard]] Cuboid sum(Cuboid cuboid) const;
	[[nodiscard]] OffsetCuboid difference(Point3D other) const;
	[[nodiscard]] Cuboid intersection(Cuboid cuboid) const;
	[[nodiscard]] CuboidSet intersection(const CuboidSet& cuboids) const;
	[[nodiscard]] Cuboid intersection(OffsetCuboid cuboid) const;
	[[nodiscard]] Cuboid intersection(Point3D point) const;
	[[nodiscard]] Point3D intersectionPoint(Point3D point) const;
	[[nodiscard]] Point3D intersectionPoint(Cuboid cuboid) const;
	[[nodiscard]] Point3D intersectionPoint(const CuboidSet& cuboid) const;
	[[nodiscard]] std::pair<Point3D, Point3D> intersectionPoints(const ParamaterizedLine& line) const;
	[[nodiscard]] Point3D intersectionPointForFace(const ParamaterizedLine& line, const Facing6 face) const;
	[[nodiscard]] OffsetCuboid above() const;
	[[nodiscard]] Cuboid getFace(Facing6 facing) const;
	[[nodiscard]] Cuboid getFace(Facing4 facing) const;
	[[nodiscard]] Cuboid getFaceNorth() const;
	[[nodiscard]] Cuboid getFaceSouth() const;
	[[nodiscard]] Cuboid getFaceEast() const;
	[[nodiscard]] Cuboid getFaceWest() const;
	[[nodiscard]] Cuboid getFaceAbove() const;
	[[nodiscard]] Cuboid getFaceBelow() const;
	[[nodiscard]] bool intersects(Point3D point) const;
	[[nodiscard]] bool intersects(Cuboid cuboid) const;
	[[nodiscard]] bool intersects(const CuboidSet& cuboid) const;
	[[nodiscard]] bool overlapsWithSphere(const Sphere& sphere) const;
	[[nodiscard]] bool overlapX(Cuboid other) const;
	[[nodiscard]] bool overlapY(Cuboid other) const;
	[[nodiscard]] bool overlapZ(Cuboid other) const;
	[[nodiscard]] int volume() const;
	[[nodiscard]] bool empty() const { return m_high.empty(); }
	[[nodiscard]] bool exists() const { return m_high.exists(); }
	[[nodiscard]] bool operator==(Cuboid cuboid) const;
	[[nodiscard]] Point3D getCenter() const;
	[[nodiscard]] Distance dimensionForFacing(const Facing6 facing) const;
	[[nodiscard]] Facing6 getFacing6TwordsOtherCuboid(Cuboid cuboid) const;
	[[nodiscard]] Facing4 getFacing4TwordsOtherCuboid(Cuboid cuboid) const;
	[[nodiscard]] bool isSomeWhatInFrontOf(Point3D position, const Facing4 facing) const;
	[[nodiscard]] bool isTouching(Cuboid cuboid) const;
	[[nodiscard]] bool isTouching(Point3D point) const;
	[[nodiscard]] bool isTouchingFace(Cuboid position) const;
	[[nodiscard]] bool isTouchingFaceFromInside(Cuboid position) const;
	// TODO: Should this return a CuboidArray<6>?
	[[nodiscard]] SmallSet<Cuboid> getChildrenWhenSplitByCuboid(Cuboid cuboid) const;
	// Overloaded methods for getChildrenWhenSplitBy.
	// TODO: Add variants for line and sphere?
	[[nodiscard]] SmallSet<Cuboid> getChildrenWhenSplitBy(Cuboid cuboid) const { return getChildrenWhenSplitByCuboid(cuboid); }
	[[nodiscard]] SmallSet<Cuboid> getChildrenWhenSplitBy(Point3D point) const { return getChildrenWhenSplitByCuboid({point, point}); }
	[[nodiscard]] std::pair<Cuboid, Cuboid> getChildrenWhenSplitBelowCuboid(Cuboid cuboid) const;
	[[nodiscard]] OffsetCuboid translate(Point3D previousPivot, Point3D nextPivot, const Facing4 previousFacing, const Facing4 nextFacing) const;
	[[nodiscard]] OffsetCuboid offsetTo(Point3D point) const;
	[[nodiscard]] SmallSet<Cuboid> sliceAtEachZ() const;
	[[nodiscard]] Cuboid slicedAtZ(const Distance z) const;
	[[nodiscard]] Distance sizeX() const;
	[[nodiscard]] Distance sizeY() const;
	[[nodiscard]] Distance sizeZ() const;
	[[nodiscard]] Point3D clamp(Point3D point) const;
	[[nodiscard]] Point3D nearestPointTo(Cuboid other) const;
	[[nodiscard]] Point3D furthestPointFrom(Point3D point) const;
	[[nodiscard]] Point3D furthestPointFrom(Cuboid cuboid) const;
	[[nodiscard]] Distance distanceTo(Cuboid other) const;
	[[nodiscard]] Distance distanceTo(Point3D point) const;
	[[nodiscard]] std::pair<Cuboid, Cuboid> splitByLongestDimension() const;
	[[nodiscard]] int countIf(auto&& condition) const
	{
		int output = 0;
		for(Point3D point : *this)
			if(condition(point))
				++output;
		return output;
	}
	[[nodiscard]] static Cuboid fromPoint(Point3D point);
	[[nodiscard]] static Cuboid fromPointPair(Point3D a, Point3D b);
	[[nodiscard]] static Cuboid fromPointSet(const SmallSet<Point3D>& set);
	[[nodiscard]] static Cuboid createCube(Point3D center, const Distance width);
	[[nodiscard]] static Cuboid create(OffsetCuboid cuboid);
	[[nodiscard]] static Cuboid create(Primitive primitive);
	class ConstIterator
	{
	private:
		Point3D m_start;
		Point3D m_end;
		Point3D m_current;
		void setToEnd();
	public:
		ConstIterator(Point3D low, Point3D high);
		ConstIterator(const ConstIterator& other) = default;
		ConstIterator& operator=(const ConstIterator& other);
		ConstIterator& operator++();
		[[nodiscard]] ConstIterator operator++(int);
		[[nodiscard]] bool operator==(const ConstIterator& other) const { return m_current == other.m_current; }
		[[nodiscard]] bool operator!=(const ConstIterator& other) const { return !(*this == other); }
		[[nodiscard]] Point3D operator*() const;
	};
	ConstIterator begin() { return ConstIterator(m_low, m_high); }
	ConstIterator end() { return ConstIterator(Point3D::null(), Point3D::null()); }
	const ConstIterator begin() const { return ConstIterator(m_low, m_high); }
	const ConstIterator end() const { return ConstIterator(Point3D::null(), Point3D::null()); }
	//TODO:
	//static_assert(std::forward_iterator<iterator>);
	CuboidSurfaceView getSurfaceView() const;
	[[nodiscard]] GDB_CALLABLE std::string toS() const;
	[[nodiscard]] std::strong_ordering operator<=>(Cuboid other) const;
	static Cuboid null() { return Cuboid(); }
	static Cuboid create(const Offset3D high, const Offset3D low) { assert(low.z() >= 0); return {Point3D::create(high), Point3D::create(low)}; }
	static Cuboid create(Point3D high, Point3D low);
	static Cuboid create(Point3D point) { return create(point, point); }
	struct Hash{
		[[nodiscard]] size_t operator()(Cuboid cuboid) { return Point3D::Hash()(cuboid.m_high); }
	};
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Cuboid, m_high, m_low);
};
struct CuboidSurfaceView : public std::ranges::view_interface<CuboidSurfaceView>
{
	Cuboid cuboid;
	CuboidSurfaceView() = default;
	CuboidSurfaceView(Cuboid c) : cuboid(c) { }
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

