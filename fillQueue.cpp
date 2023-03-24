FillQueue::FillQueue(FluidGroup& fluidGroup) : FluidQueue(fluidGroup) {} 
void FillQueue::buildFor(std::unordered_set<Block*>& members)
{
	for(Block* block : members)
	{
		assert(block->m_fluids.contains(m_fluidGroup.m_fluidType));
		for(Block* adjacent : block->m_adjacentsVector)
			 if(adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(m_fluidGroup.m_fluidType) &&
				adjacent->m_fluids.at(m_fluidGroup.m_fluidType).first != s_maxBlockVolume
			   )
				addBlock(adjacent);
	}
}
void FillQueue::initalizeForStep()
{
	for(FutureFlowBlock& futureFlowBlock : m_queue)
	{
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->volumeOfFluidTypeCanEnter(m_fluidGroup.m_fluidType);
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
	assert(volume != 0);
	assert(m_groupStart->capacity >= volume);
	assert(m_groupStart != m_groupEnd);
	validate();
	// Record new member blocks for group.
	// Check the last member of the group since it's the most recently added.
	if((m_groupEnd - 1)->delta == 0 && !(m_groupEnd - 1)->block->m_fluids.contains(m_fluidGroup.m_fluidType))
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
	if(flowCapacity == volume)
	{
		if((m_groupStart->block->volumeOfFluidTypeContains(m_fluidGroup.m_fluidType) + m_groupStart->delta) == s_maxBlockVolume)
			for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
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
		assert(!iter->block->m_fluids.contains(m_fluidGroup.m_fluidType) || iter->block->m_fluids.at(m_fluidGroup.m_fluidType).second != nullptr);
		if(!iter->block->m_fluids.contains(m_fluidGroup.m_fluidType))
			iter->block->m_fluids.emplace(m_fluidGroup.m_fluidType, std::make_pair(iter->delta, &m_fluidGroup));
		else
			iter->block->m_fluids.at(m_fluidGroup.m_fluidType).first += iter->delta;
		iter->block->m_totalFluidVolume += iter->delta;
		if(iter->block->m_totalFluidVolume > s_maxBlockVolume)
			m_overfull.insert(iter->block);
	}
	validate();
}
uint32_t FillQueue::groupLevel() const
{
	assert(m_groupStart != m_groupEnd);
	uint32_t output = m_groupStart->delta;
	output += m_groupStart->block->volumeOfFluidTypeContains(m_fluidGroup.m_fluidType);
	return output;
}
uint32_t FillQueue::getPriority(FutureFlowBlock& futureFlowBlock) const
{
	if(futureFlowBlock.capacity == 0)
		return UINT32_MAX;
	return ((futureFlowBlock.block->m_z + 1) * s_maxBlockVolume * 2) - futureFlowBlock.capacity;
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
	assert(m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end());
	assert(m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end());
	assert(m_groupEnd == m_queue.end() || m_groupEnd == m_groupStart || m_groupStart->block->m_z != m_groupEnd->block->m_z || m_groupStart->capacity > m_groupEnd->capacity);
}
