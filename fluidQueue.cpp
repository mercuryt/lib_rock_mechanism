#include <unordered_set>
#include <assert.h>

FluidQueue::FluidQueue(FluidGroup* fluidGroup) : m_fluidGroup(fluidGroup) {}
void FluidQueue::addBlock(Block* block)
{
	if(m_set.contains(block))
		return;
	m_set.insert(block);
	m_queue.emplace_back(block);
}
void FluidQueue::addBlocks(std::unordered_set<Block*>& blocks)
{
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
	std::erase_if(fluidQueue.m_set, [&](Block* block){ return m_set.contains(block); });
	m_queue.reserve(m_queue.size() + fluidQueue.m_set.size());
	for(Block* block : fluidQueue.m_set)
		m_queue.emplace_back(block);
	m_set.insert(fluidQueue.m_set.begin(), fluidQueue.m_set.end());
}
void FluidQueue::noChange()
{
	m_groupStart = m_groupEnd = m_queue.end();
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
	return m_groupEnd == m_queue.end() || m_groupEnd->block->m_z != m_groupStart->block->m_z ?
		UINT32_MAX :
		m_groupEnd->capacity - m_groupStart->capacity;
}
void FluidQueue::findGroupEnd()
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
