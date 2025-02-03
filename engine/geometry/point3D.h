// One BlockIndex has xyz dimensions of 1 meter by 1 meter by 2 meters.
// This File defines physical types.
// idTypes.h defines id types, which don't ever change.
// index.h defines index types, which do change and must be updated.

#pragma once
#include "../json.h"
#include "../types.h"
#include <compare>
#include <cstdint>
#include <iostream>
#include <string>

struct Vector3D;
class Point3D
{
	DistanceInBlocks _x;
	DistanceInBlocks _y;
	DistanceInBlocks _z;
public:
	Point3D() = default;
	Point3D(const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z) : _x(x), _y(y), _z(z) { }
	Point3D(const Point3D& other) : _x(other._x), _y(other._y), _z(other._z) { }
	Point3D& operator=(const Point3D& other)
	{
		_x = other._x;
		_y = other._y;
		_z = other._z;
		return *this;
	}
	DistanceInBlocks& x() { return _x; }
	DistanceInBlocks& y() { return _y; }
	DistanceInBlocks& z() { return _z; }
	const DistanceInBlocks& x() const { return _x; }
	const DistanceInBlocks& y() const { return _y; }
	const DistanceInBlocks& z() const { return _z; }
	Point3D operator-(const DistanceInBlocks& distance) const
	{
		return {
			x() - distance,
			y() - distance,
			z() - distance
		};
	}
	Point3D operator+(const DistanceInBlocks& distance) const
	{
		return {
			x() + distance,
			y() + distance,
			z() + distance
		};
	}
	void operator+=(const Vector3D& other);
	void clampHigh(const Point3D& other)
	{
		x() = std::min(x(), other.x());
		y() = std::min(y(), other.y());
		z() = std::min(z(), other.z());
	}
	void clampLow(const Point3D& other)
	{
		x() = std::max(x(), other.x());
		y() = std::max(y(), other.y());
		z() = std::max(z(), other.z());
	}
	void clear() { x().clear(); y().clear(); z().clear(); }
	[[nodiscard]] bool exists() const { return z().exists();}
	[[nodiscard]] bool empty() const { return z().empty();}
	[[nodiscard]] Point3D below() const { auto output = *this; --output.z(); return output; }
	[[nodiscard]] Point3D north() const { auto output = *this; --output.y(); return output; }
	[[nodiscard]] Point3D east() const { auto output = *this; ++output.x(); return output; }
	[[nodiscard]] Point3D south() const { auto output = *this; ++output.y(); return output; }
	[[nodiscard]] Point3D west() const { auto output = *this; --output.x(); return output; }
	[[nodiscard]] Point3D above() const { auto output = *this; ++output.z(); return output; }
	[[nodiscard]] bool operator==(const Point3D& other) const
	{
		return x() == other.x() && y() == other.y() && z() == other.z();
	}
	[[nodiscard]] bool operator!=(const Point3D& other) const
	{
		return !(*this == other);
	}
	[[nodiscard]] auto operator<=>(const Point3D& other) const
	{
		if (x() != other.x())
			return x() <=> other.x();
		else if (y() != other.y())
			return y() <=> other.y();
		else
			return z() <=> other.z();
	}
	[[nodiscard]] DistanceInBlocks taxiDistanceTo(const Point3D& other) const
	{
		return DistanceInBlocks::create(
			std::abs((int)x().get() - (int)other.x().get()) +
			std::abs((int)y().get() - (int)other.y().get()) +
			std::abs((int)z().get() - (int)other.z().get())
		);
	}
	[[nodiscard]] DistanceInBlocks distanceTo(const Point3D& other) const
	{
		DistanceInBlocks squared = distanceSquared(other);
		return DistanceInBlocks::create(pow((double)squared.get(), 0.5));
	}
	[[nodiscard]] DistanceInBlocks distanceSquared(const Point3D& other) const
	{
		return DistanceInBlocks::create(
			pow(std::abs((int)x().get() - (int)other.x().get()), 2) +
			pow(std::abs((int)y().get() - (int)other.y().get()), 2) +
			pow(std::abs((int)z().get() - (int)other.z().get()), 2)
		);
	}
	[[nodiscard]] std::wstring toString() const
	{
		return L"(" + std::to_wstring(x().get()) + L"," + std::to_wstring(y().get()) + L"," + std::to_wstring(z().get()) + L")";
	}
	static const int hilbertOrder = 1;
	[[nodiscard]] uint32_t hilbertNumber() const
	{
		int n = hilbertOrder;
		int _x = x().get();
		int _y = y().get();
		int _z = z().get();
		int index = 0;
		int rx, ry, rz;

		for (int i = n - 1; i >= 0; --i)
		{
			int mask = 1 << i;
			rx = (_x & mask) > 0;
			ry = (_y & mask) > 0;
			rz = (_z & mask) > 0;

			index <<= 3;

			if (ry == 0)
			{
				if (rx == 1)
				{
					std::swap(_y, _z);
					std::swap(ry, rz);
				}
				if (rz == 1)
				{
					index |= 1;
					_x ^= mask;
					_z ^= mask;
				}
				if (rx == 1)
				{
					index |= 2;
					_y ^= mask;
					_x ^= mask;
				}
			}
			else
			{
				if (rz == 1)
				{
					index |= 4;
					_x ^= mask;
					_z ^= mask;
				}
				if (rx == 1)
				{
					index |= 5;
					_y ^= mask;
					_x ^= mask;
				}
				if (rz == 0)
				{
					index |= 6;
					std::swap(_y, _z);
					std::swap(ry, rz);
				}
			}
		}
		return index;
	}
	[[nodiscard]] Vector3D toVector3D() const;
	[[nodiscard]] bool isInFrontOf(const Point3D& coordinates, const Facing4& facing) const;
	// Return value uses east rather then north as 0.
	[[nodiscard]] double degreesFacingTwords(const Point3D& other) const;
	[[nodiscard]] Facing4 getFacingTwords(const Point3D& other) const;
	[[nodiscard]] Facing8 getFacingTwordsIncludingDiagonal(const Point3D& other) const;
	void log() const
	{
		std::wcout << toString() << std::endl;
	}
	static Point3D create(int x, int y, int z)
	{
		return Point3D(DistanceInBlocks::create(x), DistanceInBlocks::create(y), DistanceInBlocks::create(z));
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Point3D, _x, _y, _z);
};
struct Point3D_fractional
{
	DistanceInBlocksFractional x;
	DistanceInBlocksFractional y;
	DistanceInBlocksFractional z;
};
struct Vector3D
{
	int32_t x = INT32_MAX;
	int32_t y = INT32_MAX;
	int32_t z = INT32_MAX;
};