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

#include "fluidGroups/drainQueue.h"
#include "fluidGroups/fillQueue.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"
#include "dataStructures/smallSet.h"
#include "dataStructures/smallMap.h"

class Area;
struct FluidType;

struct FluidGroupSplitData final
{
	SmallSet<BlockIndex> members;
	SmallSet<BlockIndex> futureAdjacent;
	FluidGroupSplitData(SmallSet<BlockIndex>& m) : members(m) {}
};

class FluidGroup final
{
public:
	FillQueue m_fillQueue;
	DrainQueue m_drainQueue;
	// For spitting into multiple fluidGroups.
	std::vector<FluidGroupSplitData> m_futureGroups;
	// For notifing groups with different fluids of unfull status. Groups with the same fluid are merged instead.
	SmallMap<FluidGroup*, SmallSet<BlockIndex>> m_futureNotifyPotentialUnfullAdjacent;

	SmallSet<BlockIndex> m_potentiallyNoLongerAdjacentFromSyncronusStep;
	SmallSet<BlockIndex> m_potentiallySplitFromSyncronusStep;

	SmallSet<BlockIndex> m_futureNewEmptyAdjacents;

	SmallSet<BlockIndex> m_futureAddToDrainQueue;
	SmallSet<BlockIndex> m_futureRemoveFromDrainQueue;
	SmallSet<BlockIndex> m_futureAddToFillQueue;
	SmallSet<BlockIndex> m_futureRemoveFromFillQueue;
	SmallMap<FluidTypeId, FluidGroup*> m_disolvedInThisGroup;
	FluidTypeId m_fluidType;
	int32_t m_excessVolume = 0;
	uint32_t m_viscosity = 0;
	// Currently at rest?
	bool m_stable = false;
	// Will be destroyed.
	bool m_destroy = false;
	// Has been merged.
	bool m_merged = false;
	bool m_disolved = false;
	bool m_aboveGround = false;

	FluidGroup(FluidAllocator& allocator, const FluidTypeId& ft, SmallSet<BlockIndex>& blocks, Area& area, bool checkMerge = true);
	FluidGroup(const FluidGroup&) = delete;
	void addFluid(Area& area, const CollisionVolume& fluidVolume);
	void removeFluid(Area& area, const CollisionVolume& fluidVolume);
	void addBlock(Area& area, const BlockIndex& block, bool checkMerge = true);
	void removeBlock(Area& area, const BlockIndex& block);
	void addMistFor(Area& area, const BlockIndex& block);
	// Takes a pointer to the other fluid group because we may switch them inorder to merge into the larger one.
	// Return the larger.
	FluidGroup* merge(Area& area, FluidGroup* fluidGroup);
	void readStep(Area& area);
	void writeStep(Area& area);
	void afterWriteStep(Area& area);
	void mergeStep(Area& area);
	void splitStep(Area& area);
	void setUnstable(Area& area);
	void validate(Area& area) const;
	void validate(Area& area, SmallSet<FluidGroup*> toErase) const;
	void log(Area& area) const;
	void logFill(Area& area) const;
	[[nodiscard]] CollisionVolume totalVolume(Area& area) const;
	[[nodiscard]] SmallSet<BlockIndex>& getBlocks() { return m_drainQueue.m_set; }
	[[nodiscard]] bool dispositionIsStable(Area& area, const CollisionVolume& fillVolume, const CollisionVolume& drainVolume) const;
	[[nodiscard]] bool operator==(const FluidGroup& fluidGroup) const { return &fluidGroup == this; }
	[[nodiscard]] Quantity countBlocksOnSurface(const Area& area) const;
	friend class Area;
};
