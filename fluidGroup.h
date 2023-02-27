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
	// No need to initalize capacity and delta here, they will be set at the begining of read step.
	FutureFluidBlock(Block* b) : block(b) {}
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
	bool m_absorbed;
	bool m_disolved;
	const FluidType* m_fluidType;
	int32_t m_excessVolume;
	std::unordered_set<FluidGroup*> m_disolvedInThisGroup;

	std::unordered_set<Block*> m_emptyAdjacentsAddedLastTurn;
	std::unordered_set<Block*> m_potentiallyAddToFillQueueFromSyncronusCode;
	std::unordered_set<Block*> m_futureBlocks;
	std::unordered_set<Block*> m_futureRemoveFromFillQueue;
	std::unordered_set<Block*> m_futureAddToFillQueue;
	std::unordered_set<Block*> m_futureRemoveFromDrainQueue;
	std::unordered_set<Block*> m_futureAddToDrainQueue;

	std::unordered_set<Block*> m_futureEmpty;
	std::unordered_set<Block*> m_futureNewlyAdded;

	std::unordered_set<Block*> m_futureFull;
	std::unordered_set<Block*> m_futureNewUnfull;

	std::unordered_set<Block*> m_futureNewEmptyAdjacents;
	std::unordered_set<Block*> m_futureRemoveFromEmptyAdjacents;
	std::unordered_map<FluidGroup*, std::unordered_set<Block*>> m_futurePotentialMerge;
	// For spitting into multiple fluidGroups.
	std::vector<std::unordered_set<Block*>> m_futureGroups;
	// For notifing groups with different fluids of unfull status. Groups with the same fluid are merged instead.
	std::unordered_map<FluidGroup*, std::unordered_set<Block*>> m_futureNotifyPotentialUnfullAdjacent;

	void addBlockInternal(Block* block);
	void removeBlockInternal(Block* block);
	void removeBlocksInternal(std::unordered_set<Block*>& blocks);
	void removeBlockAdjacent(Block* block);
	//uint32_t futureVolumeForFluidType(Block* block) const;
	//uint32_t futureTotalHeight(Block* block) const;
	void fillGroupFindEnd();
	void drainGroupFindEnd();
	void recordFill(uint32_t flowPerBlock, uint32_t flowCapacity, uint32_t flowTillNextStep);
	void recordDrain(uint32_t flowPerBlock, uint32_t flowCapacity, uint32_t flowTillNextStep);
	uint32_t drainPriority(FutureFluidBlock& futureFluidBlock) const;
	uint32_t fillPriority(FutureFluidBlock& futureFluidBlock) const;
	void setUnstable();
public:
	FluidGroup(const FluidType* ft, std::unordered_set<Block*> blocks);
	void addFluid(uint32_t fluidVolume);
	void removeFluid(uint32_t fluidVolume);
	// Not used by readStep because doesn't look at future.
	void addBlock(Block* block, bool checkMerge = true);
	void removeBlock(Block* block);
	void removeBlocks(std::vector<Block*> blocks);
	// Merge with another group of the same time.
	void absorb(FluidGroup* fluidGroup);
	// Try to spawn another group of a different type which has been disolved in this one.
	void disperseDisolved();
	void readStep();
	void mergeStep();
	void writeStep();
	friend class baseArea;
};
