#include "strongArray.h"
#include "../config/config.h"
#include "../numericTypes/index.h"
// For RTreeBoolean
template struct StrongArray<RTreeNodeIndex, RTreeArrayIndex, Config::rtreeNodeSize>;