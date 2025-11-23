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
struct FutureFlowCuboid
{
	Cuboid cuboid;
	CollisionVolume capacity;
	CollisionVolume delta;
};

/*
 * This is a shared base class for DrainQueue and FillQueue.
 */
template<typename Derived>
class FluidQueue
{
public:
	std::pmr::vector<FutureFlowCuboid> m_queue;
	// TODO: does maintaining m_set actually make any sense?
	CuboidSet m_set;
	std::pmr::vector<FutureFlowCuboid>::iterator m_groupStart, m_groupEnd;

	FluidQueue(FluidAllocator& allocator) : m_queue(&allocator) { }
	void noChange();
	void findGroupEnd();
	[[nodiscard]] uint32_t groupVolume() const;
	[[nodiscard]] CollisionVolume groupLevel() const;
	[[nodiscard]] CollisionVolume groupCapacityPerPoint() const;
	[[nodiscard]] CollisionVolume groupFlowTillNextStepPerPoint() const;
};
