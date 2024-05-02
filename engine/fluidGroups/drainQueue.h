#pragma once

#include "fluidQueue.h"

#include <unordered_set>
#include <cstdint>

class FluidGroup;
struct FutureFlowBlock;
class Block;

class DrainQueue final : public FluidQueue
{
	uint32_t getPriority(FutureFlowBlock& futureFlowBlock) const;
public:
	std::unordered_set<Block*> m_futureEmpty;
	std::unordered_set<Block*> m_futureNoLongerFull;
	std::unordered_set<Block*> m_futurePotentialNoLongerAdjacent;

	DrainQueue(FluidGroup& fluidGroup);
	void buildFor(std::unordered_set<Block*>& members);
	void initalizeForStep();
	void recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep);
	void applyDelta();
	uint32_t groupLevel() const;
	void findGroupEnd();
};
