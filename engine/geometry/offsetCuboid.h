#pragma once
#include "point3D.h"
#include "cuboid.h"
#include "../dataStructures/smallSet.h"
// TODO: Share code with Cuboid?

struct OffsetCuboid;

struct OffsetCuboid
{
	using PointType = Offset3D;
	Offset3D m_high;
	Offset3D m_low;
	OffsetCuboid() = default;
	OffsetCuboid(Offset3D high, Offset3D low);
	OffsetCuboid(const OffsetCuboid& other) = default;
	OffsetCuboid& operator=(const OffsetCuboid& other) = default;
	[[nodiscard]] std::strong_ordering operator<=>(const OffsetCuboid& other) const = default;
	[[nodiscard]] bool operator==(const OffsetCuboid& other) const = default;
	[[nodiscard]] bool exists() const { return m_high.exists(); }
	[[nodiscard]] bool empty() const { return m_high.empty(); }
	[[nodiscard]] int volume() const;
	[[nodiscard]] bool contains(Offset3D offset) const;
	[[nodiscard]] bool contains(OffsetCuboid other) const;
	[[nodiscard]] bool intersects(Offset3D offset) const;
	[[nodiscard]] bool intersects(OffsetCuboid other) const;
	[[nodiscard]] OffsetCuboid intersection(OffsetCuboid other) const;
	[[nodiscard]] bool canMerge(OffsetCuboid other) const;
	[[nodiscard]] bool isTouching(Offset3D offset) const;
	[[nodiscard]] bool isTouching(OffsetCuboid other) const;
	[[nodiscard]] bool isTouchingFace(OffsetCuboid other) const;
	[[nodiscard]] bool isTouchingFaceFromInside(OffsetCuboid other) const;
	[[nodiscard]] OffsetCuboid translate(Point3D previousPivot, Point3D nextPivot, Facing4 previousFacing, Facing4 nextFacing) const;
	[[nodiscard]] std::pair<Offset3D, Offset3D> toOffsetPair() const { return {m_high, m_low}; }
	[[nodiscard]] SmallSet<OffsetCuboid> getChildrenWhenSplitByCuboid(OffsetCuboid cuboid) const;
	[[nodiscard]] SmallSet<OffsetCuboid> getChildrenWhenSplitBy(OffsetCuboid cuboid) const { return getChildrenWhenSplitByCuboid(cuboid); }
	[[nodiscard]] SmallSet<OffsetCuboid> getChildrenWhenSplitBy(Point3D point) const { return getChildrenWhenSplitByCuboid({point, point}); }
	[[nodiscard]] OffsetCuboid relativeToPoint(Point3D point) const;
	[[nodiscard]] OffsetCuboid relativeToOffset(Offset3D point) const;
	[[nodiscard]] OffsetCuboid above() const;
	[[nodiscard]] OffsetCuboid getFace(Facing6 facing) const;
	[[nodiscard]] OffsetCuboid getFaceNorth() const;
	[[nodiscard]] OffsetCuboid getFaceSouth() const;
	[[nodiscard]] OffsetCuboid getFaceEast() const;
	[[nodiscard]] OffsetCuboid getFaceWest() const;
	[[nodiscard]] OffsetCuboid getFaceAbove() const;
	[[nodiscard]] OffsetCuboid getFaceBelow() const;
	[[nodiscard]] bool hasAnyNegativeCoordinates() const;
	[[nodiscard]] OffsetCuboid sum(OffsetCuboid other) const;
	[[nodiscard]] OffsetCuboid difference(Offset3D other) const;
	[[nodiscard]] Offset3D getCenter() const;
	[[nodiscard]] Offset3D clamp(Offset3D point) const;
	[[nodiscard]] Offset3D nearestPointTo(OffsetCuboid other) const;
	[[nodiscard]] Offset distanceTo(OffsetCuboid other) const;
	[[nodiscard]] Offset sizeX() const;
	[[nodiscard]] Offset sizeY() const;
	[[nodiscard]] Offset sizeZ() const;
	void maybeExpand(OffsetCuboid other);
	void inflate(Distance distance = {1});
	void inflateDirection(Facing6 facing, Distance distance = {1});
	void inflateHorizontal(Distance distance = {1});
	void inflateVertical(Distance distance = {1});
	void deflate(Distance distance = {1});
	void shift(Facing6 direction, Distance distance = {1});
	void shift(Offset3D offset, Distance distance = {1});
	// Provided for symetry with Cuboid, not actually useful.
	void maybeShift(Facing6 direction, Distance distance = {1}) { shift(direction, distance); }
	void maybeShift(Offset3D offset, Distance distance = {1}) { shift(offset, distance); }
	void rotateAroundPoint(Offset3D point, Facing4 facing);
	void rotate2D(Facing4 facing);
	void rotate2D(Facing4 oldFacing, Facing4 newFacing);
	void clear();
	class ConstIterator
	{
		OffsetCuboid* m_cuboid;
		Offset3D m_current;
	public:
		ConstIterator() = default;
		ConstIterator(OffsetCuboid cuboid, Offset3D current);
		ConstIterator& operator=(const ConstIterator& other) = default;
		[[nodiscard]] bool operator==(const ConstIterator& other) const;
		[[nodiscard]] Offset3D operator*() const;
		ConstIterator operator++();
		[[nodiscard]] ConstIterator operator++(int);
	};
	ConstIterator begin() const { return {*this, m_low}; }
	ConstIterator end() const { auto current = m_low; current.setZ(m_high.z() + 1); return {*this, current}; }
	[[nodiscard]] Json toJson() const;
	void load(const Json& data);
	[[nodiscard]] GDB_CALLABLE std::string toS() const;
	static OffsetCuboid create(Cuboid cuboid, Point3D point);
	static OffsetCuboid create(Cuboid cuboid);
	static OffsetCuboid create(Offset3D a, Offset3D b);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(OffsetCuboid, m_high, m_low);
};