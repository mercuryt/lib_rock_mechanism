#pragma once

#include "numericTypes/index.h"
#include "dataStructures/smallSet.h"
#include "geometry/point3D.h"
#include "space/adjacentOffsets.h"

#include <vector>
#include <array>

class Area;

inline std::mutex nthAdjacentMutex;
inline std::vector<std::vector<Offset3D>> cache;

std::vector<Offset3D> getNthAdjacentOffsets(uint32_t n);