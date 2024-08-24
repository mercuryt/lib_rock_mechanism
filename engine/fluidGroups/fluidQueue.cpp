#include "fluidQueue.h"
#include "../fluidGroup.h"
#include "../area.h"
#include "../blocks/blocks.h"
#include "types.h"
#include <assert.h>
FluidQueue::FluidQueue(FluidGroup& fluidGroup) : m_fluidGroup(fluidGroup) {}
void FluidQueue::setBlocks(BlockIndices& blocks)
{
	std::erase_if(m_queue, [&](FutureFlowBlock& futureFlowBlock){ return !blocks.contains(futureFlowBlock.block); });
	for(BlockIndex block : blocks)
		if(!m_set.contains(block))
			m_queue.emplace_back(block);
	m_set.swap(blocks);
}
void FluidQueue::maybeAddBlock(BlockIndex block)
{
	if(m_set.contains(block))
		return;
	m_set.add(block);
	m_queue.emplace_back(block);
}
void FluidQueue::maybeAddBlocks(BlockIndices& blocks)
{
	//m_queue.reserve(m_queue.size() + blocks.size());
	for(BlockIndex block : blocks)
		if(!m_set.contains(block))
			m_queue.emplace_back(block);
	m_set.merge(blocks);
}
void FluidQueue::removeBlock(BlockIndex block)
{
	m_set.remove(block);
	std::erase_if(m_queue, [&](FutureFlowBlock& futureFlowBlock){ return futureFlowBlock.block == block; });
}
void FluidQueue::maybeRemoveBlock(BlockIndex block)
{
	if(m_set.contains(block))
		removeBlock(block);
}
void FluidQueue::removeBlocks(BlockIndices& blocks)
{
	m_set.erase_if([&](BlockIndex block){ return blocks.contains(block); });
	std::erase_if(m_queue, [&](FutureFlowBlock& futureFlowBlock){ return blocks.contains(futureFlowBlock.block); });
}
void FluidQueue::merge(FluidQueue& fluidQueue)
{
	//m_queue.reserve(m_queue.size() + fluidQueue.m_set.size());
	for(BlockIndex block : fluidQueue.m_set)
		maybeAddBlock(block);
}
void FluidQueue::noChange()
{
	m_groupStart = m_groupEnd = m_queue.begin();
}
uint32_t FluidQueue::groupSize() const
{
	return m_groupEnd - m_groupStart;
}
CollisionVolume FluidQueue::groupCapacityPerBlock() const
{
	assert(m_groupStart != m_groupEnd);
	return m_groupStart->capacity;
}
CollisionVolume FluidQueue::groupFlowTillNextStepPerBlock() const
{
	assert(m_groupStart != m_groupEnd);
	auto& blocks = m_fluidGroup.m_area.getBlocks();
	if(m_groupEnd == m_queue.end() || blocks.getZ(m_groupEnd->block) != blocks.getZ(m_groupStart->block))
		return CollisionVolume::null();
	assert(m_groupEnd->capacity < m_groupStart->capacity);
	return m_groupStart->capacity - m_groupEnd->capacity;
}
bool FluidQueue::groupContains(BlockIndex block) const
{
	return std::ranges::find(m_groupStart, m_groupEnd, block, &FutureFlowBlock::block) != m_groupEnd;
}
