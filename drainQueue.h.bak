#pragma once

#include <vector>
#include <unordered_set>

#include "fluidQueue.h"

class FluidGroup;

class DrainQueue : public FluidQueue
{
	uint32_t getPriority(FutureFlowBlock& futureFlowBlock) const;
public:
	std::unordered_set<BLOCK*> m_futureEmpty;
	std::unordered_set<BLOCK*> m_futureNoLongerFull;
	std::unordered_set<BLOCK*> m_futurePotentialNoLongerAdjacent;

	DrainQueue(FluidGroup& fluidGroup);
	void buildFor(std::unordered_set<BLOCK*>& members);
	void initalizeForStep();
	void recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep);
	void applyDelta();
	uint32_t groupLevel() const;
	void findGroupEnd();
};
