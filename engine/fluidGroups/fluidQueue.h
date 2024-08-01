/*
 * A base class for FillQueue and DrainQueue.
 */
#pragma once

#include "types.h"
#include "index.h"
#include <vector>
#include <cstdint>
#include <cassert>

class FluidGroup;
/*
 * This struct holds a block, it's current capacity ( how much delta can increase at most ) and it's current delta.
 * These structs are stored in FluidQueue::m_queue, which is a vector that gets sorted at the begining of read step.
 * Capacity and delta can represent either addition or subtraction, depending on if they belong to FillQueue or DrainQueue.
 */
struct FutureFlowBlock
{
	BlockIndex block;
	CollisionVolume capacity = CollisionVolume::create(0);
	CollisionVolume delta = CollisionVolume::create(0);
	// No need to initalize capacity and delta here, they will be set at the begining of read step.
	FutureFlowBlock(BlockIndex b) : block(b) {}
};

/*
 * This is a shared base class for DrainQueue and FillQueue.
 */
class FluidQueue
{
public:
	std::vector<FutureFlowBlock> m_queue;
	BlockIndices m_set;
	std::vector<FutureFlowBlock>::iterator m_groupStart, m_groupEnd;
	FluidGroup& m_fluidGroup;

	FluidQueue(FluidGroup& fluidGroup);
	void buildFor(BlockIndices& members);
	void initalizeForStep();
	void setBlocks(BlockIndices& blocks);
	void addBlock(BlockIndex block);
	void addBlocks(BlockIndices& blocks);
	void removeBlock(BlockIndex block);
	void removeBlocks(BlockIndices& blocks);
	void findGroupEnd();
	void recordDelta(CollisionVolume volume);
	void applyDelta();
	void merge(FluidQueue& fluidQueue);
	void noChange();
	[[nodiscard]] uint32_t groupSize() const;
	[[nodiscard]] CollisionVolume groupLevel() const;
	[[nodiscard]] CollisionVolume groupCapacityPerBlock() const;
	[[nodiscard]] CollisionVolume groupFlowTillNextStepPerBlock() const;
	[[nodiscard]] bool groupContains(BlockIndex block) const;
};
