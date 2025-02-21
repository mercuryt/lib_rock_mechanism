#pragma once
#include "types.h"
#include "area/area.h"

BlockIndex getIndexForPosition(Area& area, Point3D position);
Point3D getPositionForIndex(Area& area, BlockIndex index);
