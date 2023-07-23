#include "fillQueue.h"
#include "block.h"
#include "fluidGroup.h"
FillQueue::FillQueue(FluidGroup& fluidGroup) : FluidQueue(fluidGroup) {} 
void FillQueue::buildFor(std::unordered_set<Block*>& members)
{
	for(Block* block : members)
	{
		assert(block->m_fluids.contains(&FluidQueue::m_fluidGroup.m_fluidType));
		for(Block* adjacent : block->m_adjacentsVector)
			 if(adjacent->fluidCanEnterEver() && adjacent->fluidTypeCanEnterCurrently(FluidQueue::m_fluidGroup.m_fluidType) &&
				adjacent->m_fluids.at(&FluidQueue::m_fluidGroup.m_fluidType).first != Config::maxBlockVolume
			   )
				addBlock(adjacent);
	}
}
void FillQueue::initalizeForStep()
{
	for(FutureFlowBlock& futureFlowBlock : FluidQueue::m_queue)
	{
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->volumeOfFluidTypeCanEnter(FluidQueue::m_fluidGroup.m_fluidType);
	}
	std::ranges::sort(FluidQueue::m_queue.begin(), FluidQueue::m_queue.end(), [&](FutureFlowBlock& a, FutureFlowBlock& b){
		return getPriority(a) < getPriority(b);
	});
	FluidQueue::m_groupStart = FluidQueue::m_queue.begin();
	findGroupEnd();
	m_futureFull.clear();
	m_futureNoLongerEmpty.clear();
	m_overfull.clear();
	validate();
}
void FillQueue::recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep)
{
	assert((FluidQueue::m_groupStart->capacity >= volume));
	assert(volume != 0);
	assert((FluidQueue::m_groupStart != FluidQueue::m_groupEnd));
	validate();
	// Record fluid level changes.
	for(auto iter = FluidQueue::m_groupStart; iter != FluidQueue::m_groupEnd; ++iter)
	{
		if(iter->delta == 0 && !iter->block->m_fluids.contains(&FluidQueue::m_fluidGroup.m_fluidType))
			m_futureNoLongerEmpty.insert(iter->block);
		iter->delta += volume;
		assert(iter->delta <= Config::maxBlockVolume);
		assert(iter->capacity >= volume);
		iter->capacity -= volume;
	}
	// Record full blocks and get next group.
	if(flowCapacity == volume)
	{
		assert((FluidQueue::m_groupStart->block->volumeOfFluidTypeContains(FluidQueue::m_fluidGroup.m_fluidType) + FluidQueue::m_groupStart->delta) <= Config::maxBlockVolume);
		for(auto iter = FluidQueue::m_groupStart; iter != FluidQueue::m_groupEnd; ++iter)
			if((iter->block->volumeOfFluidTypeContains(FluidQueue::m_fluidGroup.m_fluidType) + iter->delta) == Config::maxBlockVolume)
				m_futureFull.insert(iter->block);
		FluidQueue::m_groupStart = FluidQueue::m_groupEnd;
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
	for(auto iter = FluidQueue::m_queue.begin(); iter != FluidQueue::m_groupEnd; ++iter)
	{
		// TODO: This seems hackey, should try gather instead, also for drain.
		if(iter->delta == 0)
			continue;
		assert(!iter->block->m_fluids.contains(&FluidQueue::m_fluidGroup.m_fluidType) || iter->block->m_fluids.at(&FluidQueue::m_fluidGroup.m_fluidType).second != nullptr);
		auto found = iter->block->m_fluids.find(&FluidQueue::m_fluidGroup.m_fluidType);
		if(found == iter->block->m_fluids.end())
		{
			FluidGroup& fluidGroup = FluidQueue::m_fluidGroup;
			iter->block->m_fluids.emplace(&fluidGroup.m_fluidType, std::make_pair(iter->delta, &fluidGroup));
		}
		else
		{
			found->second.first += iter->delta;
			assert((found->second.second->m_fluidType == FluidQueue::m_fluidGroup.m_fluidType));
		}
		iter->block->m_totalFluidVolume += iter->delta;
		/*assert(iter->block->m_fluids.at(FluidQueue::m_fluidGroup.m_fluidType).second != &m_fluidGroup ||
				(iter->block->m_fluids.at(FluidQueue::m_fluidGroup.m_fluidType).first < Config::maxBlockVolume && !m_futureFull.contains(iter->block)) ||
				(iter->block->m_fluids.at(FluidQueue::m_fluidGroup.m_fluidType).first == Config::maxBlockVolume && m_futureFull.contains(iter->block)));
				*/
		if(iter->block->m_totalFluidVolume > Config::maxBlockVolume)
			m_overfull.insert(iter->block);
	}
	validate();
}
uint32_t FillQueue::groupLevel() const
{
	assert((FluidQueue::m_groupStart != FluidQueue::m_groupEnd));
	//TODO: calculate this durring find end.
	uint32_t highestLevel = 0;
	for(auto it = FluidQueue::m_groupStart; it != FluidQueue::m_groupEnd; ++it)
	{
		uint32_t level = it->delta + it->block->volumeOfFluidTypeContains(FluidQueue::m_fluidGroup.m_fluidType);
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
	if(FluidQueue::m_groupStart == FluidQueue::m_queue.end() || FluidQueue::m_groupStart->capacity == 0)
	{
		FluidQueue::m_groupEnd = FluidQueue::m_groupStart;
		return;
	}
	uint32_t priority = getPriority(*FluidQueue::m_groupStart);
	for(FluidQueue::m_groupEnd = FluidQueue::m_groupStart + 1; FluidQueue::m_groupEnd != FluidQueue::m_queue.end(); ++FluidQueue::m_groupEnd)
	{
		uint32_t otherPriority = getPriority(*FluidQueue::m_groupEnd);
		assert(priority <= otherPriority);
		if(otherPriority != priority)
			break;
	}
	validate();
}
void FillQueue::validate() const
{
	assert((FluidQueue::m_groupStart >= FluidQueue::m_queue.begin() && FluidQueue::m_groupStart <= FluidQueue::m_queue.end()));
	assert((FluidQueue::m_groupEnd >= FluidQueue::m_queue.begin() && FluidQueue::m_groupEnd <= FluidQueue::m_queue.end()));
	assert((FluidQueue::m_groupEnd == FluidQueue::m_queue.end() || FluidQueue::m_groupEnd == FluidQueue::m_groupStart || FluidQueue::m_groupStart->block->m_z != FluidQueue::m_groupEnd->block->m_z || FluidQueue::m_groupStart->capacity > FluidQueue::m_groupEnd->capacity));
}
