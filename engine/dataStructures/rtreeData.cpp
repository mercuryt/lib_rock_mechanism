#include "rtreeData.hpp"
#include "../space/space.h"
#include "../temperature/temperatureSource.h"
#include "../fire.h"
#include "../world/smallRivers.h"

template class RTreeData<Point3D>;
template class RTreeData<Cuboid>;
template class RTreeData<DeckId>;
template class RTreeData<Distance>;
template class RTreeData<DistanceSquared>;
template class RTreeData<Priority>;
template class RTreeData<MaterialTypeId>;
template class RTreeData<FluidTypeId>;
template class RTreeData<CollisionVolume, RTreeDataConfig{}, 0>;
template class RTreeData<PlantIndex>;
template class RTreeData<TemperatureDelta>;
template class RTreeData<TemperatureSource, RTreeDataConfigs::noMerge>;
template class RTreeData<DistanceFractional>;
template class RTreeData<RTreeDataWrapper<Project*, nullptr>>;
template class RTreeData<RTreeDataWrapper<StockPile*, nullptr>>;
template class RTreeData<ActorIndex, RTreeDataConfigs::canOverlapNoMerge>;
template class RTreeData<ItemIndex, RTreeDataConfigs::canOverlapNoMerge>;
template class RTreeData<FluidData, RTreeDataConfigs::canOverlapAndMerge>;
template class RTreeData<PointFeature, RTreeDataConfigs::canOverlapAndMerge>;
template class RTreeData<FireData, RTreeDataConfigs::canOverlapNoMerge>;
template class RTreeData<SmallRiverData>;
template class RTreeData<SettlementId>;
template class RTreeData<AreaId>;
template class RTreeData<BitSet<uint8_t, 8u>>;
template class RTreeData<Quantity>;