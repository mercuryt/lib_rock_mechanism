#pragma once

#include "fluidQueue.h"
#include "index.h"

#include <cstdint>

class FluidGroup;
struct FutureFlowBlock;

class DrainQueue final : public FluidQueue
{
public:
	BlockIndices m_futureEmpty;
	BlockIndices m_futureNoLongerFull;
	BlockIndices m_futurePotentialNoLongerAdjacent;
private:
	[[nodiscard]] uint32_t getPriority(FluidGroup& fluidGroup, FutureFlowBlock& futureFlowBlock) const;
public:
	void buildFor(BlockIndices& members);
	void initalizeForStep(FluidGroup& fluidGroup);
	void recordDelta(FluidGroup& fluidGroup, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta(FluidGroup& fluidGroup);
	[[nodiscard]] CollisionVolume groupLevel(FluidGroup& fluidGroup) const;
	void findGroupEnd(FluidGroup& fluidGroup);
};
