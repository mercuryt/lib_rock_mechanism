#include "../config.h"
#include "cuboid.h"
Vector3D Point3D::toVector3D() const { return {(int32_t)x().get(), (int32_t)y().get(), (int32_t)z().get()}; }
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
bool Point3D::isInFrontOf(const Point3D& coordinates, const Facing4& facing) const
{
	switch(facing)
	{
		case Facing4::North:
			return coordinates.y() >= y();
			break;
		case Facing4::East:
			return coordinates.x() <= x();
			break;
		case Facing4::South:
			return coordinates.y() <= y();
			break;
		case Facing4::West:
			return coordinates.x() >= x();
			break;
		default:
			assert(false);
	}
}
double Point3D::degreesFacingTwords(const Point3D& other) const
{
	double x1 = x().get(), y1 = y().get(), x2 = other.x().get(), y2 = other.y().get();
	// Calculate the angle using atan2
	double angleRadians = atan2(y2 - y1, x2 - x1);
	// make negative angles positive by adding 6 radians.
	if (angleRadians < 0 )
		angleRadians += M_PI * 2;
	// Convert to degrees
	return angleRadians * 180.0 / M_PI;
}
void Point3D::operator+=(const Vector3D& other)
{
	x() += other.x;
	y() += other.y;
	z() += other.z;
}