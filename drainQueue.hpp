template<class DerivedBlock>
DrainQueue<DerivedBlock>::DrainQueue(FluidGroup<DerivedBlock>& fluidGroup) : FluidQueue<DerivedBlock>(fluidGroup) {}
template<class DerivedBlock>
void DrainQueue<DerivedBlock>::buildFor(std::unordered_set<DerivedBlock*>& members)
{
	FluidQueue<DerivedBlock>::m_set = members;
	for(DerivedBlock* block : members)
		FluidQueue<DerivedBlock>::m_queue.emplace_back(block);
}
template<class DerivedBlock>
void DrainQueue<DerivedBlock>::initalizeForStep()
{
	for(FutureFlowBlock<DerivedBlock>& futureFlowBlock : FluidQueue<DerivedBlock>::m_queue)
	{
		assert(futureFlowBlock.block->m_fluids.contains(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType));
		assert(futureFlowBlock.block->m_fluids.at(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType).first <= s_maxBlockVolume);
		assert(futureFlowBlock.block->m_fluids.at(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType).first != 0);
		assert(futureFlowBlock.block->m_totalFluidVolume <= s_maxBlockVolume);
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->m_fluids.at(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType).first;
	}
	std::ranges::sort(FluidQueue<DerivedBlock>::m_queue.begin(), FluidQueue<DerivedBlock>::m_queue.end(), [&](FutureFlowBlock<DerivedBlock>& a, FutureFlowBlock<DerivedBlock>& b){
		return getPriority(a) > getPriority(b);
	});
	FluidQueue<DerivedBlock>::m_groupStart = FluidQueue<DerivedBlock>::m_queue.begin();
	findGroupEnd();
	m_futureEmpty.clear();
	m_futureNoLongerFull.clear();
	m_futurePotentialNoLongerAdjacent.clear();
}
template<class DerivedBlock>
void DrainQueue<DerivedBlock>::recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep)
{
	assert(volume != 0);
	assert(FluidQueue<DerivedBlock>::m_groupStart != FluidQueue<DerivedBlock>::m_groupEnd);
	assert(FluidQueue<DerivedBlock>::m_groupStart->capacity >= volume);
	assert(FluidQueue<DerivedBlock>::m_groupStart >= FluidQueue<DerivedBlock>::m_queue.begin() && FluidQueue<DerivedBlock>::m_groupStart <= FluidQueue<DerivedBlock>::m_queue.end());
	assert(FluidQueue<DerivedBlock>::m_groupEnd >= FluidQueue<DerivedBlock>::m_queue.begin() && FluidQueue<DerivedBlock>::m_groupEnd <= FluidQueue<DerivedBlock>::m_queue.end());
	// Record no longer full.
	if(FluidQueue<DerivedBlock>::m_groupStart->block->m_totalFluidVolume == s_maxBlockVolume && !m_futureNoLongerFull.contains((FluidQueue<DerivedBlock>::m_groupEnd-1)->block))
		for(auto iter = FluidQueue<DerivedBlock>::m_groupStart; iter != FluidQueue<DerivedBlock>::m_groupEnd; ++iter)
			m_futureNoLongerFull.insert(iter->block);
	// Record fluid level changes.
	for(auto iter = FluidQueue<DerivedBlock>::m_groupStart; iter != FluidQueue<DerivedBlock>::m_groupEnd; ++iter)
	{
		iter->delta += volume;
		assert(iter->delta <= s_maxBlockVolume);
		iter->capacity -= volume;
	}
	// Record empty blocks and get next group.
	if(volume == flowCapacity)
	{
		for(auto iter = FluidQueue<DerivedBlock>::m_groupStart; iter != FluidQueue<DerivedBlock>::m_groupEnd; ++iter)
			m_futureEmpty.insert(iter->block);
		FluidQueue<DerivedBlock>::m_groupStart = FluidQueue<DerivedBlock>::m_groupEnd;
		findGroupEnd();
	} 
	// Expand group for new higher level.
	else if(volume == flowTillNextStep)
		findGroupEnd();
}
template<class DerivedBlock>
void DrainQueue<DerivedBlock>::applyDelta()
{
	assert(FluidQueue<DerivedBlock>::m_groupStart >= FluidQueue<DerivedBlock>::m_queue.begin() && FluidQueue<DerivedBlock>::m_groupStart <= FluidQueue<DerivedBlock>::m_queue.end());
	assert(FluidQueue<DerivedBlock>::m_groupEnd >= FluidQueue<DerivedBlock>::m_queue.begin() && FluidQueue<DerivedBlock>::m_groupEnd <= FluidQueue<DerivedBlock>::m_queue.end());
	std::unordered_set<DerivedBlock*> drainedFromAndAdjacent;
	for(auto iter = FluidQueue<DerivedBlock>::m_queue.begin(); iter != FluidQueue<DerivedBlock>::m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert(iter->block->m_fluids.contains(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType));
		assert(iter->block->m_fluids.at(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType).first >= iter->delta);
		assert(iter->block->m_totalFluidVolume >= iter->delta);
		auto found = iter->block->m_fluids.find(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType);
		found->second.first -= iter->delta;
		iter->block->m_totalFluidVolume -= iter->delta;
		if(found->second.first == 0)
			iter->block->m_fluids.erase(found);
		// Record blocks to set fluid groups unstable.
		drainedFromAndAdjacent.insert(iter->block);
		for(DerivedBlock* adjacent : iter->block->m_adjacentsVector)
			if(adjacent->fluidCanEnterEver())
				drainedFromAndAdjacent.insert(adjacent);
	}
	// Set fluidGroups unstable.
	// TODO: Would it be better to prevent fluid groups from becoming stable while in contact with another group? Either option seems bad.
	for(DerivedBlock* block : drainedFromAndAdjacent)
		for(auto& [fluidType, pair] : block->m_fluids)
			if(fluidType != FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType)
				pair.second->m_stable = false;
}
template<class DerivedBlock>
uint32_t DrainQueue<DerivedBlock>::groupLevel() const
{
	assert(FluidQueue<DerivedBlock>::m_groupStart != FluidQueue<DerivedBlock>::m_groupEnd);
	return FluidQueue<DerivedBlock>::m_groupStart->block->m_fluids.at(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType).first - FluidQueue<DerivedBlock>::m_groupStart->delta;
}
template<class DerivedBlock>
uint32_t DrainQueue<DerivedBlock>::getPriority(FutureFlowBlock<DerivedBlock>& futureFlowBlock) const
{
	return futureFlowBlock.block->m_z * s_maxBlockVolume * 2 + futureFlowBlock.capacity;
}
template<class DerivedBlock>
void DrainQueue<DerivedBlock>::findGroupEnd()
{
	if(FluidQueue<DerivedBlock>::m_groupStart == FluidQueue<DerivedBlock>::m_queue.end())
	{
		FluidQueue<DerivedBlock>::m_groupEnd = FluidQueue<DerivedBlock>::m_groupStart;
		return;
	}
	uint32_t priority = getPriority(*FluidQueue<DerivedBlock>::m_groupStart);
	for(FluidQueue<DerivedBlock>::m_groupEnd = FluidQueue<DerivedBlock>::m_groupStart + 1; FluidQueue<DerivedBlock>::m_groupEnd != FluidQueue<DerivedBlock>::m_queue.end(); ++FluidQueue<DerivedBlock>::m_groupEnd)
		if(getPriority(*FluidQueue<DerivedBlock>::m_groupEnd) != priority)
			break;
}
