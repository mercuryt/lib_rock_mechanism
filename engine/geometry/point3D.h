// One Point3D has xyz dimensions of 1 meter by 1 meter by 2 meters.
// This File defines physical types.
// numericTypes/idTypes.h defines id types, which don't ever change.
// numericTypes/index.h defines index types, which do change and must be updated.

#pragma once
#include "../json.h"
#include "../numericTypes/types.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop
#include <compare>
#include <cstdint>
#include <iostream>
#include <string>

using Coordinates = Eigen::Array<DistanceWidth, 3, 1>;
using Offsets = Eigen::Array<OffsetWidth, 3, 1>;
struct Offset3D;
struct Cuboid;
struct OffsetCuboid;
class AdjacentIndex;

struct Point3D
{
	Coordinates data;
	using Primitive = Coordinates;
	using DimensionType = Distance;
	Point3D() { data.fill(Distance::null().get()); }
	Point3D(Coordinates v) : data(v) { }
	Point3D(const Distance& x, const Distance& y, const Distance& z) : data(x.get(), y.get(), z.get()) { }
	Point3D(const Point3D& other) : data(other.data) { }
	void clampHigh(const Point3D& other);
	void clampLow(const Point3D& other);
	Point3D min(const Point3D& other) const;
	Point3D max(const Point3D& other) const;
	void clear();
	void setX(const Distance& x) { data[0] = x.get(); }
	void setY(const Distance& y) { data[1] = y.get(); }
	void setZ(const Distance& z) { data[2] = z.get(); }
	void swap(Point3D& other) { std::swap(data, other.data); }
	Point3D& operator+=(const Offset3D& other);
	Point3D& operator-=(const Offset3D& other);
	Point3D& operator=(const Point3D& other) { data = other.data; return *this; }
	[[nodiscard]] auto get() const { return data; }
	[[nodiscard]] Distance x() const { return Distance::create(data[0]); }
	[[nodiscard]] Distance y() const { return Distance::create(data[1]); }
	[[nodiscard]] Distance z() const { return Distance::create(data[2]); }
	[[nodiscard]] Point3D operator-(const Distance& distance) const;
	[[nodiscard]] Point3D operator+(const Distance& distance) const;
	[[nodiscard]] Point3D operator-(const DistanceWidth& distance) const;
	[[nodiscard]] Point3D operator+(const DistanceWidth& distance) const;
	[[nodiscard]] Point3D operator+(const Offset3D& other) const;
	[[nodiscard]] Point3D operator-(const Offset3D& other) const;
	[[nodiscard]] Point3D operator/(const int& other) const;
	[[nodiscard]] bool exists() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] Point3D below() const;
	[[nodiscard]] Point3D north() const;
	[[nodiscard]] Point3D east() const;
	[[nodiscard]] Point3D south() const ;
	[[nodiscard]] Point3D west() const;
	[[nodiscard]] Point3D above() const;
	[[nodiscard]] bool operator==(const Point3D& other) const { return (data == other.data).all();}
	[[nodiscard]] bool operator!=(const Point3D& other) const { return (data != other.data).any();}
	[[nodiscard]] std::strong_ordering operator<=>(const Point3D& other) const;
	[[nodiscard]] Point3D subtractWithMinimum(const Distance& value) const;
	[[nodiscard]] Distance taxiDistanceTo(const Point3D& other) const;
	[[nodiscard]] Distance distanceTo(const Point3D& other) const;
	[[nodiscard]] DistanceFractional distanceToFractional(const Point3D& other) const;
	[[nodiscard]] Distance distanceToSquared(const Point3D& other) const;
	[[nodiscard]] std::string toString() const;
	static const int hilbertOrder = 1;
	[[nodiscard]] uint32_t hilbertNumber() const;
	[[nodiscard]] Offset3D toOffset() const;
	[[nodiscard]] Offset3D offsetTo(const Point3D& other) const;
	[[nodiscard]] Offset3D offsetTo(const Offset3D& other) const;
	[[nodiscard]] Offset3D applyOffset(const Offset3D& other) const;
	[[nodiscard]] bool isInFrontOf(const Point3D& coordinates, const Facing4& facing) const;
	// Return value uses east rather then north as 0.
	[[nodiscard]] double degreesFacingTwords(const Point3D& other) const;
	[[nodiscard]] Facing4 getFacingTwords(const Point3D& other) const;
	[[nodiscard]] Facing8 getFacingTwordsIncludingDiagonal(const Point3D& other) const;
	[[nodiscard]] bool isAdjacentTo(const Point3D& point) const;
	[[nodiscard]] bool isDirectlyAdjacentTo(const Point3D& point) const;
	[[nodiscard]] bool squareOfDistanceIsGreaterThen(const Point3D& point, const DistanceFractional& distanceSquared) const;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] bool contains(const Cuboid& cuboid) const;
	[[nodiscard]] OffsetCuboid offsetCuboidRotated( const OffsetCuboid& cuboid, const Facing4& previousFacing, const Facing4& newFacing) const;
	[[nodiscard]] Offset3D offsetRotated( const Offset3D& initalOffset, const Facing4& previousFacing, const Facing4& newFacing) const;
	[[nodiscard]] Offset3D offsetRotated( const Offset3D& initalOffset, const Facing4& facing) const;
	[[nodiscard]] Offset3D translate(const Point3D& previousPivot, const Point3D& nextPivot, const Facing4& previousFacing, const Facing4& nextFacing) const;
	[[nodiscard]] Offset3D moveInDirection(const Facing6& facing, const Distance& distance) const;
	[[nodiscard]] Offset3D atAdjacentIndex(const AdjacentIndex& index) const;
	[[nodiscard]] Cuboid getAllAdjacentIncludingOutOfBounds() const;
	[[nodiscard]] Cuboid boundry() const;
	//TODO: move to helper.
	[[nodiscard]] bool isAdjacentToAny(const auto& points) const
	{
		for(const Point3D& point : points)
			if(isAdjacentTo(point))
				return true;
		return false;
	}
	void log() const;
	[[nodiscard]] static Point3D create(const DistanceWidth& x, const DistanceWidth& y, const DistanceWidth& z);
	[[nodiscard]] static Point3D create(const Offset& x, const Offset& y, const Offset& z);
	[[nodiscard]] static Point3D create(const Offset3D& offset);
	[[nodiscard]] static Point3D create(const Offsets& offset);
	[[nodiscard]] static Point3D create(const Coordinates& offset);
	[[nodiscard]] __attribute__((noinline)) static Point3D createDbg(const DistanceWidth& x, const DistanceWidth& y, const DistanceWidth& z);
	[[nodiscard]] static Point3D null();
	[[nodiscard]] static Point3D max();
	[[nodiscard]] static Point3D min();
	struct Hash {
		size_t operator()(const Point3D& point) const
		{
			const size_t& max = Distance::max().get();
			return (size_t)point.x().get() + ((size_t)point.y().get() * max) + ((size_t)point.z().get() * max * max);
		}
	};
};
inline void to_json(Json& data, const Point3D& point) { data = {point.x(), point.y(), point.z()}; }
inline void from_json(const Json& data, Point3D& point)
{
	data[0].get_to(point.data[0]);
	data[1].get_to(point.data[1]);
	data[2].get_to(point.data[2]);
}
struct Point3D_fractional
{
	Eigen::Array3f data;
	Point3D_fractional() = default;
	static Point3D_fractional create(const Point3D& point) { Point3D_fractional output; output.data = point.data.cast<float>(); return output; }
	[[nodiscard]] int x() const { return data[0]; }
	[[nodiscard]] int y() const { return data[1]; }
	[[nodiscard]] int z() const { return data[2]; }
};
struct Offset3D
{
	using DimensionType = Offset;
	Offsets data;
	Offset3D() : data(Offset::null().get()) { }
	Offset3D(const Offset& x, const Offset& y, const Offset& z) : data(x.get(), y.get(), z.get()) { }
	Offset3D(const int& x, const int& y, const int& z) : data(x, y, z) { }
	Offset3D(const Offsets& offsets) : data(offsets) { }
	Offset3D(const Offset3D& other);
	Offset3D(const Point3D& point);
	Offset3D& operator=(const Offset3D& other);
	Offset3D& operator=(const Point3D& other);
	void operator*=(const Offset other) { data *= other.get(); }
	void operator*=(const Distance& distance) { data *= distance.get(); }
	void operator/=(const Offset other) { data /= other.get(); }
	void operator/=(const Distance& distance) { data /= distance.get(); }
	void operator+=(const Offset other) { data += other.get(); }
	void operator+=(const Distance& distance) { data += distance.get(); }
	void operator+=(const Offset3D& other) { data += other.data; }
	void operator-=(const Offset other) { data -= other.get(); }
	void operator-=(const Distance& distance) { data -= distance.get(); }
	void rotate2D(const Facing4& facing);
	void rotate2D(const Facing4& oldFacing, const Facing4& newFacing);
	void rotate2DInvert(const Facing4& facing);
	void clampHigh(const Offset3D& other);
	void clampLow(const Offset3D& other);
	void clear() { data.fill(Offset::null().get()); }
	void setX(const Offset& x);
	void setY(const Offset& y);
	void setZ(const Offset& z);
	static const int hilbertOrder = 1;
	[[nodiscard]] uint32_t hilbertNumber() const;
	[[nodiscard]] auto get() const { return data; }
	[[nodiscard]] const Offset x() const { return Offset::create(data[0]); }
	[[nodiscard]] const Offset y() const { return Offset::create(data[1]); }
	[[nodiscard]] const Offset z() const { return Offset::create(data[2]); }
	[[nodiscard]] Offset3D operator+(const Offset3D& other) const;
	[[nodiscard]] Offset3D operator-(const Offset3D& other) const;
	[[nodiscard]] Offset3D operator*(const Offset3D& other) const;
	[[nodiscard]] Offset3D operator/(const Offset3D& other) const;
	[[nodiscard]] Offset3D operator+(const int& other) const;
	[[nodiscard]] Offset3D operator-(const int& other) const;
	[[nodiscard]] Offset3D operator*(const int& other) const;
	[[nodiscard]] Offset3D operator/(const int& other) const;
	[[nodiscard]] bool operator==(const Offset3D& other) const { return (data == other.data).all();}
	[[nodiscard]] bool operator!=(const Offset3D& other) const { return (data != other.data).any();}
	[[nodiscard]] std::strong_ordering operator<=>(const Offset3D& other) const;
	[[nodiscard]] bool empty() const { return data[0] == Offset::null().get(); }
	[[nodiscard]] bool exists() const { return !empty(); }
	[[nodiscard]] std::string toString() const;
	[[nodiscard]] static Offset3D create(const OffsetWidth& x, const OffsetWidth& y, const OffsetWidth& z);
	[[nodiscard]] __attribute__((noinline)) static Offset3D createDbg(const OffsetWidth& x, const OffsetWidth& y, const OffsetWidth& z);
	[[nodiscard]] Offset3D below() const;
	[[nodiscard]] Offset3D north() const;
	[[nodiscard]] Offset3D east() const;
	[[nodiscard]] Offset3D south() const ;
	[[nodiscard]] Offset3D west() const;
	[[nodiscard]] Offset3D above() const;
	[[nodiscard]] Offset3D translate(const Point3D& previousPivot, const Point3D& nextPivot, const Facing4& previousFacing, const Facing4& nextFacing) const;
	[[nodiscard]] Offset3D moveInDirection(const Facing6& facing, const Distance& distance) const;
	[[nodiscard]] Offset3D min(const Offset3D& other) const;
	[[nodiscard]] Offset3D max(const Offset3D& other) const;
	[[nodiscard]] static Offset3D null();
	[[nodiscard]] static Offset3D min();
	[[nodiscard]] static Offset3D max();

};
inline void to_json(Json& data, const Offset3D& point) { data = {point.x(), point.y(), point.z()}; }
inline void from_json(const Json& data, Offset3D& point)
{
	data[0].get_to(point.data[0]);
	data[1].get_to(point.data[1]);
	data[2].get_to(point.data[2]);
}

[[nodiscard]] __attribute__((noinline)) Offset3D createOffset(const OffsetWidth& x, const OffsetWidth& y, const OffsetWidth& z);
[[nodiscard]] __attribute__((noinline)) Point3D createPoint(const DistanceWidth& x, const DistanceWidth& y, const DistanceWidth& z);