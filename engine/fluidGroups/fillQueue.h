#pragma once

#include "fluidQueue.h"
#include "types.h"
#include "blockIndices.h"

class FluidGroup;

class FillQueue final : public FluidQueue
{
public:
	BlockIndices m_futureFull;
	BlockIndices m_futureNoLongerEmpty;
	BlockIndices m_overfull;
private:
	[[nodiscard]] uint32_t getPriority(FutureFlowBlock& futureFlowBlock) const;
public:
	FillQueue(FluidGroup& fluidGroup);
	void buildFor(BlockIndices& members);
	void initalizeForStep();
	void recordDelta(CollisionVolume volume, CollisionVolume flowCapacity, CollisionVolume flowTillNextStep);
	void applyDelta();
	[[nodiscard]] CollisionVolume groupLevel() const;
	void findGroupEnd();
	void validate() const;
};
