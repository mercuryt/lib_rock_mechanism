#pragma once

#include "point3D.h"
#include "../dataStructures/smallSet.h"

namespace setOfPointsHelper
{
	[[nodiscard]] DistanceSquared distanceToClosestSquared(const SmallSet<Point3D>& pointSet, const Point3D& point);
};