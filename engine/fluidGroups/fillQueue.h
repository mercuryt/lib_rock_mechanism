#pragma once

#include <vector>
#include <unordered_set>

#include "fluidQueue.h"

class FluidGroup;

class FillQueue final : public FluidQueue
{
public:
	std::unordered_set<BlockIndex> m_futureFull;
	std::unordered_set<BlockIndex> m_futureNoLongerEmpty;
	std::unordered_set<BlockIndex> m_overfull;
private:
	[[nodiscard]] uint32_t getPriority(FutureFlowBlock& futureFlowBlock) const;
public:
	FillQueue(FluidGroup& fluidGroup);
	void buildFor(std::unordered_set<BlockIndex>& members);
	void initalizeForStep();
	void recordDelta(CollisionVolume volume, CollisionVolume flowCapacity, CollisionVolume flowTillNextStep);
	void applyDelta();
	[[nodiscard]] CollisionVolume groupLevel() const;
	void findGroupEnd();
	void validate() const;
};
