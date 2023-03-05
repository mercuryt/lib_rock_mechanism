#pragma once

#include <vector>
#include <unordered_set>

#include "fluidQueue.h"

class Block;
class FluidGroup;

class DrainQueue : public FluidQueue
{
	uint32_t getPriority(FutureFlowBlock& futureFlowBlock) const;
public:
	std::unordered_set<Block*> m_futureEmpty;
	std::unordered_set<Block*> m_futureNoLongerFull;
	std::unordered_set<Block*> m_futurePotentialNoLongerAdjacent;

	DrainQueue(FluidGroup* fluidGroup);
	void buildFor(std::unordered_set<Block*>& members);
	void initalizeForStep();
	void recordDelta(uint32_t volume);
	void applyDelta();
	uint32_t groupLevel() const;
};
