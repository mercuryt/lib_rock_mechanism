/*
 * A base class for FillQueue and DrainQueue.
 */
#pragma once

#include <vector>
#include <unordered_set>

#include "fluidType.h"
#include "fluidGroup.h"

template<class DerivedBlock, class DerivedArea> class FluidGroup;
/*
 * This struct holds a block, it's current capacity ( how much delta can increase at most ) and it's current delta.
 * These structs are stored in FluidQueue<DerivedBlock>::m_queue, which is a vector that gets sorted at the begining of read step.
 * Capacity and delta can represent either addition or subtraction, depending on if they belong to FillQueue or DrainQueue.
 */
template<class DerivedBlock>
struct FutureFlowBlock
{
	DerivedBlock* block;
	uint32_t capacity;
	uint32_t delta;
	// No need to initalize capacity and delta here, they will be set at the begining of read step.
	FutureFlowBlock(DerivedBlock* b) : block(b) {}
};

/*
 * This is a shared base class for DrainQueue and FillQueue.
 */
template<class DerivedBlock, class DerivedArea>
class FluidQueue
{
public:
	std::vector<FutureFlowBlock<DerivedBlock>> m_queue;
	std::unordered_set<DerivedBlock*> m_set;
	std::vector<FutureFlowBlock<DerivedBlock>>::iterator m_groupStart, m_groupEnd;
	FluidGroup<DerivedBlock, DerivedArea>& m_fluidGroup;

	FluidQueue(FluidGroup<DerivedBlock, DerivedArea>& fluidGroup);
	void buildFor(std::unordered_set<DerivedBlock*>& members);
	void initalizeForStep();
	void setBlocks(std::unordered_set<DerivedBlock*>& blocks);
	void addBlock(DerivedBlock* block);
	void addBlocks(std::unordered_set<DerivedBlock*>& blocks);
	void removeBlock(DerivedBlock* block);
	void removeBlocks(std::unordered_set<DerivedBlock*>& blocks);
	void findGroupEnd();
	void recordDelta(uint32_t volume);
	void applyDelta();
	void merge(FluidQueue<DerivedBlock, DerivedArea>& fluidQueue);
	void noChange();
	uint32_t groupSize() const;
	uint32_t groupLevel() const;
	uint32_t groupCapacityPerBlock() const;
	uint32_t groupFlowTillNextStepPerBlock() const;
	const FluidType* getFluidType() const;
};
