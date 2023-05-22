#pragma once

#include <vector>
#include <unordered_set>

#include "fluidGroup.h"
#include "fluidQueue.h"

template<class Block, class Area, class FluidType>
class DrainQueue : public FluidQueue<Block, Area, FluidType>
{
	uint32_t getPriority(FutureFlowBlock<Block>& futureFlowBlock) const;
public:
	std::unordered_set<Block*> m_futureEmpty;
	std::unordered_set<Block*> m_futureNoLongerFull;
	std::unordered_set<Block*> m_futurePotentialNoLongerAdjacent;

	DrainQueue(FluidGroup<Block, Area, FluidType>& fluidGroup);
	void buildFor(std::unordered_set<Block*>& members);
	void initalizeForStep();
	void recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep);
	void applyDelta();
	uint32_t groupLevel() const;
	void findGroupEnd();
};
