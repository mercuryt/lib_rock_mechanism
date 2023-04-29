#include <unordered_set>
#include <assert.h>
template<class DerivedBlock>
FluidQueue<DerivedBlock>::FluidQueue(FluidGroup<DerivedBlock>& fluidGroup) : m_fluidGroup(fluidGroup) {}

template<class DerivedBlock>
void FluidQueue<DerivedBlock>::setBlocks(std::unordered_set<DerivedBlock*>& blocks)
{
	std::erase_if(m_queue, [&](FutureFlowBlock<DerivedBlock>& futureFlowBlock){ return !blocks.contains(futureFlowBlock.block); });
	for(DerivedBlock* block : blocks)
		if(!m_set.contains(block))
			m_queue.emplace_back(block);
	m_set.swap(blocks);
}
template<class DerivedBlock>
void FluidQueue<DerivedBlock>::addBlock(DerivedBlock* block)
{
	if(m_set.contains(block))
		return;
	m_set.insert(block);
	m_queue.emplace_back(block);
}
template<class DerivedBlock>
void FluidQueue<DerivedBlock>::addBlocks(std::unordered_set<DerivedBlock*>& blocks)
{
	//m_queue.reserve(m_queue.size() + blocks.size());
	for(DerivedBlock* block : blocks)
		if(!m_set.contains(block))
			m_queue.emplace_back(block);
	m_set.insert(blocks.begin(), blocks.end());
}
template<class DerivedBlock>
void FluidQueue<DerivedBlock>::removeBlock(DerivedBlock* block)
{
	m_set.erase(block);
	std::erase_if(m_queue, [&](FutureFlowBlock<DerivedBlock>& futureFlowBlock){ return futureFlowBlock.block == block; });
}
template<class DerivedBlock>
void FluidQueue<DerivedBlock>::removeBlocks(std::unordered_set<DerivedBlock*>& blocks)
{
	std::erase_if(m_set, [&](DerivedBlock* block){ return blocks.contains(block); });
	std::erase_if(m_queue, [&](FutureFlowBlock<DerivedBlock>& futureFlowBlock){ return blocks.contains(futureFlowBlock.block); });
}
template<class DerivedBlock>
void FluidQueue<DerivedBlock>::merge(FluidQueue<DerivedBlock>& fluidQueue)
{
	//m_queue.reserve(m_queue.size() + fluidQueue.m_set.size());
	for(DerivedBlock* block : fluidQueue.m_set)
		addBlock(block);
}
template<class DerivedBlock>
void FluidQueue<DerivedBlock>::noChange()
{
	m_groupStart = m_groupEnd = m_queue.begin();
}
template<class DerivedBlock>
uint32_t FluidQueue<DerivedBlock>::groupSize() const
{
	return m_groupEnd - m_groupStart;
}
template<class DerivedBlock>
uint32_t FluidQueue<DerivedBlock>::groupCapacityPerBlock() const
{
	assert(m_groupStart != m_groupEnd);
	return m_groupStart->capacity;
}
template<class DerivedBlock>
uint32_t FluidQueue<DerivedBlock>::groupFlowTillNextStepPerBlock() const
{
	assert(m_groupStart != m_groupEnd);
	if(m_groupEnd == m_queue.end() || m_groupEnd->block->m_z != m_groupStart->block->m_z)
		return UINT32_MAX;
	assert(m_groupEnd->capacity < m_groupStart->capacity);
	return m_groupStart->capacity - m_groupEnd->capacity;
}
template<class DerivedBlock>
const FluidType* FluidQueue<DerivedBlock>::getFluidType() const
{
	return m_fluidGroup.m_fluidType;
}
