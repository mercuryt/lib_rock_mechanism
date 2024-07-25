#pragma once
#include "index.h"
#include "types.h"
using DestinationCondition = std::function<bool(BlockIndex, Facing)>;
using AccessCondition = std::function<bool(BlockIndex, Facing)>;
using OpenListPush = std::function<void(BlockIndex)>;
using OpenListPop = std::function<BlockIndex()>;
using OpenListEmpty = std::function<bool()>;
