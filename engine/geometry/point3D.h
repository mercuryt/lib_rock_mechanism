// One BlockIndex has xyz dimensions of 1 meter by 1 meter by 2 meters.
// This File defines physical types.
// idTypes.h defines id types, which don't ever change.
// index.h defines index types, which do change and must be updated.

#pragma once
#include "../json.h"
#include "../types.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop
#include <compare>
#include <cstdint>
#include <iostream>
#include <string>

using Coordinates = Eigen::Array<DistanceInBlocksWidth, 3, 1>;
struct Offset3D;
struct Point3D
{
	Coordinates data;
	Point3D() { data.fill(DistanceInBlocks::null().get()); }
	Point3D(Coordinates v) : data(v) { }
	Point3D(const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z) : data(x.get(), y.get(), z.get()) { }
	Point3D(const Point3D& other) : data(other.data) { }
	void clampHigh(const Point3D& other);
	void clampLow(const Point3D& other);
	void clear();
	void setX(const DistanceInBlocks& x) { data[0] = x.get(); }
	void setY(const DistanceInBlocks& y) { data[1] = y.get(); }
	void setZ(const DistanceInBlocks& z) { data[2] = z.get(); }
	void swap(Point3D& other) { std::swap(data, other.data); }
	Point3D& operator+=(const Offset3D& other);
	Point3D& operator-=(const Offset3D& other);
	Point3D& operator=(const Point3D& other) { data = other.data; return *this; }
	[[nodiscard]] DistanceInBlocks x() const { return DistanceInBlocks::create(data[0]); }
	[[nodiscard]] DistanceInBlocks y() const { return DistanceInBlocks::create(data[1]); }
	[[nodiscard]] DistanceInBlocks z() const { return DistanceInBlocks::create(data[2]); }
	[[nodiscard]] Point3D operator-(const DistanceInBlocks& distance) const;
	[[nodiscard]] Point3D operator+(const DistanceInBlocks& distance) const;
	[[nodiscard]] Point3D operator-(const DistanceInBlocksWidth& distance) const;
	[[nodiscard]] Point3D operator+(const DistanceInBlocksWidth& distance) const;
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
	[[nodiscard]] DistanceInBlocks taxiDistanceTo(const Point3D& other) const;
	[[nodiscard]] DistanceInBlocks distanceTo(const Point3D& other) const;
	[[nodiscard]] DistanceInBlocksFractional distanceToFractional(const Point3D& other) const;
	[[nodiscard]] DistanceInBlocks distanceSquared(const Point3D& other) const;
	[[nodiscard]] std::string toString() const;
	static const int hilbertOrder = 1;
	[[nodiscard]] uint32_t hilbertNumber() const;
	[[nodiscard]] Offset3D toOffset() const;
	[[nodiscard]] bool isInFrontOf(const Point3D& coordinates, const Facing4& facing) const;
	// Return value uses east rather then north as 0.
	[[nodiscard]] double degreesFacingTwords(const Point3D& other) const;
	[[nodiscard]] Facing4 getFacingTwords(const Point3D& other) const;
	[[nodiscard]] Facing8 getFacingTwordsIncludingDiagonal(const Point3D& other) const;
	void log() const;
	static Point3D create(int x, int y, int z);
	static Point3D create(const Offset3D& offset);
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
	void operator*=(const DistanceInBlocks& distance) { data *= distance.get(); }
	void operator/=(const Offset other) { data /= other.get(); }
	void operator/=(const DistanceInBlocks& distance) { data /= distance.get(); }
	void operator+=(const Offset other) { data += other.get(); }
	void operator+=(const DistanceInBlocks& distance) { data += distance.get(); }
	void operator+=(const Offset3D& other) { data += other.data; }
	void operator-=(const Offset other) { data -= other.get(); }
	void operator-=(const DistanceInBlocks& distance) { data -= distance.get(); }
	void rotate2D(const Facing4& facing);
	void rotate2D(const Facing4& oldFacing, const Facing4& newFacing);
	void rotate2DInvert(const Facing4& facing);
	void clampHigh(const Offset3D& other);
	void clampLow(const Offset3D& other);
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