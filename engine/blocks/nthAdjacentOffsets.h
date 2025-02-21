#pragma once

#include "index.h"
#include "vectorContainers.h"
#include "geometry/point3D.h"
#include "blocks/adjacentOffsets.h"

#include <vector>
#include <array>

class Area;

inline std::mutex nthAdjacentMutex;
inline SmallSet<Offset3D> closedList;
inline std::vector<std::vector<Offset3D>> cache;
inline std::array<Offset3D, 6> offsets = { Offset3D(0,0,-1), Offset3D(0,0,1), Offset3D(0,-1,0), Offset3D(0,1,0), Offset3D(-1,0,0), Offset3D(1,0,0) };

std::vector<Offset3D> getNthAdjacentOffsets(uint32_t n);