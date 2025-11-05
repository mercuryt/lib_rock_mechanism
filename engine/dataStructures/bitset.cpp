#include "bitset.hpp"

#include <cstdint>

template struct BitSet<uint8_t, 3u>; // For Point feature.
template struct BitSet<uint32_t, 26u>; // For enterable.
template struct BitSet<uint64_t, 64u>; // For rtree.