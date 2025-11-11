#pragma once

#include "fluidQueue.h"
#include "../numericTypes/index.h"

#include <cstdint>

class FluidGroup;
struct FutureFlowPoint;

class DrainQueue final : public FluidQueue
{
public:
	CuboidSet m_futureEmpty;
	CuboidSet m_futureNoLongerFull;
private:
	[[nodiscard]] uint32_t getPriority(FutureFlowPoint& futureFlowPoint) const;
public:
	DrainQueue(FluidAllocator& allocator) : FluidQueue(allocator) { }
	void initalizeForStep(Area& area, FluidGroup& fluidGroup);
	void recordDelta(Area& area, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta(Area& area, FluidGroup& fluidGroup);
	[[nodiscard]] CollisionVolume groupLevel(Area& area, FluidGroup& fluidGroup) const;
	void findGroupEnd();
};
