#include "drainQueue.h"
#include "fluidGroup.h"
#include "../area/area.h"
#include "../config.h"
#include "../blocks/blocks.h"
#include "types.h"
#include <algorithm>
void DrainQueue::buildFor(BlockIndices& members)
{
	m_set = members;
	for(BlockIndex block : members)
		m_queue.emplace_back(block);
}
void DrainQueue::initalizeForStep(Area& area, FluidGroup& fluidGroup)
{
	Blocks& blocks = area.getBlocks();
	for(FutureFlowBlock& futureFlowBlock : m_queue)
	{
		assert((blocks.fluid_contains(futureFlowBlock.block, fluidGroup.m_fluidType)));
		assert(blocks.fluid_getTotalVolume(futureFlowBlock.block) <= Config::maxBlockVolume);
		futureFlowBlock.delta = CollisionVolume::create(0);
		futureFlowBlock.capacity = blocks.fluid_volumeOfTypeContains(futureFlowBlock.block, fluidGroup.m_fluidType);
	}
	std::ranges::sort(m_queue.begin(), m_queue.end(), [&](FutureFlowBlock& a, FutureFlowBlock& b){
		return getPriority(area, a) > getPriority(area, b);
	});
	m_groupStart = m_queue.begin();
	findGroupEnd(area);
	m_futureEmpty.clear();
	m_futureNoLongerFull.clear();
	m_futurePotentialNoLongerAdjacent.clear();
}
void DrainQueue::recordDelta(Area& area, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep)
{
	assert(volume != 0);
	assert((m_groupStart != m_groupEnd));
	assert((m_groupStart->capacity >= volume));
	assert((m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end()));
	assert((m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end()));
	Blocks& blocks = area.getBlocks();
	// Record no longer full.
	if(blocks.fluid_getTotalVolume(m_groupStart->block) == Config::maxBlockVolume && !m_futureNoLongerFull.contains((m_groupEnd-1)->block))
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			//TODO: is this maybe correct? Why would they already be added?
			m_futureNoLongerFull.maybeAdd(iter->block);
	// Record fluid level changes.
	for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
	{
		iter->delta += volume;
		assert(iter->delta <= Config::maxBlockVolume);
		iter->capacity -= volume;
	}
	// Record empty blocks and get next group.
	if(volume == flowCapacity)
	{
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			m_futureEmpty.add(iter->block);
		m_groupStart = m_groupEnd;
		findGroupEnd(area);
	}
	// Expand group for new higher level.
	else if(volume == flowTillNextStep)
		findGroupEnd(area);
}
void DrainQueue::applyDelta(Area& area, FluidGroup& fluidGroup)
{
	assert((m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end()));
	assert((m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end()));
	BlockIndices drainedFromAndAdjacent;
	Blocks& blocks = area.getBlocks();
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert(blocks.fluid_contains(iter->block, fluidGroup.m_fluidType));
		assert(blocks.fluid_volumeOfTypeContains(iter->block, fluidGroup.m_fluidType) >= iter->delta);
		assert(iter->block.exists());
		assert(iter->delta.exists());
		assert(blocks.fluid_getTotalVolume(iter->block) >= iter->delta);
		blocks.fluid_drainInternal(iter->block, iter->delta, fluidGroup.m_fluidType);
		// Record blocks to set fluid groups unstable.
		drainedFromAndAdjacent.maybeAdd(iter->block);
		for(const BlockIndex& adjacent : blocks.getDirectlyAdjacent(iter->block))
			if(blocks.fluid_canEnterEver(adjacent))
				drainedFromAndAdjacent.maybeAdd(adjacent);
	}
	// Set fluidGroups unstable.
	// TODO: Would it be better to prevent fluid groups from becoming stable while in contact with another group? Either option seems bad.
	for(BlockIndex block : drainedFromAndAdjacent)
		blocks.fluid_setAllUnstableExcept(block, fluidGroup.m_fluidType);
}
CollisionVolume DrainQueue::groupLevel(Area& area, FluidGroup& fluidGroup) const
{
	assert((m_groupStart != m_groupEnd));
	Blocks& blocks = area.getBlocks();
	return blocks.fluid_volumeOfTypeContains(m_groupStart->block, fluidGroup.m_fluidType) - m_groupStart->delta;
}
uint32_t DrainQueue::getPriority(Area& area, FutureFlowBlock& futureFlowBlock) const
{
	Blocks& blocks = area.getBlocks();
	//TODO: What is happening here?
	return blocks.getZ(futureFlowBlock.block).get() * Config::maxBlockVolume.get() * 2 + futureFlowBlock.capacity.get();
}
void DrainQueue::findGroupEnd(Area& area)
{
	if(m_groupStart == m_queue.end())
	{
		m_groupEnd = m_groupStart;
		return;
	}
	uint32_t priority = getPriority(area, *m_groupStart);
	for(m_groupEnd = m_groupStart + 1; m_groupEnd != m_queue.end(); ++m_groupEnd)
		if(getPriority(area, *m_groupEnd) != priority)
			break;
}