#include "rtreeData.hpp"
#include "../path/terrainFacade.h"

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