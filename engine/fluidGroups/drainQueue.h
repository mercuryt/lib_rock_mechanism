#pragma once

#include "fluidQueue.h"

#include <unordered_set>
#include <cstdint>

class FluidGroup;
struct FutureFlowBlock;

class DrainQueue final : public FluidQueue
{
public:
	std::unordered_set<BlockIndex> m_futureEmpty;
	std::unordered_set<BlockIndex> m_futureNoLongerFull;
	std::unordered_set<BlockIndex> m_futurePotentialNoLongerAdjacent;
private:
	[[nodiscard]] uint32_t getPriority(FutureFlowBlock& futureFlowBlock) const;
public:
	DrainQueue(FluidGroup& fluidGroup);
	void buildFor(std::unordered_set<BlockIndex>& members);
	void initalizeForStep();
	void recordDelta(CollisionVolume volume, CollisionVolume flowCapacity, CollisionVolume flowTillNextStep);
	void applyDelta();
	[[nodiscard]] CollisionVolume groupLevel() const;
	void findGroupEnd();
};
