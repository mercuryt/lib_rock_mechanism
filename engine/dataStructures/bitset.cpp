#include "bitset.hpp"

#include <cstdint>

template struct BitSet<uint8_t, 3>; // For Point feature.
template struct BitSet<uint32_t, 26>; // For enterable.
template struct BitSet<uint64_t, 64>; // For rtree.