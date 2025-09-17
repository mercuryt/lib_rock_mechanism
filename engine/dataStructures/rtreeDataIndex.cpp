#include "rtreeDataIndex.hpp"
#include "smallSet.h"
#include "../pointFeature.h"
#include "../reservable.h"
#include "../space/space.h"

class Fire;
class Project;

template class RTreeDataIndex<std::unique_ptr<Reservable>, uint16_t, RTreeDataConfigs::noMergeOrOverlap>;
template class RTreeDataIndex<SmallMapStable<MaterialTypeId, Fire>*, uint32_t, RTreeDataConfigs::noMergeOrOverlap>;

template class RTreeData<RTreeDataIndex<std::unique_ptr<Reservable>, uint16_t, RTreeDataConfigs::noMergeOrOverlap>::DataIndex, RTreeDataConfigs::noMergeOrOverlap>;
template class RTreeData<RTreeDataIndex<SmallMapStable<MaterialTypeId, Fire>*, uint32_t, RTreeDataConfigs::noMergeOrOverlap>::DataIndex, RTreeDataConfigs::noMergeOrOverlap>;