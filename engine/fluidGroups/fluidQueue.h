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
	FutureFlowBlock(const BlockIndex& b) : block(b) { assert(b.exists()); }
};

/*
 * This is a shared base class for DrainQueue and FillQueue.
 */
class FluidQueue
{
public:
	std::vector<FutureFlowBlock> m_queue;
	// TODO: does maintaining m_set actually make any sense?
	BlockIndices m_set;
	std::vector<FutureFlowBlock>::iterator m_groupStart, m_groupEnd;

	void setBlocks(BlockIndices& blocks);
	void maybeAddBlock(const BlockIndex& block);
	void maybeAddBlocks(BlockIndices& blocks);
	void removeBlock(const BlockIndex& block);
	void maybeRemoveBlock(const BlockIndex& block);
	void removeBlocks(BlockIndices& blocks);
	void merge(FluidQueue& fluidQueue);
	void noChange();
	[[nodiscard]] uint32_t groupSize() const;
	[[nodiscard]] CollisionVolume groupLevel() const;
	[[nodiscard]] CollisionVolume groupCapacityPerBlock() const;
	[[nodiscard]] CollisionVolume groupFlowTillNextStepPerBlock(Area& area) const;
	[[nodiscard]] bool groupContains(const BlockIndex& block) const;
};
