/*
 * A base class for FillQueue and DrainQueue.
 */
#pragma once

#include <vector>
#include <unordered_set>

class FluidGroup;
class Block;
/*
 * This struct holds a block, it's current capacity ( how much delta can increase at most ) and it's current delta.
 * These structs are stored in FluidQueue::m_queue, which is a vector that gets sorted at the begining of read step.
 * Capacity and delta can represent either addition or subtraction, depending on if they belong to FillQueue or DrainQueue.
 */
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
class FluidQueue
{
public:
	std::vector<FutureFlowBlock> m_queue;
	std::unordered_set<Block*> m_set;
	std::vector<FutureFlowBlock>::iterator m_groupStart, m_groupEnd;
	FluidGroup& m_fluidGroup;

	FluidQueue(FluidGroup& fluidGroup);
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
	void merge(FluidQueue& fluidQueue);
	void noChange();
	uint32_t groupSize() const;
	uint32_t groupLevel() const;
	uint32_t groupCapacityPerBlock() const;
	uint32_t groupFlowTillNextStepPerBlock() const;
};
