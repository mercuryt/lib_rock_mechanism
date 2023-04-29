template<class DerivedBlock>
FillQueue<DerivedBlock>::FillQueue(FluidGroup<DerivedBlock>& fluidGroup) : FluidQueue<DerivedBlock>(fluidGroup) {} 
template<class DerivedBlock>
void FillQueue<DerivedBlock>::buildFor(std::unordered_set<DerivedBlock*>& members)
{
	for(DerivedBlock* block : members)
	{
		assert(block->m_fluids.contains(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType));
		for(DerivedBlock* adjacent : block->m_adjacentsVector)
			 if(adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType) &&
				adjacent->m_fluids.at(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType).first != s_maxBlockVolume
			   )
				addBlock(adjacent);
	}
}
template<class DerivedBlock>
void FillQueue<DerivedBlock>::initalizeForStep()
{
	for(FutureFlowBlock<DerivedBlock>& futureFlowBlock : FluidQueue<DerivedBlock>::m_queue)
	{
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->volumeOfFluidTypeCanEnter(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType);
	}
	std::ranges::sort(FluidQueue<DerivedBlock>::m_queue.begin(), FluidQueue<DerivedBlock>::m_queue.end(), [&](FutureFlowBlock<DerivedBlock>& a, FutureFlowBlock<DerivedBlock>& b){
		return getPriority(a) < getPriority(b);
	});
	FluidQueue<DerivedBlock>::m_groupStart = FluidQueue<DerivedBlock>::m_queue.begin();
	findGroupEnd();
	m_futureFull.clear();
	m_futureNoLongerEmpty.clear();
	m_overfull.clear();
	validate();
}
template<class DerivedBlock>
void FillQueue<DerivedBlock>::recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep)
{
	assert(FluidQueue<DerivedBlock>::m_groupStart->capacity >= volume);
	assert(volume != 0);
	assert(FluidQueue<DerivedBlock>::m_groupStart != FluidQueue<DerivedBlock>::m_groupEnd);
	validate();
	// Record fluid level changes.
	for(auto iter = FluidQueue<DerivedBlock>::m_groupStart; iter != FluidQueue<DerivedBlock>::m_groupEnd; ++iter)
	{
		if(iter->delta == 0 && !iter->block->m_fluids.contains(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType))
			m_futureNoLongerEmpty.insert(iter->block);
		iter->delta += volume;
		assert(iter->delta <= s_maxBlockVolume);
		assert(iter->capacity >= volume);
		iter->capacity -= volume;
	}
	// Record full blocks and get next group.
	if(flowCapacity == volume)
	{
		assert((FluidQueue<DerivedBlock>::m_groupStart->block->volumeOfFluidTypeContains(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType) + FluidQueue<DerivedBlock>::m_groupStart->delta) <= s_maxBlockVolume);
		for(auto iter = FluidQueue<DerivedBlock>::m_groupStart; iter != FluidQueue<DerivedBlock>::m_groupEnd; ++iter)
			if((iter->block->volumeOfFluidTypeContains(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType) + iter->delta) == s_maxBlockVolume)
				m_futureFull.insert(iter->block);
		FluidQueue<DerivedBlock>::m_groupStart = FluidQueue<DerivedBlock>::m_groupEnd;
		findGroupEnd();
	} 
	// Expand group for new higher level.
	// TODO: continue from current position rather then reseting to start + 1.
	else if(flowTillNextStep == volume)
		findGroupEnd();
	validate();
}
template<class DerivedBlock>
void FillQueue<DerivedBlock>::applyDelta()
{
	validate();
	for(auto iter = FluidQueue<DerivedBlock>::m_queue.begin(); iter != FluidQueue<DerivedBlock>::m_groupEnd; ++iter)
	{
		// TODO: This seems hackey, should try gather instead, also for drain.
		if(iter->delta == 0)
			continue;
		assert(!iter->block->m_fluids.contains(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType) || iter->block->m_fluids.at(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType).second != nullptr);
		auto found = iter->block->m_fluids.find(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType);
		if(found == iter->block->m_fluids.end())
		{
			FluidGroup<DerivedBlock>& fluidGroup = FluidQueue<DerivedBlock>::m_fluidGroup;
			iter->block->m_fluids.emplace(fluidGroup.m_fluidType, std::make_pair(iter->delta, &fluidGroup));
		}
		else
		{
			found->second.first += iter->delta;
			assert(found->second.second->m_fluidType == FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType);
		}
		iter->block->m_totalFluidVolume += iter->delta;
		/*assert(iter->block->m_fluids.at(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType).second != &m_fluidGroup ||
				(iter->block->m_fluids.at(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType).first < s_maxBlockVolume && !m_futureFull.contains(iter->block)) ||
				(iter->block->m_fluids.at(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType).first == s_maxBlockVolume && m_futureFull.contains(iter->block)));
				*/
		if(iter->block->m_totalFluidVolume > s_maxBlockVolume)
			m_overfull.insert(iter->block);
	}
	validate();
}
template<class DerivedBlock>
uint32_t FillQueue<DerivedBlock>::groupLevel() const
{
	assert(FluidQueue<DerivedBlock>::m_groupStart != FluidQueue<DerivedBlock>::m_groupEnd);
	//TODO: calculate this durring find end.
	uint32_t highestLevel = 0;
	for(auto it = FluidQueue<DerivedBlock>::m_groupStart; it != FluidQueue<DerivedBlock>::m_groupEnd; ++it)
	{
		uint32_t level = it->delta + it->block->volumeOfFluidTypeContains(FluidQueue<DerivedBlock>::m_fluidGroup.m_fluidType);
		if(level > highestLevel)
			highestLevel = level;

	}
	return highestLevel;
}
template<class DerivedBlock>
uint32_t FillQueue<DerivedBlock>::getPriority(FutureFlowBlock<DerivedBlock>& futureFlowBlock) const
{
	if(futureFlowBlock.capacity == 0)
		return UINT32_MAX;
	return ((futureFlowBlock.block->m_z + 1) * s_maxBlockVolume * 2) - futureFlowBlock.capacity;
}
template<class DerivedBlock>
void FillQueue<DerivedBlock>::findGroupEnd()
{
	if(FluidQueue<DerivedBlock>::m_groupStart == FluidQueue<DerivedBlock>::m_queue.end() || FluidQueue<DerivedBlock>::m_groupStart->capacity == 0)
	{
		FluidQueue<DerivedBlock>::m_groupEnd = FluidQueue<DerivedBlock>::m_groupStart;
		return;
	}
	uint32_t priority = getPriority(*FluidQueue<DerivedBlock>::m_groupStart);
	for(FluidQueue<DerivedBlock>::m_groupEnd = FluidQueue<DerivedBlock>::m_groupStart + 1; FluidQueue<DerivedBlock>::m_groupEnd != FluidQueue<DerivedBlock>::m_queue.end(); ++FluidQueue<DerivedBlock>::m_groupEnd)
	{
		uint32_t otherPriority = getPriority(*FluidQueue<DerivedBlock>::m_groupEnd);
		assert(priority <= otherPriority);
		if(otherPriority != priority)
			break;
	}
	validate();
}
template<class DerivedBlock>
void FillQueue<DerivedBlock>::validate() const
{
	assert(FluidQueue<DerivedBlock>::m_groupStart >= FluidQueue<DerivedBlock>::m_queue.begin() && FluidQueue<DerivedBlock>::m_groupStart <= FluidQueue<DerivedBlock>::m_queue.end());
	assert(FluidQueue<DerivedBlock>::m_groupEnd >= FluidQueue<DerivedBlock>::m_queue.begin() && FluidQueue<DerivedBlock>::m_groupEnd <= FluidQueue<DerivedBlock>::m_queue.end());
	assert(FluidQueue<DerivedBlock>::m_groupEnd == FluidQueue<DerivedBlock>::m_queue.end() || FluidQueue<DerivedBlock>::m_groupEnd == FluidQueue<DerivedBlock>::m_groupStart || FluidQueue<DerivedBlock>::m_groupStart->block->m_z != FluidQueue<DerivedBlock>::m_groupEnd->block->m_z || FluidQueue<DerivedBlock>::m_groupStart->capacity > FluidQueue<DerivedBlock>::m_groupEnd->capacity);
}
