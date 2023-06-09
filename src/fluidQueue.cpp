#include <unordered_set>
#include <assert.h>
FluidQueue::FluidQueue(FluidGroup& fluidGroup) : m_fluidGroup(fluidGroup) {}
void FluidQueue::setBlocks(std::unordered_set<Block*>& blocks)
{
	std::erase_if(m_queue, [&](FutureFlowBlock& futureFlowBlock){ return !blocks.contains(futureFlowBlock.block); });
	for(Block* block : blocks)
		if(!m_set.contains(block))
			m_queue.emplace_back(block);
	m_set.swap(blocks);
}
void FluidQueue::addBlock(Block* block)
{
	if(m_set.contains(block))
		return;
	m_set.insert(block);
	m_queue.emplace_back(block);
}
void FluidQueue::addBlocks(std::unordered_set<Block*>& blocks)
{
	//m_queue.reserve(m_queue.size() + blocks.size());
	for(Block* block : blocks)
		if(!m_set.contains(block))
			m_queue.emplace_back(block);
	m_set.insert(blocks.begin(), blocks.end());
}
void FluidQueue::removeBlock(Block* block)
{
	m_set.erase(block);
	std::erase_if(m_queue, [&](FutureFlowBlock& futureFlowBlock){ return futureFlowBlock.block == block; });
}
void FluidQueue::removeBlocks(std::unordered_set<Block*>& blocks)
{
	std::erase_if(m_set, [&](Block* block){ return blocks.contains(block); });
	std::erase_if(m_queue, [&](FutureFlowBlock& futureFlowBlock){ return blocks.contains(futureFlowBlock.block); });
}
void FluidQueue::merge(FluidQueue& fluidQueue)
{
	//m_queue.reserve(m_queue.size() + fluidQueue.m_set.size());
	for(Block* block : fluidQueue.m_set)
		addBlock(block);
}
void FluidQueue::noChange()
{
	m_groupStart = m_groupEnd = m_queue.begin();
}
uint32_t FluidQueue::groupSize() const
{
	return m_groupEnd - m_groupStart;
}
uint32_t FluidQueue::groupCapacityPerBlock() const
{
	assert(m_groupStart != m_groupEnd);
	return m_groupStart->capacity;
}
uint32_t FluidQueue::groupFlowTillNextStepPerBlock() const
{
	assert(m_groupStart != m_groupEnd);
	if(m_groupEnd == m_queue.end() || m_groupEnd->block->m_z != m_groupStart->block->m_z)
		return UINT32_MAX;
	assert(m_groupEnd->capacity < m_groupStart->capacity);
	return m_groupStart->capacity - m_groupEnd->capacity;
}
