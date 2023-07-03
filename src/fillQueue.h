#pragma once

#include <vector>
#include <unordered_set>

#include "fluidQueue.h"

class FluidGroup;

class FillQueue : public FluidQueue
{
	uint32_t getPriority(FutureFlowBlock<Block>& futureFlowBlock) const;
public:
	std::unordered_set<Block*> m_futureFull;
	std::unordered_set<Block*> m_futureNoLongerEmpty;
	std::unordered_set<Block*> m_overfull;

	FillQueue(FluidGroup& fluidGroup);
	void buildFor(std::unordered_set<Block*>& members);
	void initalizeForStep();
	void recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep);
	void applyDelta();
	uint32_t groupLevel() const;
	void findGroupEnd();
	void validate() const;
};
