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
	OffsetCuboid(const Offset3D& high, const Offset3D& low);
	OffsetCuboid(const OffsetCuboid& other) = default;
	OffsetCuboid& operator=(const OffsetCuboid& other) = default;
	[[nodiscard]] std::strong_ordering operator<=>(const OffsetCuboid& other) const = default;
	[[nodiscard]] bool operator==(const OffsetCuboid& other) const = default;
	[[nodiscard]] bool exists() const { return m_high.exists(); }
	[[nodiscard]] uint volume() const;
	[[nodiscard]] bool contains(const Offset3D& offset) const;
	[[nodiscard]] bool contains(const OffsetCuboid& other) const;
	[[nodiscard]] bool intersects(const OffsetCuboid& other) const;
	[[nodiscard]] OffsetCuboid intersection(const OffsetCuboid& other) const;
	[[nodiscard]] bool canMerge(const OffsetCuboid& other) const;
	[[nodiscard]] bool isTouching(const OffsetCuboid& other) const;
	[[nodiscard]] bool isTouchingFace(const OffsetCuboid& other) const;
	[[nodiscard]] bool isTouchingFaceFromInside(const OffsetCuboid& other) const;
	[[nodiscard]] OffsetCuboid translate(const Point3D& previousPivot, const Point3D& nextPivot, const Facing4& previousFacing, const Facing4& nextFacing) const;
	[[nodiscard]] std::pair<Offset3D, Offset3D> toOffsetPair() const { return {m_high, m_low}; }
	[[nodiscard]] SmallSet<OffsetCuboid> getChildrenWhenSplitByCuboid(const OffsetCuboid& cuboid) const;
	[[nodiscard]] SmallSet<OffsetCuboid> getChildrenWhenSplitBy(const OffsetCuboid& cuboid) const { return getChildrenWhenSplitByCuboid(cuboid); }
	[[nodiscard]] SmallSet<OffsetCuboid> getChildrenWhenSplitBy(const Point3D& point) const { return getChildrenWhenSplitByCuboid({point, point}); }
	[[nodiscard]] OffsetCuboid relativeToPoint(const Point3D& point) const;
	[[nodiscard]] OffsetCuboid relativeToOffset(const Offset3D& point) const;
	[[nodiscard]] OffsetCuboid above() const;
	[[nodiscard]] OffsetCuboid getFace(const Facing6& facing) const;
	[[nodiscard]] bool hasAnyNegativeCoordinates() const;
	[[nodiscard]] OffsetCuboid sum(const OffsetCuboid& other) const;
	[[nodiscard]] OffsetCuboid difference(const Offset3D& other) const;
	[[nodiscard]] Offset3D getCenter() const;
	void maybeExpand(const OffsetCuboid& other);
	void inflate(const Distance& distance);
	void shift(const Facing6& direction, const Distance& distance);
	void shift(const Offset3D& offset, const Distance& distance);
	void rotateAroundPoint(const Offset3D& point, const Facing4& facing);
	void rotate2D(const Facing4& facing);
	void rotate2D(const Facing4& oldFacing, const Facing4& newFacing);
	void clear();
	class ConstIterator
	{
		OffsetCuboid* m_cuboid;
		Offset3D m_current;
	public:
		ConstIterator() = default;
		ConstIterator(const OffsetCuboid& cuboid, const Offset3D& current);
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
	[[nodiscard]] std::string toString() const { return "{" + m_high.toString() + "," + m_low.toString() + "}"; }
	static OffsetCuboid create(const Cuboid& cuboid, const Point3D& point);
	static OffsetCuboid create(const Cuboid& cuboid);
	static OffsetCuboid create(const Offset3D& a, const Offset3D& b);
};
inline void to_json(Json& data, const OffsetCuboid& cuboid) { data = cuboid.toJson(); }
inline void from_json(const Json& data, OffsetCuboid& cuboid) { cuboid.load(data); }