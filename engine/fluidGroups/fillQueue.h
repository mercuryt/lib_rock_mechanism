#pragma once

#include "fluidQueue.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"

class FluidGroup;

class FillQueue final : public FluidQueue
{
public:
	CuboidSet m_futureFull;
	CuboidSet m_futureNoLongerEmpty;
	CuboidSet m_overfull;
private:
	[[nodiscard]] uint32_t getPriority(FutureFlowPoint& futureFlowPoint) const;
public:
	FillQueue(FluidAllocator& allocator) : FluidQueue(allocator) { }
	void initalizeForStep(Area& area, FluidGroup& fluidGroup);
	void recordDelta(Area& area, FluidGroup& fluidGroup, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta(Area& area, FluidGroup& fluidGroup);
	[[nodiscard]] CollisionVolume groupLevel(Area& area, FluidGroup& fluidGroup) const;
	void findGroupEnd();
	void validate() const;
};