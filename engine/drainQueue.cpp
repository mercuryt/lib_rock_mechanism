#include "drainQueue.h"
#include "block.h"
#include "fluidGroup.h"
#include "config.h"
#include <algorithm>
DrainQueue::DrainQueue(FluidGroup& fluidGroup) : FluidQueue(fluidGroup) {}
void DrainQueue::buildFor(std::unordered_set<Block*>& members)
{
	m_set = members;
	for(Block* block : members)
		m_queue.emplace_back(block);
}
void DrainQueue::initalizeForStep()
{
	for(FutureFlowBlock& futureFlowBlock : m_queue)
	{
		assert((futureFlowBlock.block->m_hasFluids.m_fluids.contains(&m_fluidGroup.m_fluidType)));
		assert(futureFlowBlock.block->m_hasFluids.m_totalFluidVolume <= Config::maxBlockVolume);
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->m_hasFluids.m_fluids.at(&m_fluidGroup.m_fluidType).first;
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
	// Record no longer full.
	if(m_groupStart->block->m_hasFluids.m_totalFluidVolume == Config::maxBlockVolume && !m_futureNoLongerFull.contains((m_groupEnd-1)->block))
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
	std::unordered_set<Block*> drainedFromAndAdjacent;
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert((iter->block->m_hasFluids.m_fluids.contains(&m_fluidGroup.m_fluidType)));
		assert((iter->block->m_hasFluids.m_fluids.at(&m_fluidGroup.m_fluidType).first >= iter->delta));
		assert(iter->block->m_hasFluids.m_totalFluidVolume >= iter->delta);
		auto found = iter->block->m_hasFluids.m_fluids.find(&m_fluidGroup.m_fluidType);
		found->second.first -= iter->delta;
		iter->block->m_hasFluids.m_totalFluidVolume -= iter->delta;
		if(found->second.first == 0)
			iter->block->m_hasFluids.m_fluids.erase(found);
		// Record blocks to set fluid groups unstable.
		drainedFromAndAdjacent.insert(iter->block);
		for(Block* adjacent : iter->block->m_adjacents)
			if(adjacent && adjacent->m_hasFluids.fluidCanEnterEver())
				drainedFromAndAdjacent.insert(adjacent);
	}
	// Set fluidGroups unstable.
	// TODO: Would it be better to prevent fluid groups from becoming stable while in contact with another group? Either option seems bad.
	for(Block* block : drainedFromAndAdjacent)
		for(auto& [fluidType, pair] : block->m_hasFluids.m_fluids)
			if(fluidType != &m_fluidGroup.m_fluidType)
				pair.second->m_stable = false;
}
uint32_t DrainQueue::groupLevel() const
{
	assert((m_groupStart != m_groupEnd));
	return m_groupStart->block->m_hasFluids.m_fluids.at(&m_fluidGroup.m_fluidType).first - m_groupStart->delta;
}
uint32_t DrainQueue::getPriority(FutureFlowBlock& futureFlowBlock) const
{
	return futureFlowBlock.block->m_z * Config::maxBlockVolume * 2 + futureFlowBlock.capacity;
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
