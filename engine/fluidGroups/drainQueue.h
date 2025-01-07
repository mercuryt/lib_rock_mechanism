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
	[[nodiscard]] uint32_t getPriority(Area& area, FutureFlowBlock& futureFlowBlock) const;
public:
	void buildFor(BlockIndices& members);
	void initalizeForStep(Area& area, FluidGroup& fluidGroup);
	void recordDelta(Area& area, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta(Area& area, FluidGroup& fluidGroup);
	[[nodiscard]] CollisionVolume groupLevel(Area& area, FluidGroup& fluidGroup) const;
	void findGroupEnd(Area& area);
};
