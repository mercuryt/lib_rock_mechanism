#include "rtreeDataIndex.hpp"
#include "smallSet.h"
#include "../pointFeature.h"
#include "../reservable.h"
#include "../space/space.h"

class Fire;
class Project;

constexpr static RTreeDataConfig noMerge{.splitAndMerge = false};

template class RTreeDataIndex<std::unique_ptr<Reservable>, uint16_t, noMerge>;
template class RTreeDataIndex<SmallMapStable<MaterialTypeId, Fire>*, uint32_t, noMerge>;
template class RTreeDataIndex<PointFeatureSet, uint32_t>;
template class RTreeDataIndex<SmallMap<ActorIndex, CollisionVolume>, uint32_t>;
template class RTreeDataIndex<SmallMap<ItemIndex, CollisionVolume>, uint32_t>;
template class RTreeDataIndex<SmallSet<ActorIndex>, uint32_t>;
template class RTreeDataIndex<SmallSet<ItemIndex>, uint32_t>;