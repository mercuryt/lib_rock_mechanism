#pragma once
#include "paramaterizedLine.h"
#include "plane.h"

struct LinePath
{
	std::vector<Point3D> m_points;
	[[nodiscard]] static LinePath create(Point3D start, Point3D end);
	void addWayPoint(Point3D newPoint, Point3D nextPoint);
	[[nodiscard]] SmallSet<Point3D> toPoints() const;
	[[nodiscard]] ParamaterizedLine last() const;
};