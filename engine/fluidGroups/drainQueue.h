#pragma once

#include "fluidQueue.h"
#include "numericTypes/index.h"

#include <cstdint>

class FluidGroup;
struct FutureFlowBlock;

class DrainQueue final : public FluidQueue
{
public:
	SmallSet<BlockIndex> m_futureEmpty;
	SmallSet<BlockIndex> m_futureNoLongerFull;
	SmallSet<BlockIndex> m_futurePotentialNoLongerAdjacent;
private:
	[[nodiscard]] uint32_t getPriority(Area& area, FutureFlowBlock& futureFlowBlock) const;
public:
	DrainQueue(FluidAllocator& allocator) : FluidQueue(allocator) { }
	void buildFor(SmallSet<BlockIndex>& members);
	void initalizeForStep(Area& area, FluidGroup& fluidGroup);
	void recordDelta(Area& area, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep);
	void applyDelta(Area& area, FluidGroup& fluidGroup);
	[[nodiscard]] CollisionVolume groupLevel(Area& area, FluidGroup& fluidGroup) const;
	void findGroupEnd(Area& area);
};
