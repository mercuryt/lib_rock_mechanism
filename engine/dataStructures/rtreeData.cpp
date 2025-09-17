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
template class RTreeData<RTreeDataWrapper<Project*, nullptr>>;
template class RTreeData<RTreeDataWrapper<StockPile*, nullptr>>;
template class RTreeData<ActorIndex, RTreeDataConfigs::canOverlapNoMerge>;
template class RTreeData<ItemIndex, RTreeDataConfigs::canOverlapNoMerge>;
template class RTreeData<FluidData, RTreeDataConfigs::canOverlapAndMerge>;
template class RTreeData<PointFeature, RTreeDataConfigs::canOverlapAndMerge>;