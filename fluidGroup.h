/*
 * A set of blocks which are adjacent to eachother and contain the same fluid.
 * Intented to be run in series in a pool thread.
 * Maintains two priority queues:
 * 	m_blocksByZandTotalFluidHeight is all blocks which have fluid. It is sorted by high to low. This is the source of flow.
 * 	adjacentAndUnfullBlocks is all blocks which have some fluid but aren't full, as well as blocks with no fluid that are non-solid and adjacent. It is sorted by low to high. This is the destination of flow.
 * The flow for the step is computed in readStep and applied in writeStep so that readStep can run concurrently with other read only tasks.
 */

#pragma once

#include <vector>
#include <unordered_set>

#include "fluidType.h"

class Block;

struct FutureFluidBlock
{
	Block* block;
	uint32_t capacity;
	int32_t delta;
};
class FluidGroup
{
// public here is for testing, is there a better way?
public:
	// Blocks with contiguous fluid of the same type.
	std::unordered_set<Block*> m_blocks;
	// Blocks sorted by first to drain from, stored with fluid type volume and delta.
	std::vector<FutureFluidBlock> m_drainQueue;
	std::vector<FutureFluidBlock>::iterator m_drainGroupBegin, m_drainGroupEnd;
	// Blocks sorted by first to fill to, stored with fluid type capacity and delta.
	std::vector<FutureFluidBlock> m_fillQueue;
	std::vector<FutureFluidBlock>::iterator m_fillGroupBegin, m_fillGroupEnd;
	// Currently at rest?
	bool m_stable;
	bool m_destroy;
	FluidType* m_fluidType;
	uint32_t m_excessVolume;
	std::unordered_set<Block*> m_futureBlocks;
	std::unordered_set<Block*> m_futureEmpty;
	std::unordered_set<Block*> m_futureFull;
	std::unordered_set<Block*> m_futureUnfull;
	std::unordered_set<Block*> m_futureNewlyAdded;
	std::unordered_set<Block*> m_futureEmptyAdjacents;
	std::unordered_set<Block*> m_futureRemoveFromEmptyAdjacents;
	std::unordered_set<FluidGroup*> m_futureMerge;
	// For spitting into multiple fluidGroups.
	std::vector<std::unordered_set<Block*>> m_futureGroups;
	// For notifing groups with different fluids of unfull status. Groups with the same fluid are merged instead.
	std::unordered_map<FluidGroup*, std::unordered_set<Block*>> m_futureNotifyUnfull;

	void addBlockInternal(Block* block);
	void removeBlockInternal(Block* block);
	void removeBlocksInternal(std::unordered_set<Block*>& blocks);
	//uint32_t futureVolumeForFluidType(Block* block) const;
	//uint32_t futureTotalHeight(Block* block) const;
	void fillGroupFindEnd();
	void drainGroupFindEnd();
	void recordFill(uint32_t flowPerBlock, uint32_t flowMaximum, uint32_t flowCapacity, uint32_t flowTillNextStep);
	void recordDrain(uint32_t flowPerBlock, uint32_t flowMaximum, uint32_t flowCapacity, uint32_t flowTillNextStep);
	uint32_t drainPriority(FutureFluidBlock& futureFluidBlock) const;
	uint32_t fillPriority(FutureFluidBlock& futureFluidBlock) const;
	void setUnstable();
public:
	FluidGroup(FluidType* ft, std::unordered_set<Block*> blocks);
	void addFluid(uint32_t fluidVolume);
	void removeFluid(uint32_t fluidVolume);
	// Not used by readStep because doesn't look at future.
	void addBlock(Block* block, bool checkMerge = true);
	void removeBlock(Block* block);
	void removeBlocks(std::vector<Block*> blocks);
	void absorb(FluidGroup* fluidGroup);
	void readStep();
	void writeStep();
	friend class baseArea;
};
