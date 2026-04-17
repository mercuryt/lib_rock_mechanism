#pragma once
#include "paramaterizedLine.h"
#include "plane.h"

struct LinePath
{
	std::vector<Point3D> m_points;
	[[nodiscard]] static LinePath create(Point3D start, Point3D end);
	[[nodiscard]] SmallSet<Point3D> toPoints() const;
	[[nodiscard]] ParamaterizedLine last() const;
};