#pragma once
#include "vectorContainers.h"
#include "index.h"
using OccupiedBlocksForHasShape = SmallSet<BlockIndex>;
//inline void to_json(Json& data, OccupiedBlocksForHasShape* const& set) { data = set->toJson(); }
//inline void from_json(const Json& data, OccupiedBlocksForHasShape*& set) { set->fromJson(data); }