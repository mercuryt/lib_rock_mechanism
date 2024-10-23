#pragma once
#include "index.h"
#include "types.h"
using DestinationCondition = std::function<std::pair<bool, BlockIndex>(const BlockIndex&, const Facing&)>;
using AccessCondition = std::function<bool(const BlockIndex&, const Facing&)>;
using OpenListPush = std::function<void(const BlockIndex&)>;
using OpenListPop = std::function<BlockIndex()>;
using OpenListEmpty = std::function<bool()>;
using ClosedListAdd = std::function<void(const BlockIndex&, const BlockIndex&)>;
using ClosedListContains = std::function<bool(const BlockIndex&)>;
using ClosedListGetPath = std::function<BlockIndices(const BlockIndex&)>;
