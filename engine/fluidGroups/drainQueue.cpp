#include "drainQueue.h"
#include "../fluidGroup.h"
#include "../area.h"
#include "../config.h"
#include <algorithm>
DrainQueue::DrainQueue(FluidGroup& fluidGroup) : FluidQueue(fluidGroup) {}
void DrainQueue::buildFor(std::unordered_set<BlockIndex>& members)
{
	m_set = members;
	for(BlockIndex block : members)
		m_queue.emplace_back(block);
}
void DrainQueue::initalizeForStep()
{
	Blocks& blocks = m_fluidGroup.m_area.getBlocks();
	for(FutureFlowBlock& futureFlowBlock : m_queue)
	{
		assert((blocks.fluid_contains(futureFlowBlock.block, m_fluidGroup.m_fluidType)));
		assert(blocks.fluid_getTotalVolume(futureFlowBlock.block) <= Config::maxBlockVolume);
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = blocks.fluid_volumeOfTypeContains(futureFlowBlock.block, m_fluidGroup.m_fluidType);
	}
	std::ranges::sort(m_queue.begin(), m_queue.end(), [&](FutureFlowBlock& a, FutureFlowBlock& b){
		return getPriority(a) > getPriority(b);
	});
	m_groupStart = m_queue.begin();
	findGroupEnd();
	m_futureEmpty.clear();
	m_futureNoLongerFull.clear();
	m_futurePotentialNoLongerAdjacent.clear();
}
void DrainQueue::recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep)
{
	assert(volume != 0);
	assert((m_groupStart != m_groupEnd));
	assert((m_groupStart->capacity >= volume));
	assert((m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end()));
	assert((m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end()));
	Blocks& blocks = m_fluidGroup.m_area.getBlocks();
	// Record no longer full.
	if(blocks.fluid_getTotalVolume(m_groupStart->block) == Config::maxBlockVolume && !m_futureNoLongerFull.contains((m_groupEnd-1)->block))
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			m_futureNoLongerFull.insert(iter->block);
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
			m_futureEmpty.insert(iter->block);
		m_groupStart = m_groupEnd;
		findGroupEnd();
	} 
	// Expand group for new higher level.
	else if(volume == flowTillNextStep)
		findGroupEnd();
}
void DrainQueue::applyDelta()
{
	assert((m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end()));
	assert((m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end()));
	std::unordered_set<BlockIndex> drainedFromAndAdjacent;
	Blocks& blocks = m_fluidGroup.m_area.getBlocks();
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert(blocks.fluid_contains(iter->block, m_fluidGroup.m_fluidType));
		assert(blocks.fluid_volumeOfTypeContains(iter->block, m_fluidGroup.m_fluidType) >= iter->delta);
		assert(blocks.fluid_getTotalVolume(iter->block) >= iter->delta);
		blocks.fluid_drainInternal(iter->block, iter->delta, m_fluidGroup.m_fluidType);
		// Record blocks to set fluid groups unstable.
		drainedFromAndAdjacent.insert(iter->block);
		for(BlockIndex adjacent : blocks.getDirectlyAdjacent(iter->block))
			if(adjacent != BLOCK_INDEX_MAX && blocks.fluid_canEnterEver(adjacent))
				drainedFromAndAdjacent.insert(adjacent);
	}
	// Set fluidGroups unstable.
	// TODO: Would it be better to prevent fluid groups from becoming stable while in contact with another group? Either option seems bad.
	for(BlockIndex block : drainedFromAndAdjacent)
		blocks.fluid_setAllUnstableExcept(block, m_fluidGroup.m_fluidType);
}
uint32_t DrainQueue::groupLevel() const
{
	assert((m_groupStart != m_groupEnd));
	Blocks& blocks = m_fluidGroup.m_area.getBlocks();
	return blocks.fluid_volumeOfTypeContains(m_groupStart->block, m_fluidGroup.m_fluidType) - m_groupStart->delta;
}
uint32_t DrainQueue::getPriority(FutureFlowBlock& futureFlowBlock) const
{
	Blocks& blocks = m_fluidGroup.m_area.getBlocks();
	return blocks.getZ(futureFlowBlock.block) * Config::maxBlockVolume * 2 + futureFlowBlock.capacity;
}
void DrainQueue::findGroupEnd()
{
	if(m_groupStart == m_queue.end())
	{
		m_groupEnd = m_groupStart;
		return;
	}
	uint32_t priority = getPriority(*m_groupStart);
	for(m_groupEnd = m_groupStart + 1; m_groupEnd != m_queue.end(); ++m_groupEnd)
		if(getPriority(*m_groupEnd) != priority)
			break;
}
