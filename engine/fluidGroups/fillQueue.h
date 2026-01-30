#pragma once

#include "fluidQueue.h"
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"

class FluidGroup;

class FillQueue final : public FluidQueue<FillQueue>
{
public:
	CuboidSet m_futureFull;
	CuboidSet m_futureNoLongerEmpty;
	CuboidSet m_overfull;
	[[nodiscard]] static int getPriority(const FutureFlowCuboid& futureFlowPoint);
	[[nodiscard]] static bool compare(const FutureFlowCuboid& a, const FutureFlowCuboid& b);
	FillQueue(FluidAllocator& allocator) : FluidQueue<FillQueue>(allocator) { }
	void initializeForStep(Area& area, FluidGroup& fluidGroup);
	void recordDelta(Area& area, FluidGroup& fluidGroup, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta(Area& area, FluidGroup& fluidGroup);
	[[nodiscard]] CollisionVolume groupLevel(Area& area, FluidGroup& fluidGroup) const;
	void validate() const;
};