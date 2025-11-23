/*
 * A set of space which are adjacent to eachother and contain the same fluid.
 * Intented to be run in series in a pool thread.
 * Maintains two priority queues:
 * 	m_pointsByZandTotalFluidHeight is all space which have fluid. It is sorted by high to low. This is the source of flow.
 * 	adjacentAndUnfullPoints is all space which have some fluid but aren't full, as well as space with no fluid that are non-solid and adjacent. It is sorted by low to high. This is the destination of flow.
 * The flow for the step is computed in readStep and applied in writeStep so that readStep can run concurrently with other read only tasks.
 */

#pragma once

#include <vector>

#include "fluidGroups/drainQueue.h"
#include "fluidGroups/fillQueue.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"
#include "dataStructures/smallMap.h"

class Area;
struct FluidType;

struct FluidGroupSplitData final
{
	CuboidSet members;
	CuboidSet futureAdjacent;
	FluidGroupSplitData(const CuboidSet& m) : members(m) {}
};

class FluidGroup final
{
public:
	FillQueue m_fillQueue;
	DrainQueue m_drainQueue;
	// For spitting into multiple fluidGroups.
	std::vector<FluidGroupSplitData> m_futureGroups;

	CuboidSet m_potentiallyNoLongerAdjacentFromSyncronusStep;
	CuboidSet m_potentiallySplitFromSyncronusStep;

	CuboidSet m_futureNewEmptyAdjacents;

	CuboidSet m_futureAddToDrainQueue;
	CuboidSet m_futureRemoveFromDrainQueue;
	CuboidSet m_futureAddToFillQueue;
	CuboidSet m_futureRemoveFromFillQueue;
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

	FluidGroup(FluidAllocator& allocator, const FluidTypeId& ft, const CuboidSet& points, Area& area, bool checkMerge = true);
	FluidGroup(const FluidGroup&) = delete;
	void addFluid(Area& area, const CollisionVolume& fluidVolume);
	void removeFluid(Area& area, const CollisionVolume& fluidVolume);
	void addPoint(Area& area, const Point3D& point, bool checkMerge = true);
	void addPoints(Area& area, const CuboidSet& points, bool checkMerge = true);
	void removePoint(Area& area, const Point3D& point);
	void addMistFor(Area& area, const Point3D& point);
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
	[[nodiscard]] CollisionVolume totalVolume(Area& area) const;
	[[nodiscard]] const CuboidSet& getPoints() const { return m_drainQueue.m_set; }
	[[nodiscard]] bool dispositionIsStable(const CollisionVolume& fillVolume, const CollisionVolume& drainVolume) const;
	[[nodiscard]] bool operator==(const FluidGroup& fluidGroup) const { return &fluidGroup == this; }
	[[nodiscard]] Quantity countPointsOnSurface(const Area& area) const;
	[[nodiscard]] uint countPoints() const;
	friend class Area;
};
