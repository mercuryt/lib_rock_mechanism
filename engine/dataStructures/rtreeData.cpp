#include "rtreeData.hpp"
#include "../path/terrainFacade.h"
#include "../space/space.h"

template class RTreeData<DeckId>;
template class RTreeData<Distance>;
template class RTreeData<LongRangePathNodeIndex>;
template class RTreeData<Priority>;
template class RTreeData<AdjacentData>;
template class RTreeData<MaterialTypeId>;
template class RTreeData<FluidTypeId>;
template class RTreeData<CollisionVolume>;
template class RTreeData<PlantIndex>;
template class RTreeData<TemperatureDelta>;
template class RTreeData<VisionCuboidId>;
template class RTreeData<FluidData, RTreeDataConfig{.leavesCanOverlap = true}>;
template class RTreeData<RTreeDataWrapper<Project*, nullptr>>;