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
		assert(futureFlowBlock.block->containsFluidType(m_fluidGroup.m_fluidType));
		assert(futureFlowBlock.block->volumeOfFluidTypeContains(m_fluidGroup.m_fluidType) <= s_maxBlockVolume);
		assert(futureFlowBlock.block->getTotalFluidVolume() <= s_maxBlockVolume);
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->volumeOfFluidTypeContains(m_fluidGroup.m_fluidType);
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
	assert(m_groupStart != m_groupEnd);
	assert(m_groupStart->capacity >= volume);
	assert(m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end());
	assert(m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end());
	// Record no longer full.
	if(m_groupStart->block->getTotalFluidVolume() == s_maxBlockVolume && !m_futureNoLongerFull.contains((m_groupEnd-1)->block))
		for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
			m_futureNoLongerFull.insert(iter->block);
	// Record fluid level changes.
	for(auto iter = m_groupStart; iter != m_groupEnd; ++iter)
	{
		iter->delta += volume;
		assert(iter->delta <= s_maxBlockVolume);
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
	assert(m_groupStart >= m_queue.begin() && m_groupStart <= m_queue.end());
	assert(m_groupEnd >= m_queue.begin() && m_groupEnd <= m_queue.end());
	std::unordered_set<Block*> drainedFromAndAdjacent;
	// Create blocksAndDeltas to iterate over so we can modify m_queue.
	std::vector<std::pair<Block*, uint32_t>> blocksAndDeltas;
	for(auto iter = m_queue.begin(); iter != m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert(iter->block->containsFluidType(m_fluidGroup.m_fluidType));
		assert(iter->block->volumeOfFluidTypeContains(m_fluidGroup.m_fluidType) >= iter->delta);
		assert(iter->block->getTotalFluidVolume() >= iter->delta);
		assert(m_set.contains(iter->block));
		blocksAndDeltas.emplace_back(iter->block, iter->delta);
	}
	for(auto [block, delta] : blocksAndDeltas)
	{
		block->removeFluidVolume(delta, m_fluidGroup.m_fluidType);
		// Record blocks to set fluid groups unstable.
		drainedFromAndAdjacent.insert(block);
		for(Block* adjacent : block->m_adjacentsVector)
			if(adjacent->fluidCanEnterEver() && 
				(adjacent->getFluidGroup(m_fluidGroup.m_fluidType) != &m_fluidGroup)
			  )
				drainedFromAndAdjacent.insert(adjacent);
	}
	// Set fluidGroups unstable.
	// TODO: Would it be better to prevent fluid groups from becoming stable while in contact with another group? Either option seems bad.
	for(Block* block : drainedFromAndAdjacent)
		block->setAllFluidGroupsUnstableExcept(m_fluidGroup.m_fluidType);
}
uint32_t DrainQueue::groupLevel() const
{
	assert(m_groupStart != m_groupEnd);
	return m_groupStart->block->volumeOfFluidTypeContains(m_fluidGroup.m_fluidType) - m_groupStart->delta;
}
uint32_t DrainQueue::getPriority(FutureFlowBlock& futureFlowBlock) const
{
	return futureFlowBlock.block->m_z * s_maxBlockVolume * 2 + futureFlowBlock.capacity;
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
