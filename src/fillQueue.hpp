template<class Block, class Area, class FluidType>
FillQueue<Block, Area, FluidType>::FillQueue(FluidGroup<Block, Area, FluidType>& fluidGroup) : FluidQueue<Block, Area, FluidType>(fluidGroup) {} 
template<class Block, class Area, class FluidType>
void FillQueue<Block, Area, FluidType>::buildFor(std::unordered_set<Block*>& members)
{
	for(Block* block : members)
	{
		assert(block->m_fluids.contains(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType));
		for(Block* adjacent : block->m_adjacentsVector)
			 if(adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType) &&
				adjacent->m_fluids.at(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType).first != Config::maxBlockVolume
			   )
				addBlock(adjacent);
	}
}
template<class Block, class Area, class FluidType>
void FillQueue<Block, Area, FluidType>::initalizeForStep()
{
	for(FutureFlowBlock<Block>& futureFlowBlock : FluidQueue<Block, Area, FluidType>::m_queue)
	{
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->volumeOfFluidTypeCanEnter(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType);
	}
	std::ranges::sort(FluidQueue<Block, Area, FluidType>::m_queue.begin(), FluidQueue<Block, Area, FluidType>::m_queue.end(), [&](FutureFlowBlock<Block>& a, FutureFlowBlock<Block>& b){
		return getPriority(a) < getPriority(b);
	});
	FluidQueue<Block, Area, FluidType>::m_groupStart = FluidQueue<Block, Area, FluidType>::m_queue.begin();
	findGroupEnd();
	m_futureFull.clear();
	m_futureNoLongerEmpty.clear();
	m_overfull.clear();
	validate();
}
template<class Block, class Area, class FluidType>
void FillQueue<Block, Area, FluidType>::recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep)
{
	assert((FluidQueue<Block, Area, FluidType>::m_groupStart->capacity >= volume));
	assert(volume != 0);
	assert((FluidQueue<Block, Area, FluidType>::m_groupStart != FluidQueue<Block, Area, FluidType>::m_groupEnd));
	validate();
	// Record fluid level changes.
	for(auto iter = FluidQueue<Block, Area, FluidType>::m_groupStart; iter != FluidQueue<Block, Area, FluidType>::m_groupEnd; ++iter)
	{
		if(iter->delta == 0 && !iter->block->m_fluids.contains(&FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType))
			m_futureNoLongerEmpty.insert(iter->block);
		iter->delta += volume;
		assert(iter->delta <= Config::maxBlockVolume);
		assert(iter->capacity >= volume);
		iter->capacity -= volume;
	}
	// Record full blocks and get next group.
	if(flowCapacity == volume)
	{
		assert((FluidQueue<Block, Area, FluidType>::m_groupStart->block->volumeOfFluidTypeContains(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType) + FluidQueue<Block, Area, FluidType>::m_groupStart->delta) <= Config::maxBlockVolume);
		for(auto iter = FluidQueue<Block, Area, FluidType>::m_groupStart; iter != FluidQueue<Block, Area, FluidType>::m_groupEnd; ++iter)
			if((iter->block->volumeOfFluidTypeContains(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType) + iter->delta) == Config::maxBlockVolume)
				m_futureFull.insert(iter->block);
		FluidQueue<Block, Area, FluidType>::m_groupStart = FluidQueue<Block, Area, FluidType>::m_groupEnd;
		findGroupEnd();
	} 
	// Expand group for new higher level.
	// TODO: continue from current position rather then reseting to start + 1.
	else if(flowTillNextStep == volume)
		findGroupEnd();
	validate();
}
template<class Block, class Area, class FluidType>
void FillQueue<Block, Area, FluidType>::applyDelta()
{
	validate();
	for(auto iter = FluidQueue<Block, Area, FluidType>::m_queue.begin(); iter != FluidQueue<Block, Area, FluidType>::m_groupEnd; ++iter)
	{
		// TODO: This seems hackey, should try gather instead, also for drain.
		if(iter->delta == 0)
			continue;
		assert(!iter->block->m_fluids.contains(&FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType) || iter->block->m_fluids.at(&FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType).second != nullptr);
		auto found = iter->block->m_fluids.find(&FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType);
		if(found == iter->block->m_fluids.end())
		{
			FluidGroup<Block, Area, FluidType>& fluidGroup = FluidQueue<Block, Area, FluidType>::m_fluidGroup;
			iter->block->m_fluids.emplace(&fluidGroup.m_fluidType, std::make_pair(iter->delta, &fluidGroup));
		}
		else
		{
			found->second.first += iter->delta;
			assert((found->second.second->m_fluidType == FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType));
		}
		iter->block->m_totalFluidVolume += iter->delta;
		/*assert(iter->block->m_fluids.at(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType).second != &m_fluidGroup ||
				(iter->block->m_fluids.at(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType).first < Config::maxBlockVolume && !m_futureFull.contains(iter->block)) ||
				(iter->block->m_fluids.at(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType).first == Config::maxBlockVolume && m_futureFull.contains(iter->block)));
				*/
		if(iter->block->m_totalFluidVolume > Config::maxBlockVolume)
			m_overfull.insert(iter->block);
	}
	validate();
}
template<class Block, class Area, class FluidType>
uint32_t FillQueue<Block, Area, FluidType>::groupLevel() const
{
	assert((FluidQueue<Block, Area, FluidType>::m_groupStart != FluidQueue<Block, Area, FluidType>::m_groupEnd));
	//TODO: calculate this durring find end.
	uint32_t highestLevel = 0;
	for(auto it = FluidQueue<Block, Area, FluidType>::m_groupStart; it != FluidQueue<Block, Area, FluidType>::m_groupEnd; ++it)
	{
		uint32_t level = it->delta + it->block->volumeOfFluidTypeContains(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType);
		if(level > highestLevel)
			highestLevel = level;

	}
	return highestLevel;
}
template<class Block, class Area, class FluidType>
uint32_t FillQueue<Block, Area, FluidType>::getPriority(FutureFlowBlock<Block>& futureFlowBlock) const
{
	if(futureFlowBlock.capacity == 0)
		return UINT32_MAX;
	return ((futureFlowBlock.block->m_z + 1) * Config::maxBlockVolume * 2) - futureFlowBlock.capacity;
}
template<class Block, class Area, class FluidType>
void FillQueue<Block, Area, FluidType>::findGroupEnd()
{
	if(FluidQueue<Block, Area, FluidType>::m_groupStart == FluidQueue<Block, Area, FluidType>::m_queue.end() || FluidQueue<Block, Area, FluidType>::m_groupStart->capacity == 0)
	{
		FluidQueue<Block, Area, FluidType>::m_groupEnd = FluidQueue<Block, Area, FluidType>::m_groupStart;
		return;
	}
	uint32_t priority = getPriority(*FluidQueue<Block, Area, FluidType>::m_groupStart);
	for(FluidQueue<Block, Area, FluidType>::m_groupEnd = FluidQueue<Block, Area, FluidType>::m_groupStart + 1; FluidQueue<Block, Area, FluidType>::m_groupEnd != FluidQueue<Block, Area, FluidType>::m_queue.end(); ++FluidQueue<Block, Area, FluidType>::m_groupEnd)
	{
		uint32_t otherPriority = getPriority(*FluidQueue<Block, Area, FluidType>::m_groupEnd);
		assert(priority <= otherPriority);
		if(otherPriority != priority)
			break;
	}
	validate();
}
template<class Block, class Area, class FluidType>
void FillQueue<Block, Area, FluidType>::validate() const
{
	assert((FluidQueue<Block, Area, FluidType>::m_groupStart >= FluidQueue<Block, Area, FluidType>::m_queue.begin() && FluidQueue<Block, Area, FluidType>::m_groupStart <= FluidQueue<Block, Area, FluidType>::m_queue.end()));
	assert((FluidQueue<Block, Area, FluidType>::m_groupEnd >= FluidQueue<Block, Area, FluidType>::m_queue.begin() && FluidQueue<Block, Area, FluidType>::m_groupEnd <= FluidQueue<Block, Area, FluidType>::m_queue.end()));
	assert((FluidQueue<Block, Area, FluidType>::m_groupEnd == FluidQueue<Block, Area, FluidType>::m_queue.end() || FluidQueue<Block, Area, FluidType>::m_groupEnd == FluidQueue<Block, Area, FluidType>::m_groupStart || FluidQueue<Block, Area, FluidType>::m_groupStart->block->m_z != FluidQueue<Block, Area, FluidType>::m_groupEnd->block->m_z || FluidQueue<Block, Area, FluidType>::m_groupStart->capacity > FluidQueue<Block, Area, FluidType>::m_groupEnd->capacity));
}
