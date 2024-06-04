#pragma once
#include "types.h"
namespace visionUtil
{
	bool hasLineOfSightUsingVisionCuboid(const BlockIndex from, const BlockIndex to);
	bool hasLineOfSightBasic(const BlockIndex from, const BlockIndex to);
}
