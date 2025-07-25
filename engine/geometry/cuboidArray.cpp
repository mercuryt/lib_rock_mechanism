#include "cuboidArray.hpp"
#include "../dataStructures/rtreeBoolean.h"

// For Rtree.
template class CuboidArray<RTreeBoolean::nodeSize>;
// For finding best merge in Rtree.
template class CuboidArray<RTreeBoolean::nodeSize + 1>;
// For OctTree.
template class CuboidArray<8>;