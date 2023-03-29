FillQueue::FillQueue(FluidGroup& fluidGroup) : FluidQueue(fluidGroup) {} 
void FillQueue::buildFor(std::unordered_set<Block*>& members)
{
	for(Block* block : members)
	{
		assert(block->containsFluidType(m_fluidGroup.m_fluidType));
		for(Block* adjacent : block->m_adjacentsVector)
			 if(adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(m_fluidGroup.m_fluidType) &&
				adjacent->volumeOfFluidTypeContains(m_fluidGroup.m_fluidType) != s_maxBlockVolume
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
	assert(m_groupStart->capacity >= volume);
	assert(m_groupStart != m_groupEnd);
	validate();
	// Record new member blocks for group.
	// Check the last member of the group since it's the most recently added.
	if((m_groupEnd - 1)->delta == 0 && !(m_groupEnd - 1)->block->containsFluidType(m_fluidGroup.m_fluidType))
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
		// Capacity == volume is the same as futureFull when only one fluid type is present, but we need to look at each block individually because blocks with the same capacity for a type don't neccesarily have the same volume of that type.
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
		{
			assert((iter->block->volumeOfFluidTypeContains(m_fluidGroup.m_fluidType) + iter->delta) <= s_maxBlockVolume);
			if((iter->block->volumeOfFluidTypeContains(m_fluidGroup.m_fluidType) + iter->delta) == s_maxBlockVolume)
				m_futureFull.insert(iter->block);
		}
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
	std::vector<std::pair<Block*, uint32_t>> blocksAndDeltas;
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		// TODO: This seems hackey, should try gather instead, also for drain.
		if(iter->delta == 0)
			continue;
		assert(!iter->block->containsFluidType(m_fluidGroup.m_fluidType) || iter->block->getFluidGroup(m_fluidGroup.m_fluidType)!= nullptr);
		blocksAndDeltas.emplace_back(iter->block, iter->delta);
	}
	for(auto [block, delta] : blocksAndDeltas)
	{
		block->setFluidGroupAndAddVolume(m_fluidGroup, delta);
		if(block->getTotalFluidVolume() > s_maxBlockVolume)
			m_overfull.insert(block);
	}
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
