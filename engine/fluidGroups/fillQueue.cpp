#include "fillQueue.h"
#include "block.h"
#include "fluidGroup.h"
FillQueue::FillQueue(FluidGroup& fluidGroup) : FluidQueue(fluidGroup) {} 
void FillQueue::buildFor(std::unordered_set<Block*>& members)
{
	for(Block* block : members)
	{
		assert(block->m_hasFluids.m_fluids.contains(&m_fluidGroup.m_fluidType));
		for(Block* adjacent : block->m_adjacents)
			 if(adjacent && adjacent->m_hasFluids.fluidCanEnterEver() && adjacent->m_hasFluids.fluidTypeCanEnterCurrently(m_fluidGroup.m_fluidType) &&
				adjacent->m_hasFluids.m_fluids.at(&m_fluidGroup.m_fluidType).first != Config::maxBlockVolume
			   )
				addBlock(adjacent);
	}
}
void FillQueue::initalizeForStep()
{
	for(FutureFlowBlock& futureFlowBlock : m_queue)
	{
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->m_hasFluids.volumeOfFluidTypeCanEnter(m_fluidGroup.m_fluidType);
	}
	std::ranges::sort(m_queue.begin(), m_queue.end(), [&](FutureFlowBlock& a, FutureFlowBlock& b){
		return getPriority(a) < getPriority(b);
	});
	m_groupStart = m_queue.begin();
	findGroupEnd();
	m_futureFull.clear();
	m_futureNoLongerEmpty.clear();
	m_overfull.clear();
	validate();
}
void FillQueue::recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep)
{
	assert((m_groupStart->capacity >= volume));
	assert(volume != 0);
	assert((m_groupStart != m_groupEnd));
	validate();
	// Record fluid level changes.
	for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0 && !iter->block->m_hasFluids.m_fluids.contains(&m_fluidGroup.m_fluidType))
			m_futureNoLongerEmpty.insert(iter->block);
		iter->delta += volume;
		assert(iter->delta <= Config::maxBlockVolume);
		assert(iter->capacity >= volume);
		iter->capacity -= volume;
	}
	// Record full blocks and get next group.
	if(flowCapacity == volume)
	{
		assert((m_groupStart->block->m_hasFluids.volumeOfFluidTypeContains(m_fluidGroup.m_fluidType) + m_groupStart->delta) <= Config::maxBlockVolume);
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			if((iter->block->m_hasFluids.volumeOfFluidTypeContains(m_fluidGroup.m_fluidType) + iter->delta) == Config::maxBlockVolume)
				m_futureFull.insert(iter->block);
		m_groupStart = m_groupEnd;
		findGroupEnd();
	} 
	// Expand group for new higher level.
	// TODO: continue from current position rather then reseting to start + 1.
	else if(flowTillNextStep == volume)
		findGroupEnd();
	validate();
}
void FillQueue::applyDelta()
{
	validate();
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		// TODO: This seems hackey, should try gather instead, also for drain.
		if(iter->delta == 0)
			continue;
		assert(!iter->block->m_hasFluids.m_fluids.contains(&m_fluidGroup.m_fluidType) || iter->block->m_hasFluids.m_fluids.at(&m_fluidGroup.m_fluidType).second != nullptr);
		auto found = iter->block->m_hasFluids.m_fluids.find(&m_fluidGroup.m_fluidType);
		if(found == iter->block->m_hasFluids.m_fluids.end())
		{
			FluidGroup& fluidGroup = m_fluidGroup;
			iter->block->m_hasFluids.m_fluids.emplace(&fluidGroup.m_fluidType, std::make_pair(iter->delta, &fluidGroup));
		}
		else
		{
			found->second.first += iter->delta;
			assert(&found->second.second->m_fluidType == &m_fluidGroup.m_fluidType);
		}
		iter->block->m_hasFluids.m_totalFluidVolume += iter->delta;
		/*assert(iter->block->m_hasFluids.m_fluids.at(m_fluidGroup.m_fluidType).second != &m_fluidGroup ||
				(iter->block->m_hasFluids.m_fluids.at(m_fluidGroup.m_fluidType).first < Config::maxBlockVolume && !m_futureFull.contains(iter->block)) ||
				(iter->block->m_hasFluids.m_fluids.at(m_fluidGroup.m_fluidType).first == Config::maxBlockVolume && m_futureFull.contains(iter->block)));
				*/
		if(iter->block->m_hasFluids.m_totalFluidVolume > Config::maxBlockVolume)
			m_overfull.insert(iter->block);
	}
	validate();
}
uint32_t FillQueue::groupLevel() const
{
	assert((m_groupStart != m_groupEnd));
	//TODO: calculate this durring find end.
	uint32_t highestLevel = 0;
	for(auto it = m_groupStart; it != m_groupEnd; ++it)
	{
		uint32_t level = it->delta + it->block->m_hasFluids.volumeOfFluidTypeContains(m_fluidGroup.m_fluidType);
		if(level > highestLevel)
			highestLevel = level;

	}
	return highestLevel;
}
uint32_t FillQueue::getPriority(FutureFlowBlock& futureFlowBlock) const
{
	if(futureFlowBlock.capacity == 0)
		return UINT32_MAX;
	return ((futureFlowBlock.block->m_z + 1) * Config::maxBlockVolume * 2) - futureFlowBlock.capacity;
}
void FillQueue::findGroupEnd()
{
	if(m_groupStart == m_queue.end() || m_groupStart->capacity == 0)
	{
		m_groupEnd = m_groupStart;
		return;
	}
	uint32_t priority = getPriority(*m_groupStart);
	for(m_groupEnd = m_groupStart + 1; m_groupEnd != m_queue.end(); ++m_groupEnd)
	{
		uint32_t otherPriority = getPriority(*m_groupEnd);
		assert(priority <= otherPriority);
		if(otherPriority != priority)
			break;
	}
	validate();
}
void FillQueue::validate() const
{
	assert((m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end()));
	assert((m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end()));
	assert((m_groupEnd == m_queue.end() || m_groupEnd == m_groupStart || m_groupStart->block->m_z != m_groupEnd->block->m_z || m_groupStart->capacity > m_groupEnd->capacity));
}