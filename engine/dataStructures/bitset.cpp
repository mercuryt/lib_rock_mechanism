#include "bitset.hpp"

#include <cstdint>

template struct BitSet<uint8_t, 3u>; // For PointFeature.
template struct BitSet<uint8_t, 4u>; // For facing in ClosedListWithFacing.
template struct BitSet<uint64_t, 64u>; // For rtree.