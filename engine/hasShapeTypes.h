#pragma once
#include "dataStructures/smallSet.h"
#include "geometry/point3D.h"
using OccupiedSpaceForHasShape = SmallSet<Point3D>;
//inline void to_json(Json& data, OccupiedPointsForHasShape* const& set) { data = set->toJson(); }
//inline void from_json(const Json& data, OccupiedPointsForHasShape*& set) { set->fromJson(data); }