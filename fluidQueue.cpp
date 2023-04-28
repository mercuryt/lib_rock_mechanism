#include <unordered_set>
#include <assert.h>

FluidQueue::FluidQueue(FluidGroup& fluidGroup) : m_fluidGroup(fluidGroup) {}
void FluidQueue::setBlocks(std::unordered_set<DerivedBlock*>& blocks)
{
	std::erase_if(m_queue, [&](FutureFlowBlock& futureFlowBlock){ return !blocks.contains(futureFlowBlock.block); });
	for(DerivedBlock* block : blocks)
		if(!m_set.contains(block))
			m_queue.emplace_back(block);
	m_set.swap(blocks);
}
void FluidQueue::addBlock(DerivedBlock* block)
{
	if(m_set.contains(block))
		return;
	m_set.insert(block);
	m_queue.emplace_back(block);
}
void FluidQueue::addBlocks(std::unordered_set<DerivedBlock*>& blocks)
{
	//m_queue.reserve(m_queue.size() + blocks.size());
	for(DerivedBlock* block : blocks)
		if(!m_set.contains(block))
			m_queue.emplace_back(block);
	m_set.insert(blocks.begin(), blocks.end());
}
void FluidQueue::removeBlock(DerivedBlock* block)
{
	m_set.erase(block);
	std::erase_if(m_queue, [&](FutureFlowBlock& futureFlowBlock){ return futureFlowBlock.block == block; });
}
void FluidQueue::removeBlocks(std::unordered_set<DerivedBlock*>& blocks)
{
	std::erase_if(m_set, [&](DerivedBlock* block){ return blocks.contains(block); });
	std::erase_if(m_queue, [&](FutureFlowBlock& futureFlowBlock){ return blocks.contains(futureFlowBlock.block); });
}
void FluidQueue::merge(FluidQueue& fluidQueue)
{
	//m_queue.reserve(m_queue.size() + fluidQueue.m_set.size());
	for(DerivedBlock* block : fluidQueue.m_set)
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
