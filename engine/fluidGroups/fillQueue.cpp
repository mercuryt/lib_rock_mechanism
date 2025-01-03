#include "fillQueue.h"
#include "../blocks/blocks.h"
#include "../fluidGroup.h"
#include "../area.h"
#include "../blocks/blocks.h"
#include "types.h"
void FillQueue::buildFor(FluidGroup& fluidGroup, BlockIndices& members)
{
	auto& blocks = fluidGroup.m_area.getBlocks();
	for(BlockIndex block : members)
	{
		assert(blocks.fluid_contains(block, fluidGroup.m_fluidType));
		for(BlockIndex adjacent : blocks.getDirectlyAdjacent(block))
			if(
				adjacent.exists() && blocks.fluid_canEnterEver(adjacent) && 
				blocks.fluid_canEnterCurrently(adjacent, fluidGroup.m_fluidType) &&
				blocks.fluid_volumeOfTypeContains(adjacent, fluidGroup.m_fluidType) != Config::maxBlockVolume
			)
				maybeAddBlock(adjacent);
	}
}
void FillQueue::initalizeForStep(FluidGroup& fluidGroup)
{
	auto& blocks = fluidGroup.m_area.getBlocks();
	for(FutureFlowBlock& futureFlowBlock : m_queue)
	{
		futureFlowBlock.delta = CollisionVolume::create(0);
		futureFlowBlock.capacity = blocks.fluid_volumeOfTypeCanEnter(futureFlowBlock.block, fluidGroup.m_fluidType);
	}
	std::ranges::sort(m_queue.begin(), m_queue.end(), [&](FutureFlowBlock& a, FutureFlowBlock& b){
		return getPriority(fluidGroup, a) < getPriority(fluidGroup, b);
	});
	m_groupStart = m_queue.begin();
	findGroupEnd(fluidGroup);
	m_futureFull.clear();
	m_futureNoLongerEmpty.clear();
	m_overfull.clear();
	validate(fluidGroup);
}
void FillQueue::recordDelta(FluidGroup& fluidGroup, const CollisionVolume& volume, const CollisionVolume& flowCapacity, const CollisionVolume& flowTillNextStep)
{
	assert((m_groupStart->capacity >= volume));
	assert(volume != 0);
	assert((m_groupStart != m_groupEnd));
	assert(flowTillNextStep.exists());
	validate(fluidGroup);
	auto& blocks = fluidGroup.m_area.getBlocks();
	// Record fluid level changes.
	for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0 && !blocks.fluid_contains(iter->block, fluidGroup.m_fluidType))
			m_futureNoLongerEmpty.maybeAdd(iter->block);
		iter->delta += volume;
		assert(iter->delta <= Config::maxBlockVolume);
		assert(iter->capacity >= volume);
		iter->capacity -= volume;
	}
	// Record full blocks and get next group.
	if(flowCapacity == volume)
	{
		assert((blocks.fluid_volumeOfTypeContains(m_groupStart->block, fluidGroup.m_fluidType) + m_groupStart->delta) <= Config::maxBlockVolume);
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			if((blocks.fluid_volumeOfTypeContains(iter->block, fluidGroup.m_fluidType) + iter->delta) == Config::maxBlockVolume)
				m_futureFull.add(iter->block);
		m_groupStart = m_groupEnd;
		findGroupEnd(fluidGroup);
	} 
	// Expand group for new higher level.
	// TODO: continue from current position rather then reseting to start + 1.
	else if(flowTillNextStep == volume)
		findGroupEnd(fluidGroup);
	validate(fluidGroup);
}
void FillQueue::applyDelta(FluidGroup& fluidGroup)
{
	validate(fluidGroup);
	auto& blocks = fluidGroup.m_area.getBlocks();
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert(
				!blocks.fluid_contains(iter->block, fluidGroup.m_fluidType) || 
				blocks.fluid_getGroup(iter->block, fluidGroup.m_fluidType) != nullptr);
		blocks.fluid_fillInternal(iter->block, iter->delta, fluidGroup);
		/*assert(iter->block->m_hasFluids.m_fluids[m_fluidGroup.m_fluidType].second != &m_fluidGroup ||
			(iter->block->m_hasFluids.m_fluids[m_fluidGroup.m_fluidType].first < Config::maxBlockVolume && !m_futureFull.contains(iter->block)) ||
			(iter->block->m_hasFluids.m_fluids[m_fluidGroup.m_fluidType].first == Config::maxBlockVolume && m_futureFull.contains(iter->block)));
		*/
		if(blocks.fluid_getTotalVolume(iter->block) > Config::maxBlockVolume)
			m_overfull.add(iter->block);
	}
	validate(fluidGroup);
}
CollisionVolume FillQueue::groupLevel(FluidGroup& fluidGroup) const
{
	assert((m_groupStart != m_groupEnd));
	//TODO: calculate this durring find end.
	CollisionVolume highestLevel = CollisionVolume::create(0);
	for(auto it = m_groupStart; it != m_groupEnd; ++it)
	{
		CollisionVolume level = it->delta + fluidGroup.m_area.getBlocks().fluid_volumeOfTypeContains(it->block, fluidGroup.m_fluidType);
		if(level > highestLevel)
			highestLevel = level;

	}
	return highestLevel;
}
uint32_t FillQueue::getPriority(FluidGroup& fluidGroup, FutureFlowBlock& futureFlowBlock) const
{
	if(futureFlowBlock.capacity == 0)
		return UINT32_MAX;
	//TODO: What is happening here?
	return ((fluidGroup.m_area.getBlocks().getZ(futureFlowBlock.block).get() + 1) * Config::maxBlockVolume.get() * 2) - futureFlowBlock.capacity.get();
}
void FillQueue::findGroupEnd(FluidGroup& fluidGroup)
{
	if(m_groupStart == m_queue.end() || m_groupStart->capacity == 0)
	{
		m_groupEnd = m_groupStart;
		return;
	}
	uint32_t priority = getPriority(fluidGroup, *m_groupStart);
	for(m_groupEnd = m_groupStart + 1; m_groupEnd != m_queue.end(); ++m_groupEnd)
	{
		uint32_t otherPriority = getPriority(fluidGroup, *m_groupEnd);
		assert(priority <= otherPriority);
		if(otherPriority != priority)
			break;
	}
	validate(fluidGroup);
}
void FillQueue::validate(FluidGroup& fluidGroup) const
{
	auto& blocks = fluidGroup.m_area.getBlocks();
	assert((m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end()));
	assert((m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end()));
	assert((m_groupEnd == m_queue.end() || m_groupEnd == m_groupStart || blocks.getZ(m_groupStart->block) != blocks.getZ(m_groupEnd->block) || m_groupStart->capacity > m_groupEnd->capacity));
}