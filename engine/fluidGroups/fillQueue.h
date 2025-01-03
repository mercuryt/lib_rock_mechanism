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
	[[nodiscard]] uint32_t getPriority(FluidGroup& fluidGroup, FutureFlowBlock& futureFlowBlock) const;
public:
	void buildFor(FluidGroup& fluidGroup, BlockIndices& members);
	void initalizeForStep(FluidGroup& fluidGroup);
	void recordDelta(FluidGroup& fluidGroup, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta(FluidGroup& fluidGroup);
	[[nodiscard]] CollisionVolume groupLevel(FluidGroup& fluidGroup) const;
	void findGroupEnd(FluidGroup& fluidGroup);
	void validate(FluidGroup& fluidGroup) const;
};