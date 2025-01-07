#pragma once

#include "fluidQueue.h"
#include "types.h"
#include "index.h"

class FluidGroup;

class FillQueue final : public FluidQueue
{
public:
	BlockIndices m_futureFull;
	BlockIndices m_futureNoLongerEmpty;
	BlockIndices m_overfull;
private:
	[[nodiscard]] uint32_t getPriority(Area& area, FutureFlowBlock& futureFlowBlock) const;
public:
	void buildFor(Area& area, FluidGroup& fluidGroup, BlockIndices& members);
	void initalizeForStep(Area& area, FluidGroup& fluidGroup);
	void recordDelta(Area& area, FluidGroup& fluidGroup, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta(Area& area, FluidGroup& fluidGroup);
	[[nodiscard]] CollisionVolume groupLevel(Area& area, FluidGroup& fluidGroup) const;
	void findGroupEnd(Area& area);
	void validate(Area& area) const;
};