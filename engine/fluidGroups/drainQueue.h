#pragma once

#include "fluidQueue.h"
#include "../numericTypes/index.h"

#include <cstdint>

class FluidGroup;
struct FutureFlowCuboid;

class DrainQueue final : public FluidQueue<DrainQueue>
{
public:
	CuboidSet m_futureEmpty;
	CuboidSet m_futureNoLongerFull;
	[[nodiscard]] static uint32_t getPriority(const FutureFlowCuboid& futureFlowPoint);
	[[nodiscard]] static bool compare(const FutureFlowCuboid& a, const FutureFlowCuboid& b);
	DrainQueue(FluidAllocator& allocator) : FluidQueue<DrainQueue>(allocator) { }
	void initalizeForStep(Area& area, FluidGroup& fluidGroup);
	void recordDelta(Area& area, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta(Area& area, FluidGroup& fluidGroup);
	[[nodiscard]] CollisionVolume groupLevel(Area& area, FluidGroup& fluidGroup) const;
};
