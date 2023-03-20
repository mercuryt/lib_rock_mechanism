FillQueue::FillQueue(FluidGroup* fluidGroup) : FluidQueue(fluidGroup) {} 
void FillQueue::buildFor(std::unordered_set<Block*>& members)
{
	for(Block* block : members)
		for(Block* adjacent : block->m_adjacentsVector)
			 if(adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(m_fluidGroup->m_fluidType) &&
				adjacent->m_fluids[m_fluidGroup->m_fluidType].first != s_maxBlockVolume
			   )
				addBlock(adjacent);
}
void FillQueue::initalizeForStep()
{
	for(FutureFlowBlock& futureFlowBlock : m_queue)
	{
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->volumeOfFluidTypeCanEnter(m_fluidGroup->m_fluidType);
	}
	std::ranges::sort(m_queue.begin(), m_queue.end(), [&](FutureFlowBlock& a, FutureFlowBlock& b){
		return getPriority(a) < getPriority(b);
	});
	m_groupStart = m_queue.begin();
	findGroupEnd();
	m_futureFull.clear();
	m_futureNoLongerEmpty.clear();
}
void FillQueue::recordDelta(uint32_t volume)
{
	assert(volume != 0);
	assert(m_groupStart->capacity >= volume);
	assert(m_groupStart != m_groupEnd);
	assert(m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end());
	assert(m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end());
	// Record new member blocks for group.
	// Check the last member of the group since it's the most recently added.
	if((m_groupEnd - 1)->delta == 0 && !(m_groupEnd - 1)->block->m_fluids.contains(m_fluidGroup->m_fluidType))
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			m_futureNoLongerEmpty.insert(iter->block);
	// Record fluid level changes.
	for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
	{
		iter->delta += volume;
		assert(iter->delta <= s_maxBlockVolume);
		assert(iter->capacity >= volume);
		iter->capacity -= volume;
	}
	// Record full blocks and get next group.
	if(groupCapacityPerBlock() == 0)
	{
		if((m_groupStart->block->volumeOfFluidTypeContains(m_fluidGroup->m_fluidType) + m_groupStart->delta) == s_maxBlockVolume)
			for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
				m_futureFull.insert(iter->block);
		m_groupStart = m_groupEnd;
		findGroupEnd();
	} 
	// Expand group for new higher level.
	else if(volume == groupFlowTillNextStepPerBlock())
		findGroupEnd();
}
void FillQueue::applyDelta()
{
	assert(m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end());
	assert(m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end());
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		// TODO: This seems hackey, should try gather instead, also for drain.
		if(iter->delta == 0)
			continue;
		assert(!(iter->block->m_fluids.contains(m_fluidGroup->m_fluidType) && iter->block->m_fluids[m_fluidGroup->m_fluidType].second == nullptr));
		if(!iter->block->m_fluids.contains(m_fluidGroup->m_fluidType))
			iter->block->m_fluids.emplace(m_fluidGroup->m_fluidType, std::make_pair(iter->delta, m_fluidGroup));
		else
			iter->block->m_fluids[m_fluidGroup->m_fluidType].first += iter->delta;
		iter->block->m_totalFluidVolume += iter->delta;
		if(iter->block->m_totalFluidVolume > s_maxBlockVolume)
			m_overfull.insert(iter->block);
	}
}
uint32_t FillQueue::groupLevel() const
{
	assert(m_groupStart != m_groupEnd);
	uint32_t output = m_groupStart->delta;
	output += m_groupStart->block->volumeOfFluidTypeContains(m_fluidGroup->m_fluidType);
	return output;
}
uint32_t FillQueue::getPriority(FutureFlowBlock& futureFlowBlock) const
{
	if(futureFlowBlock.capacity == 0)
		return UINT32_MAX;
	return (futureFlowBlock.block->m_z + 1) * s_maxBlockVolume * 2 - futureFlowBlock.capacity;
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
		if(getPriority(*m_groupEnd) != priority)
			break;
}
