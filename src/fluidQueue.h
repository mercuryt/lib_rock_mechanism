/*
 * A base class for FillQueue and DrainQueue.
 */
#pragma once

#include <vector>
#include <unordered_set>

#include "fluidGroup.h"

template<class Block, class Area, class FluidType> class FluidGroup;
/*
 * This struct holds a block, it's current capacity ( how much delta can increase at most ) and it's current delta.
 * These structs are stored in FluidQueue<Block>::m_queue, which is a vector that gets sorted at the begining of read step.
 * Capacity and delta can represent either addition or subtraction, depending on if they belong to FillQueue or DrainQueue.
 */
template<class Block>
struct FutureFlowBlock
{
	Block* block;
	uint32_t capacity;
	uint32_t delta;
	// No need to initalize capacity and delta here, they will be set at the begining of read step.
	FutureFlowBlock(Block* b) : block(b) {}
};

/*
 * This is a shared base class for DrainQueue and FillQueue.
 */
template<class Block, class Area, class FluidType>
class FluidQueue
{
public:
	std::vector<FutureFlowBlock<Block>> m_queue;
	std::unordered_set<Block*> m_set;
	std::vector<FutureFlowBlock<Block>>::iterator m_groupStart, m_groupEnd;
	FluidGroup<Block, Area, FluidType>& m_fluidGroup;

	FluidQueue(FluidGroup<Block, Area, FluidType>& fluidGroup);
	void buildFor(std::unordered_set<Block*>& members);
	void initalizeForStep();
	void setBlocks(std::unordered_set<Block*>& blocks);
	void addBlock(Block* block);
	void addBlocks(std::unordered_set<Block*>& blocks);
	void removeBlock(Block* block);
	void removeBlocks(std::unordered_set<Block*>& blocks);
	void findGroupEnd();
	void recordDelta(uint32_t volume);
	void applyDelta();
	void merge(FluidQueue<Block, Area, FluidType>& fluidQueue);
	void noChange();
	uint32_t groupSize() const;
	uint32_t groupLevel() const;
	uint32_t groupCapacityPerBlock() const;
	uint32_t groupFlowTillNextStepPerBlock() const;
};
