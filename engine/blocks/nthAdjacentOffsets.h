#pragma once

#include "index.h"
#include "vectorContainers.h"
#include "geometry/point3D.h"
#include "blocks/adjacentOffsets.h"

#include <vector>
#include <array>

class Area;

inline std::mutex nthAdjacentMutex;
inline std::vector<std::vector<Offset3D>> cache;

std::vector<Offset3D> getNthAdjacentOffsets(uint32_t n);