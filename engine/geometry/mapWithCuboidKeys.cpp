#include "mapWithCuboidKeys.hpp"
#include "../numericTypes/types.h"
#include "../pointFeature.h"

template class MapWithCuboidKeysBase<CollisionVolume, Cuboid, CuboidSet>;
template class MapWithCuboidKeysBase<MaterialTypeId, Cuboid, CuboidSet>;
template class MapWithCuboidKeysBase<PointFeature, Cuboid, CuboidSet>;
template class MapWithCuboidKeysBase<std::pair<FluidTypeId, CollisionVolume>, Cuboid, CuboidSet>;
template class MapWithCuboidKeysBase<std::pair<FluidTypeId, CollisionVolume>, OffsetCuboid, OffsetCuboidSet>;
template class MapWithCuboidKeysBase<CollisionVolume, OffsetCuboid, OffsetCuboidSet>;
template class MapWithCuboidKeysBase<PointFeature, OffsetCuboid, OffsetCuboidSet>;
template class MapWithCuboidKeysBase<MaterialTypeId, OffsetCuboid, OffsetCuboidSet>;

template class MapWithCuboidKeys<CollisionVolume>;
template class MapWithCuboidKeys<MaterialTypeId>;
template class MapWithCuboidKeys<PointFeature>;
template class MapWithCuboidKeys<std::pair<FluidTypeId, CollisionVolume>>;
template class MapWithOffsetCuboidKeys<CollisionVolume>;
template class MapWithOffsetCuboidKeys<PointFeature>;
template class MapWithOffsetCuboidKeys<MaterialTypeId>;
