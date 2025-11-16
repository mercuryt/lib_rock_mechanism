/*
 * A base class for FillQueue and DrainQueue.
 */
#pragma once

#include "../numericTypes/types.h"
#include "../geometry/cuboidSet.h"
#include "fluidAllocator.h"
#include <vector>
#include <cstdint>
#include <cassert>

class Area;
class FluidGroup;
/*
 * This struct holds a point, it's current capacity ( how much delta can increase at most ) and it's current delta.
 * These structs are stored in FluidQueue::m_queue, which is a vector that gets sorted at the begining of read step.
 * Capacity and delta can represent either addition or subtraction, depending on if they belong to FillQueue or DrainQueue.
 */
struct FutureFlowPoint
{
	Point3D point;
	CollisionVolume capacity = CollisionVolume::create(0);
	CollisionVolume delta = CollisionVolume::create(0);
	// No need to initalize capacity and delta here, they will be set at the begining of read step.
	FutureFlowPoint(const Point3D& b) : point(b) { assert(b.exists()); }
};

/*
 * This is a shared base class for DrainQueue and FillQueue.
 */
class FluidQueue
{
public:
	std::pmr::vector<FutureFlowPoint> m_queue;
	// TODO: does maintaining m_set actually make any sense?
	CuboidSet m_set;
	std::pmr::vector<FutureFlowPoint>::iterator m_groupStart, m_groupEnd;

	FluidQueue(FluidAllocator& allocator) : m_queue(&allocator) { }
	void maybeAddPoint(const Point3D& point);
	void maybeAddPoints(const CuboidSet& points);
	void removePoint(const Point3D& point);
	void maybeRemovePoint(const Point3D& point);
	void maybeRemovePoints(const CuboidSet& points);
	void merge(FluidQueue& fluidQueue);
	void noChange();
	[[nodiscard]] uint32_t groupSize() const;
	[[nodiscard]] CollisionVolume groupLevel() const;
	[[nodiscard]] CollisionVolume groupCapacityPerPoint() const;
	[[nodiscard]] CollisionVolume groupFlowTillNextStepPerPoint() const;
	[[nodiscard]] bool groupContains(const Point3D& point) const;
};
