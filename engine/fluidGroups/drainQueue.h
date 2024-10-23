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
	[[nodiscard]] uint32_t getPriority(FutureFlowBlock& futureFlowBlock) const;
public:
	DrainQueue(FluidGroup& fluidGroup);
	void buildFor(BlockIndices& members);
	void initalizeForStep();
	void recordDelta(const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta();
	[[nodiscard]] CollisionVolume groupLevel() const;
	void findGroupEnd();
};
