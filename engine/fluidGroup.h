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

#include "drainQueue.h"
#include "fillQueue.h"
#include "fluidType.h"

class Area;
class Block;

struct FluidGroupSplitData final
{
	std::unordered_set<Block*> members;
	std::unordered_set<Block*> futureAdjacent;
	FluidGroupSplitData(std::unordered_set<Block*>& m) : members(m) {}
};

class FluidGroup final
{
public:
	// Currently at rest?
	bool m_stable;
	bool m_destroy;
	bool m_merged;
	bool m_disolved;
	const FluidType& m_fluidType;
	int32_t m_excessVolume;
	uint32_t m_viscosity;
	FillQueue m_fillQueue;
	DrainQueue m_drainQueue;
	std::unordered_map<const FluidType*, FluidGroup*> m_disolvedInThisGroup;
	Area& m_area;

	// For spitting into multiple fluidGroups.
	std::vector<FluidGroupSplitData> m_futureGroups;
	// For notifing groups with different fluids of unfull status. Groups with the same fluid are merged instead.
	std::unordered_map<FluidGroup*, std::unordered_set<Block*>> m_futureNotifyPotentialUnfullAdjacent;
	
	std::unordered_set<Block*> m_diagonalBlocks;

	std::unordered_set<Block*> m_potentiallyNoLongerAdjacentFromSyncronusStep;
	std::unordered_set<Block*> m_potentiallySplitFromSyncronusStep;

	std::unordered_set<Block*> m_futureNewEmptyAdjacents;

	std::unordered_set<Block*> m_futureAddToDrainQueue;
	std::unordered_set<Block*> m_futureRemoveFromDrainQueue;
	std::unordered_set<Block*> m_futureAddToFillQueue;
	std::unordered_set<Block*> m_futureRemoveFromFillQueue;

	FluidGroup(const FluidType& ft, std::unordered_set<Block*>& blocks, Area& area, bool checkMerge = true);
	FluidGroup(const FluidGroup&) = delete;
	void addFluid(uint32_t fluidVolume);
	void removeFluid(uint32_t fluidVolume);
	void addBlock(Block& block, bool checkMerge = true);
	void removeBlock(Block& block);
	void removeBlocks(std::unordered_set<Block*>& blocks);
	void addMistFor(Block& block);
	// Takes a pointer to the other fluid group because we may switch them inorder to merge into the larger one.
	// Return the larger.
	FluidGroup* merge(FluidGroup* fluidGroup);
	void readStep();
	void writeStep();
	void afterWriteStep();
	void mergeStep();
	void splitStep();
	void setUnstable();
	void addDiagonalsFor(Block& block);
	void validate() const;
	void validate(std::unordered_set<FluidGroup*> toErase);
	[[nodiscard]] int32_t totalVolume() const;
	[[nodiscard]] std::unordered_set<Block*>& getBlocks() { return m_drainQueue.m_set; }
	[[nodiscard]] bool dispositionIsStable(CollisionVolume fillVolume, CollisionVolume drainVolume) const;
	[[nodiscard]] bool operator==(const FluidGroup& fluidGroup) const { return &fluidGroup == this; }
	friend class Area;
};
