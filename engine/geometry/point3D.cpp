#include "../config.h"
#include "cuboid.h"
Point3D Point3D::operator-(const Distance& distance) const
{
	return (*this) - distance.get();
}
Point3D Point3D::operator+(const Distance& distance) const
{
	return (*this) + distance.get();
}
Point3D Point3D::operator-(const DistanceWidth& distance) const
{
	Offsets result = data.cast<int>() - (int)distance;
	auto greaterThenZero = result > 0;
	result *= greaterThenZero.cast<int>();
	return Coordinates(result.cast<DistanceWidth>());
}
Point3D Point3D::operator+(const DistanceWidth& distance) const
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
Point3D Point3D::min(const Point3D& other) const
{
	return Point3D(data.min(other.data));
}
Point3D Point3D::max(const Point3D& other) const
{
	return Point3D(data.max(other.data));
}
void Point3D::clear() { data.fill(Distance::null().get()); }
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
Distance Point3D::taxiDistanceTo(const Point3D& other) const
{
	return Distance::create((data.cast<int>() - other.data.cast<int>()).abs().sum());
}
Distance Point3D::distanceTo(const Point3D& other) const
{
	Distance squared = distanceToSquared(other);
	return Distance::create(pow((double)squared.get(), 0.5));
}
DistanceFractional Point3D::distanceToFractional(const Point3D& other) const
{
	Distance squared = distanceToSquared(other);
	return DistanceFractional::create(pow((double)squared.get(), 0.5));
}
Distance Point3D::distanceToSquared(const Point3D& other) const
{
	return Distance::create((data - other.data).square().sum());
}
std::string Point3D::toString() const
{
	return "(" + std::to_string(x().get()) + "," + std::to_string(y().get()) + "," + std::to_string(z().get()) + ")";
}
Offset3D Point3D::toOffset() const { return *this; }
Offset3D Point3D::offsetTo(const Point3D& other) const
{
	return other.toOffset() - toOffset();
}
Point3D Point3D::applyOffset(const Offset3D& other) const
{
	Offsets copy = data.cast<OffsetWidth>();
	copy += other.data;
	return Point3D(copy);
}
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
			std::unreachable();
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
	data += other.data.cast<DistanceWidth>();
	return *this;
}
Point3D& Point3D::operator-=(const Offset3D& other)
{
	data -= other.data.cast<DistanceWidth>();
	return *this;
}
Point3D Point3D::operator+(const Offset3D& other) const
{
	return Point3D(data + other.data.cast<DistanceWidth>());
}
Point3D Point3D::operator-(const Offset3D& other) const
{
	return Point3D(data - other.data.cast<DistanceWidth>());
}
void Point3D::log() const
{
	std::cout << toString() << std::endl;
}
Point3D Point3D::create(const DistanceWidth& x, const DistanceWidth& y, const DistanceWidth& z)
{
	return Point3D(Distance::create(x), Distance::create(y), Distance::create(z));
}
Point3D Point3D::create(const Offset3D& offset)
{
	assert(offset.x() >= 0);
	assert(offset.y() >= 0);
	assert(offset.z() >= 0);
	return create(offset.x(), offset.y(), offset.z());
}
Point3D Point3D::create(const Coordinates& coordinates)
{
	return Point3D(coordinates);
}
bool Point3D::isAdjacentTo(const Point3D& point) const
{
	return (data >= point.data - 1).all() && (data <= point.data + 1).all() && (data != point.data).all();
}
bool Point3D::squareOfDistanceIsGreaterThen(const Point3D& point, const DistanceFractional& distanceSquared) const
{
	return distanceToSquared(point) > distanceSquared.get();
}
Point3D Point3D::offsetRotated(const Offset3D& initalOffset, const Facing4& previousFacing, const Facing4& newFacing) const
{
	int rotation = (int)newFacing - (int)previousFacing;
	if(rotation < 0)
		rotation += 4;
	return offsetRotated(initalOffset, (Facing4)rotation);
}
Point3D Point3D::offsetRotated(const Offset3D& initalOffset, const Facing4& facing) const
{
	Offset3D rotatedOffset = initalOffset;
	rotatedOffset.rotate2D(facing);
	return applyOffset(rotatedOffset);
}
Point3D Point3D::translate(const Point3D& previousPivot, const Point3D& nextPivot, const Facing4& previousFacing, const Facing4& nextFacing) const
{
	Offset3D destinationOffset = offsetTo(previousPivot);
	return nextPivot.offsetRotated(destinationOffset, previousFacing, nextFacing);
}
Point3D Point3D::moveInDirection(const Facing6& facing, const Distance& distance) const
{
	Point3D output = *this;
	switch(facing)
	{
		case Facing6::East:
			output.data[0] += distance.get();
			break;
		case Facing6::West:
			output.data[0] -= distance.get();
			break;
		case Facing6::South:
			output.data[1] += distance.get();
			break;
		case Facing6::North:
			output.data[1] -= distance.get();
			break;
		case Facing6::Above:
			output.data[2] += distance.get();
			break;
		case Facing6::Below:
			output.data[2] -= distance.get();
			break;
	}
	return output;
}
std::array<Point3D, 26> Point3D::getAllAdjacentIncludingOutOfBounds() const
{
	std::array<Point3D, 26> output;
	Cuboid cuboid = {Point3D(data + 1), Point3D(data - 1)};
	uint i = 0;
	for(const Point3D& point : cuboid)
		if(point != *this)
		{
			output[i] = point;
			++i;
		}
	return output;
}
Point3D Point3D::null() { return {Distance::null(), Distance::null(), Distance::null()}; }
Offset3D::Offset3D(const Offset3D& other) : data(other.data) { }
Offset3D::Offset3D(const Point3D& point) { data = point.data.cast<int>(); }
Offset3D& Offset3D::operator=(const Offset3D& other) { data = other.data; return *this; }
Offset3D& Offset3D::operator=(const Point3D& other) { data = other.data.cast<int>(); return *this; }
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
Offset3D Offset3D::operator+(const int& other) const { return Offsets(data + other); }
Offset3D Offset3D::operator-(const int& other) const { return Offsets(data - other); }
Offset3D Offset3D::operator*(const int& other) const { return Offsets(data * other); }
Offset3D Offset3D::operator/(const int& other) const { return Offsets(data / other); }
std::string Offset3D::toString() const
{
	return "(" + std::to_string(x()) + "," + std::to_string(y()) + "," + std::to_string(z()) + ")";
}
void Offset3D::rotate2DInvert(const Facing4& facing)
{
	switch(facing)
	{
		case Facing4::North:
			break;
		case Facing4::East:
			rotate2D(Facing4::West);
			break;
		case Facing4::South:
			rotate2D(Facing4::South);
			break;
		case Facing4::West:
			rotate2D(Facing4::East);
			break;
		default:
			std::unreachable();
	}
}
void Offset3D::rotate2D(const Facing4& facing)
{
	switch(facing)
	{
		case Facing4::North:
			break;
		case Facing4::East:
			data[1] *= -1;
			std::swap(data[0], data[1]);
			break;
		case Facing4::South:
			data[0] *= -1;
			data[1] *= -1;
			break;
		case Facing4::West:
			data[0] *= -1;
			std::swap(data[0], data[1]);
			break;
		default:
			std::unreachable();
	}
}
void Offset3D::rotate2D(const Facing4& oldFacing, const Facing4& newFacing)
{
	int rotation = (int)newFacing - (int)oldFacing;
	if(rotation < 0)
		rotation += 4;
	rotate2D((Facing4)rotation);
}
void Offset3D::clampHigh(const Offset3D& other)
{
	data = data.max(other.data);
}
void Offset3D::clampLow(const Offset3D& other)
{
	data = data.min(other.data);
}