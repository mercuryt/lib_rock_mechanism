#pragma once

#include <vector>
#include <unordered_set>

#include "fluidQueue.h"
#include "fluidGroup.h"


template<class DerivedBlock>
class FillQueue : public FluidQueue<DerivedBlock>
{
	uint32_t getPriority(FutureFlowBlock<DerivedBlock>& futureFlowBlock) const;
public:
	std::unordered_set<DerivedBlock*> m_futureFull;
	std::unordered_set<DerivedBlock*> m_futureNoLongerEmpty;
	std::unordered_set<DerivedBlock*> m_overfull;

	FillQueue(FluidGroup<DerivedBlock>& fluidGroup);
	void buildFor(std::unordered_set<DerivedBlock*>& members);
	void initalizeForStep();
	void recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep);
	void applyDelta();
	uint32_t groupLevel() const;
	void findGroupEnd();
	void validate() const;
};
