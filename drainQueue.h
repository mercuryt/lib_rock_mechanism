#pragma once

#include <vector>
#include <unordered_set>

#include "fluidGroup.h"
#include "fluidQueue.h"

template<class DerivedBlock>
class DrainQueue : public FluidQueue<DerivedBlock>
{
	uint32_t getPriority(FutureFlowBlock<DerivedBlock>& futureFlowBlock) const;
public:
	std::unordered_set<DerivedBlock*> m_futureEmpty;
	std::unordered_set<DerivedBlock*> m_futureNoLongerFull;
	std::unordered_set<DerivedBlock*> m_futurePotentialNoLongerAdjacent;

	DrainQueue(FluidGroup<DerivedBlock>& fluidGroup);
	void buildFor(std::unordered_set<DerivedBlock*>& members);
	void initalizeForStep();
	void recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep);
	void applyDelta();
	uint32_t groupLevel() const;
	void findGroupEnd();
};
