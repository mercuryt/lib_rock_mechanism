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
struct Offset3D;
struct Point3D
{
	Coordinates data;
	using Primitive = Coordinates;
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
	[[nodiscard]] Distance x() const { return Distance::create(data[0]); }
	[[nodiscard]] Distance y() const { return Distance::create(data[1]); }
	[[nodiscard]] Distance z() const { return Distance::create(data[2]); }
	[[nodiscard]] Point3D operator-(const Distance& distance) const;
	[[nodiscard]] Point3D operator+(const Distance& distance) const;
	[[nodiscard]] Point3D operator-(const DistanceWidth& distance) const;
	[[nodiscard]] Point3D operator+(const DistanceWidth& distance) const;
	[[nodiscard]] Point3D operator+(const Offset3D& other) const;
	[[nodiscard]] Point3D operator-(const Offset3D& other) const;
	[[nodiscard]] bool exists() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] Point3D below() const;
	[[nodiscard]] Point3D north() const;
	[[nodiscard]] Point3D east() const;
	[[nodiscard]] Point3D south() const ;
	[[nodiscard]] Point3D west() const;
	[[nodiscard]] Point3D above() const;
	[[nodiscard]] bool operator==(const Point3D& other) const;
	[[nodiscard]] bool operator!=(const Point3D& other) const;
	[[nodiscard]] std::strong_ordering operator<=>(const Point3D& other) const;
	[[nodiscard]] Distance taxiDistanceTo(const Point3D& other) const;
	[[nodiscard]] Distance distanceTo(const Point3D& other) const;
	[[nodiscard]] DistanceFractional distanceToFractional(const Point3D& other) const;
	[[nodiscard]] Distance distanceToSquared(const Point3D& other) const;
	[[nodiscard]] std::string toString() const;
	static const int hilbertOrder = 1;
	[[nodiscard]] uint32_t hilbertNumber() const;
	[[nodiscard]] Offset3D toOffset() const;
	[[nodiscard]] Offset3D offsetTo(const Point3D& other) const;
	[[nodiscard]] Point3D applyOffset(const Offset3D& other) const;
	[[nodiscard]] bool isInFrontOf(const Point3D& coordinates, const Facing4& facing) const;
	// Return value uses east rather then north as 0.
	[[nodiscard]] double degreesFacingTwords(const Point3D& other) const;
	[[nodiscard]] Facing4 getFacingTwords(const Point3D& other) const;
	[[nodiscard]] Facing8 getFacingTwordsIncludingDiagonal(const Point3D& other) const;
	[[nodiscard]] bool isAdjacentTo(const Point3D& point) const;
	[[nodiscard]] bool squareOfDistanceIsGreaterThen(const Point3D& point, const DistanceFractional& distanceSquared) const;
	[[nodiscard]] Point3D offsetRotated( const Offset3D& initalOffset, const Facing4& previousFacing, const Facing4& newFacing) const;
	[[nodiscard]] Point3D offsetRotated( const Offset3D& initalOffset, const Facing4& facing) const;
	[[nodiscard]] Point3D translate(const Point3D& previousPivot, const Point3D& nextPivot, const Facing4& previousFacing, const Facing4& nextFacing) const;
	[[nodiscard]] Point3D moveInDirection(const Facing6& facing, const Distance& distance) const;
	// TODO: make this a view / iterator pair.
	[[nodiscard]] std::array<Point3D, 26> getAllAdjacentIncludingOutOfBounds() const;
	[[nodiscard]] bool isAdjacentToAny(const auto& points) const
	{
		for(const Point3D& point : points)
			if(isAdjacentTo(point))
				return true;
		return false;
	}
	void log() const;
	static Point3D create(const DistanceWidth& x, const DistanceWidth& y, const DistanceWidth& z);
	static Point3D create(const Offset3D& offset);
	static Point3D create(const Coordinates& offset);
	static Point3D null();
	static Point3D max() { return { Distance::max(), Distance::max(), Distance::max()}; }
	struct Hash {
		size_t operator()(const Point3D& point) const
		{
			return (point.x() + (point.y() * Distance::max()) + (point.z() * Distance::max() * Distance::max())).get();
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
using Offsets = Eigen::Array<OffsetWidth, 3, 1>;
struct Offset3D
{
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
	[[nodiscard]] Offset3D operator+(const Offset3D& other) const;
	[[nodiscard]] Offset3D operator-(const Offset3D& other) const;
	[[nodiscard]] Offset3D operator*(const Offset3D& other) const;
	[[nodiscard]] Offset3D operator/(const Offset3D& other) const;
	[[nodiscard]] Offset3D operator+(const int& other) const;
	[[nodiscard]] Offset3D operator-(const int& other) const;
	[[nodiscard]] Offset3D operator*(const int& other) const;
	[[nodiscard]] Offset3D operator/(const int& other) const;
	[[nodiscard]] bool operator==(const Offset3D& other) const { return (data == other.data).all();}
	[[nodiscard]] bool operator!=(const Offset3D& other) const { return !((*this) == other); }
	[[nodiscard]] std::strong_ordering operator<=>(const Offset3D& other) const;
	[[nodiscard]] const int& x() const { return data[0]; }
	[[nodiscard]] const int& y() const { return data[1]; }
	[[nodiscard]] const int& z() const { return data[2]; }
	[[nodiscard]] int& x() { return data[0]; }
	[[nodiscard]] int& y() { return data[1]; }
	[[nodiscard]] int& z() { return data[2]; }
	[[nodiscard]] bool empty() { return data[0] == Offset::null().get(); }
	[[nodiscard]] std::string toString() const;
	static Offset3D create(const OffsetWidth& x, const OffsetWidth& y, const OffsetWidth& z) { return {Offset::create(x), Offset::create(y), Offset::create(z)}; }
};
inline void to_json(Json& data, const Offset3D& point) { data = {point.x(), point.y(), point.z()}; }
inline void from_json(const Json& data, Offset3D& point)
{
	data[0].get_to(point.data[0]);
	data[1].get_to(point.data[1]);
	data[2].get_to(point.data[2]);
}