#pragma once
#include "../geometry/point3D.h"
#include "../dataStructures/smallSet.h"

struct PathResult
{
	SmallSet<Point3D> m_path;
	Point3D m_target;
	[[nodiscard]] bool found() const { return m_target.exists(); }
	[[nodiscard]] bool useCurrentLocation() const { return m_path.empty() && found(); }
};