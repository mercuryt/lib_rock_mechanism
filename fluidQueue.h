/*
 * A base class for FillQueue and DrainQueue.
 */
#pragma once

#include <vector>
#include <unordered_set>

#include "fluidType.h"

class Block;
class FLuidGroup;

struct FutureFlowBlock
{
	Block* block;
	uint32_t capacity;
	uint32_t delta;
	// No need to initalize capacity and delta here, they will be set at the begining of read step.
	FutureFlowBlock(Block* b) : block(b) {}
};

class FluidQueue
{
public:
	std::vector<FutureFlowBlock> m_queue;
	std::unordered_set<Block*> m_set;
	std::vector<FutureFlowBlock>::iterator m_groupStart, m_groupEnd;
	FluidGroup* m_fluidGroup;

	virtual uint32_t getPriority(FutureFlowBlock& futureFlowBlock) const = 0;

	FluidQueue(FluidGroup* fluidGroup);
	virtual void buildFor(std::unordered_set<Block*>& members) = 0;
	virtual void initalizeForStep() = 0;
	void addBlock(Block* block);
	void addBlocks(std::unordered_set<Block*>& blocks);
	void removeBlock(Block* block);
	void removeBlocks(std::unordered_set<Block*>& blocks);
	void findGroupEnd();
	virtual void recordDelta(uint32_t volume) = 0;
	virtual void applyDelta() = 0;
	void merge(FluidQueue& fluidQueue);
	void noChange();
	uint32_t groupSize() const;
	virtual uint32_t groupLevel() const = 0;
	uint32_t groupCapacityPerBlock() const;
	uint32_t groupFlowTillNextStepPerBlock() const;
};
