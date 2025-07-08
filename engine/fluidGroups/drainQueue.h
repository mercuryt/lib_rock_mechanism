#pragma once

#include "fluidQueue.h"
#include "numericTypes/index.h"

#include <cstdint>

class FluidGroup;
struct FutureFlowPoint;

class DrainQueue final : public FluidQueue
{
public:
	SmallSet<Point3D> m_futureEmpty;
	SmallSet<Point3D> m_futureNoLongerFull;
	SmallSet<Point3D> m_futurePotentialNoLongerAdjacent;
private:
	[[nodiscard]] uint32_t getPriority(FutureFlowPoint& futureFlowPoint) const;
public:
	DrainQueue(FluidAllocator& allocator) : FluidQueue(allocator) { }
	void buildFor(SmallSet<Point3D>& members);
	void initalizeForStep(Area& area, FluidGroup& fluidGroup);
	void recordDelta(Area& area, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta(Area& area, FluidGroup& fluidGroup);
	[[nodiscard]] CollisionVolume groupLevel(Area& area, FluidGroup& fluidGroup) const;
	void findGroupEnd();
};
