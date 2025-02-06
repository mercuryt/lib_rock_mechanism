#include "../config.h"
#include "cuboid.h"
Point3D Point3D::operator-(const DistanceInBlocks& distance) const
{
	return (*this) - distance.get();
}
Point3D Point3D::operator+(const DistanceInBlocks& distance) const
{
	return (*this) + distance.get();
}
Point3D Point3D::operator-(const DistanceInBlocksWidth& distance) const
{
	Coordinates result = data - distance;
	return result;
}
Point3D Point3D::operator+(const DistanceInBlocksWidth& distance) const
{
	Coordinates result = data + distance;
	return result;
}
void Point3D::clampHigh(const Point3D& other)
{
	data = data.min(other.data);
}
void Point3D::clampLow(const Point3D& other)
{
	data = data.max(other.data);
}
void Point3D::clear() { data.fill(DistanceInBlocks::null().get()); }
bool Point3D::exists() const { return z().exists();}
bool Point3D::empty() const { return z().empty();}
Point3D Point3D::below() const { auto output = *this; --output.data[2]; return output; }
Point3D Point3D::north() const { auto output = *this; --output.data[1]; return output; }
Point3D Point3D::east() const { auto output = *this; ++output.data[0]; return output; }
Point3D Point3D::south() const { auto output = *this; ++output.data[1]; return output; }
Point3D Point3D::west() const { auto output = *this; --output.data[0]; return output; }
Point3D Point3D::above() const { auto output = *this; ++output.data[2]; return output; }
bool Point3D::operator==(const Point3D& other) const
{
	return x() == other.x() && y() == other.y() && z() == other.z();
}
bool Point3D::operator!=(const Point3D& other) const
{
	return !(*this == other);
}
uint32_t Point3D::hilbertNumber() const
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
std::strong_ordering Point3D::operator<=>(const Point3D& other) const
{
	if (x() != other.x())
		return x() <=> other.x();
	else if (y() != other.y())
		return y() <=> other.y();
	else
		return z() <=> other.z();
}
DistanceInBlocks Point3D::taxiDistanceTo(const Point3D& other) const
{
	return DistanceInBlocks::create((data.cast<int>() - other.data.cast<int>()).abs().sum());
}
DistanceInBlocks Point3D::distanceTo(const Point3D& other) const
{
	DistanceInBlocks squared = distanceSquared(other);
	return DistanceInBlocks::create(pow((double)squared.get(), 0.5));
}
DistanceInBlocksFractional Point3D::distanceToFractional(const Point3D& other) const
{
	DistanceInBlocks squared = distanceSquared(other);
	return DistanceInBlocksFractional::create(pow((double)squared.get(), 0.5));
}
DistanceInBlocks Point3D::distanceSquared(const Point3D& other) const
{
	return DistanceInBlocks::create((data - other.data).square().sum());
}
std::wstring Point3D::toString() const
{
	return L"(" + std::to_wstring(x().get()) + L"," + std::to_wstring(y().get()) + L"," + std::to_wstring(z().get()) + L")";
}
Offset3D Point3D::toOffset() const { return *this; }
Facing4 Point3D::getFacingTwords(const Point3D& other) const
{
	double degrees = degreesFacingTwords(other);
	assert(degrees <= 360);
	if(degrees >= 45 && degrees < 135)
		return Facing4::South;
	if(degrees >= 135 && degrees < 225)
		return Facing4::West;
	if(degrees >= 225 && degrees < 315)
		return Facing4::North;
	assert(degrees >= 315 || degrees < 45);
	return Facing4::East;
}
Facing8 Point3D::getFacingTwordsIncludingDiagonal(const Point3D& other) const
{
	double degrees = degreesFacingTwords(other);
	assert(degrees <= 360);
	if(degrees >= 22.5 && degrees < 67.5)
		return Facing8::SouthEast;
	if(degrees >= 67.5 && degrees < 112.5)
		return Facing8::South;
	if(degrees >= 112.5 && degrees < 157.5)
		return Facing8::SouthWest;
	if(degrees >= 157.5 && degrees < 202.5)
		return Facing8::West;
	if(degrees >= 202.5 && degrees < 247.5)
		return Facing8::NorthWest;
	if(degrees >= 247.5 && degrees < 292.5)
		return Facing8::North;
	if(degrees >= 292.5 && degrees < 337.5)
		return Facing8::NorthEast;
	assert(degrees >= 337.5 || degrees < 22.5);
	return Facing8::East;
}
bool Point3D::isInFrontOf(const Point3D& other, const Facing4& facing) const
{
	switch(facing)
	{
		case Facing4::North:
			return other.y() >= y();
			break;
		case Facing4::East:
			return other.x() <= x();
			break;
		case Facing4::South:
			return other.y() <= y();
			break;
		case Facing4::West:
			return other.x() >= x();
			break;
		default:
			assert(false);
	}
}
double Point3D::degreesFacingTwords(const Point3D& other) const
{
	// TODO: SIMD.
	double x1 = x().get(), y1 = y().get(), x2 = other.x().get(), y2 = other.y().get();
	// Calculate the angle using atan2
	double angleRadians = atan2(y2 - y1, x2 - x1);
	// make negative angles positive by adding 6 radians.
	if (angleRadians < 0 )
		angleRadians += M_PI * 2;
	// Convert to degrees
	return angleRadians * 180.0 / M_PI;
}
Point3D& Point3D::operator+=(const Offset3D& other)
{
	data += other.data.cast<DistanceInBlocksWidth>();
	return *this;
}
Point3D& Point3D::operator-=(const Offset3D& other)
{
	data -= other.data.cast<DistanceInBlocksWidth>();
	return *this;
}
Point3D Point3D::operator+(const Offset3D& other) const
{
	return Point3D(data + other.data.cast<DistanceInBlocksWidth>());
}
Point3D Point3D::operator-(const Offset3D& other) const
{
	return Point3D(data - other.data.cast<DistanceInBlocksWidth>());
}
void Point3D::log() const
{
	std::wcout << toString() << std::endl;
}
Point3D Point3D::create(int x, int y, int z)
{
	return Point3D(DistanceInBlocks::create(x), DistanceInBlocks::create(y), DistanceInBlocks::create(z));
}
Offset3D::Offset3D(const Offset3D& other) : data(other.data) { }
Offset3D::Offset3D(const Point3D& point) { data = point.data.cast<int>(); }
Offset3D& Offset3D::operator=(const Offset3D& other) { data = other.data; return *this; }
Offset3D& Offset3D::operator=(const Point3D& other) { data = other.data.cast<int>(); return *this; }
void Offset3D::operator*=(const int& x) { data *= x; }
void Offset3D::operator/=(const int& x) { data /= x; }
void Offset3D::operator+=(const int& x) { data += x; }
void Offset3D::operator-=(const int& x) { data -= x; }
std::strong_ordering Offset3D::operator<=>(const Offset3D& other) const
{
	if (x() != other.x())
		return x() <=> other.x();
	else if (y() != other.y())
		return y() <=> other.y();
	else
		return z() <=> other.z();
}
Offset3D Offset3D::operator+(const Offset3D& other) const { return Offsets(data + other.data); }
Offset3D Offset3D::operator-(const Offset3D& other) const { return Offsets(data - other.data); }
Offset3D Offset3D::operator*(const Offset3D& other) const { return Offsets(data * other.data); }
Offset3D Offset3D::operator/(const Offset3D& other) const { return Offsets(data / other.data); }
std::wstring Offset3D::toString() const
{
	return L"(" + std::to_wstring(x()) + L"," + std::to_wstring(y()) + L"," + std::to_wstring(z()) + L")";
}