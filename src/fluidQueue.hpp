#include <unordered_set>
#include <assert.h>
template<class DerivedBlock, class DerivedArea>
FluidQueue<DerivedBlock, DerivedArea>::FluidQueue(FluidGroup<DerivedBlock, DerivedArea>& fluidGroup) : m_fluidGroup(fluidGroup) {}

template<class DerivedBlock, class DerivedArea>
void FluidQueue<DerivedBlock, DerivedArea>::setBlocks(std::unordered_set<DerivedBlock*>& blocks)
{
	std::erase_if(m_queue, [&](FutureFlowBlock<DerivedBlock>& futureFlowBlock){ return !blocks.contains(futureFlowBlock.block); });
	for(DerivedBlock* block : blocks)
		if(!m_set.contains(block))
			m_queue.emplace_back(block);
	m_set.swap(blocks);
}
template<class DerivedBlock, class DerivedArea>
void FluidQueue<DerivedBlock, DerivedArea>::addBlock(DerivedBlock* block)
{
	if(m_set.contains(block))
		return;
	m_set.insert(block);
	m_queue.emplace_back(block);
}
template<class DerivedBlock, class DerivedArea>
void FluidQueue<DerivedBlock, DerivedArea>::addBlocks(std::unordered_set<DerivedBlock*>& blocks)
{
	//m_queue.reserve(m_queue.size() + blocks.size());
	for(DerivedBlock* block : blocks)
		if(!m_set.contains(block))
			m_queue.emplace_back(block);
	m_set.insert(blocks.begin(), blocks.end());
}
template<class DerivedBlock, class DerivedArea>
void FluidQueue<DerivedBlock, DerivedArea>::removeBlock(DerivedBlock* block)
{
	m_set.erase(block);
	std::erase_if(m_queue, [&](FutureFlowBlock<DerivedBlock>& futureFlowBlock){ return futureFlowBlock.block == block; });
}
template<class DerivedBlock, class DerivedArea>
void FluidQueue<DerivedBlock, DerivedArea>::removeBlocks(std::unordered_set<DerivedBlock*>& blocks)
{
	std::erase_if(m_set, [&](DerivedBlock* block){ return blocks.contains(block); });
	std::erase_if(m_queue, [&](FutureFlowBlock<DerivedBlock>& futureFlowBlock){ return blocks.contains(futureFlowBlock.block); });
}
template<class DerivedBlock, class DerivedArea>
void FluidQueue<DerivedBlock, DerivedArea>::merge(FluidQueue<DerivedBlock, DerivedArea>& fluidQueue)
{
	//m_queue.reserve(m_queue.size() + fluidQueue.m_set.size());
	for(DerivedBlock* block : fluidQueue.m_set)
		addBlock(block);
}
template<class DerivedBlock, class DerivedArea>
void FluidQueue<DerivedBlock, DerivedArea>::noChange()
{
	m_groupStart = m_groupEnd = m_queue.begin();
}
template<class DerivedBlock, class DerivedArea>
uint32_t FluidQueue<DerivedBlock, DerivedArea>::groupSize() const
{
	return m_groupEnd - m_groupStart;
}
template<class DerivedBlock, class DerivedArea>
uint32_t FluidQueue<DerivedBlock, DerivedArea>::groupCapacityPerBlock() const
{
	assert(m_groupStart != m_groupEnd);
	return m_groupStart->capacity;
}
template<class DerivedBlock, class DerivedArea>
uint32_t FluidQueue<DerivedBlock, DerivedArea>::groupFlowTillNextStepPerBlock() const
{
	assert(m_groupStart != m_groupEnd);
	if(m_groupEnd == m_queue.end() || m_groupEnd->block->m_z != m_groupStart->block->m_z)
		return UINT32_MAX;
	assert(m_groupEnd->capacity < m_groupStart->capacity);
	return m_groupStart->capacity - m_groupEnd->capacity;
}
template<class DerivedBlock, class DerivedArea>
const FluidType* FluidQueue<DerivedBlock, DerivedArea>::getFluidType() const
{
	return m_fluidGroup.m_fluidType;
}
