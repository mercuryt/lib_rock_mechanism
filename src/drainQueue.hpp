template<class Block, class Area, class FluidType>
DrainQueue<Block, Area, FluidType>::DrainQueue(FluidGroup<Block, Area, FluidType>& fluidGroup) : FluidQueue<Block, Area, FluidType>(fluidGroup) {}
template<class Block, class Area, class FluidType>
void DrainQueue<Block, Area, FluidType>::buildFor(std::unordered_set<Block*>& members)
{
	FluidQueue<Block, Area, FluidType>::m_set = members;
	for(Block* block : members)
		FluidQueue<Block, Area, FluidType>::m_queue.emplace_back(block);
}
template<class Block, class Area, class FluidType>
void DrainQueue<Block, Area, FluidType>::initalizeForStep()
{
	for(FutureFlowBlock<Block>& futureFlowBlock : FluidQueue<Block, Area, FluidType>::m_queue)
	{
		assert((futureFlowBlock.block->m_fluids.contains(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType)));
		assert(futureFlowBlock.block->m_totalFluidVolume <= Config::maxBlockVolume);
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->m_fluids.at(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType).first;
	}
	std::ranges::sort(FluidQueue<Block, Area, FluidType>::m_queue.begin(), FluidQueue<Block, Area, FluidType>::m_queue.end(), [&](FutureFlowBlock<Block>& a, FutureFlowBlock<Block>& b){
		return getPriority(a) > getPriority(b);
	});
	FluidQueue<Block, Area, FluidType>::m_groupStart = FluidQueue<Block, Area, FluidType>::m_queue.begin();
	findGroupEnd();
	m_futureEmpty.clear();
	m_futureNoLongerFull.clear();
	m_futurePotentialNoLongerAdjacent.clear();
}
template<class Block, class Area, class FluidType>
void DrainQueue<Block, Area, FluidType>::recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep)
{
	assert(volume != 0);
	assert((FluidQueue<Block, Area, FluidType>::m_groupStart != FluidQueue<Block, Area, FluidType>::m_groupEnd));
	assert((FluidQueue<Block, Area, FluidType>::m_groupStart->capacity >= volume));
	assert((FluidQueue<Block, Area, FluidType>::m_groupStart >= FluidQueue<Block, Area, FluidType>::m_queue.begin() && FluidQueue<Block, Area, FluidType>::m_groupStart <= FluidQueue<Block, Area, FluidType>::m_queue.end()));
	assert((FluidQueue<Block, Area, FluidType>::m_groupEnd >= FluidQueue<Block, Area, FluidType>::m_queue.begin() && FluidQueue<Block, Area, FluidType>::m_groupEnd <= FluidQueue<Block, Area, FluidType>::m_queue.end()));
	// Record no longer full.
	if(FluidQueue<Block, Area, FluidType>::m_groupStart->block->m_totalFluidVolume == Config::maxBlockVolume && !m_futureNoLongerFull.contains((FluidQueue<Block, Area, FluidType>::m_groupEnd-1)->block))
		for(auto iter = FluidQueue<Block, Area, FluidType>::m_groupStart; iter != FluidQueue<Block, Area, FluidType>::m_groupEnd; ++iter)
			m_futureNoLongerFull.insert(iter->block);
	// Record fluid level changes.
	for(auto iter = FluidQueue<Block, Area, FluidType>::m_groupStart; iter != FluidQueue<Block, Area, FluidType>::m_groupEnd; ++iter)
	{
		iter->delta += volume;
		assert(iter->delta <= Config::maxBlockVolume);
		iter->capacity -= volume;
	}
	// Record empty blocks and get next group.
	if(volume == flowCapacity)
	{
		for(auto iter = FluidQueue<Block, Area, FluidType>::m_groupStart; iter != FluidQueue<Block, Area, FluidType>::m_groupEnd; ++iter)
			m_futureEmpty.insert(iter->block);
		FluidQueue<Block, Area, FluidType>::m_groupStart = FluidQueue<Block, Area, FluidType>::m_groupEnd;
		findGroupEnd();
	} 
	// Expand group for new higher level.
	else if(volume == flowTillNextStep)
		findGroupEnd();
}
template<class Block, class Area, class FluidType>
void DrainQueue<Block, Area, FluidType>::applyDelta()
{
	assert((FluidQueue<Block, Area, FluidType>::m_groupStart >= FluidQueue<Block, Area, FluidType>::m_queue.begin() && FluidQueue<Block, Area, FluidType>::m_groupStart <= FluidQueue<Block, Area, FluidType>::m_queue.end()));
	assert((FluidQueue<Block, Area, FluidType>::m_groupEnd >= FluidQueue<Block, Area, FluidType>::m_queue.begin() && FluidQueue<Block, Area, FluidType>::m_groupEnd <= FluidQueue<Block, Area, FluidType>::m_queue.end()));
	std::unordered_set<Block*> drainedFromAndAdjacent;
	for(auto iter = FluidQueue<Block, Area, FluidType>::m_queue.begin(); iter != FluidQueue<Block, Area, FluidType>::m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert((iter->block->m_fluids.contains(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType)));
		assert((iter->block->m_fluids.at(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType).first >= iter->delta));
		assert(iter->block->m_totalFluidVolume >= iter->delta);
		auto found = iter->block->m_fluids.find(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType);
		found->second.first -= iter->delta;
		iter->block->m_totalFluidVolume -= iter->delta;
		if(found->second.first == 0)
			iter->block->m_fluids.erase(found);
		// Record blocks to set fluid groups unstable.
		drainedFromAndAdjacent.insert(iter->block);
		for(Block* adjacent : iter->block->m_adjacentsVector)
			if(adjacent->fluidCanEnterEver())
				drainedFromAndAdjacent.insert(adjacent);
	}
	// Set fluidGroups unstable.
	// TODO: Would it be better to prevent fluid groups from becoming stable while in contact with another group? Either option seems bad.
	for(Block* block : drainedFromAndAdjacent)
		for(auto& [fluidType, pair] : block->m_fluids)
			if(fluidType != FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType)
				pair.second->m_stable = false;
}
template<class Block, class Area, class FluidType>
uint32_t DrainQueue<Block, Area, FluidType>::groupLevel() const
{
	assert((FluidQueue<Block, Area, FluidType>::m_groupStart != FluidQueue<Block, Area, FluidType>::m_groupEnd));
	return FluidQueue<Block, Area, FluidType>::m_groupStart->block->m_fluids.at(FluidQueue<Block, Area, FluidType>::m_fluidGroup.m_fluidType).first - FluidQueue<Block, Area, FluidType>::m_groupStart->delta;
}
template<class Block, class Area, class FluidType>
uint32_t DrainQueue<Block, Area, FluidType>::getPriority(FutureFlowBlock<Block>& futureFlowBlock) const
{
	return futureFlowBlock.block->m_z * Config::maxBlockVolume * 2 + futureFlowBlock.capacity;
}
template<class Block, class Area, class FluidType>
void DrainQueue<Block, Area, FluidType>::findGroupEnd()
{
	if(FluidQueue<Block, Area, FluidType>::m_groupStart == FluidQueue<Block, Area, FluidType>::m_queue.end())
	{
		FluidQueue<Block, Area, FluidType>::m_groupEnd = FluidQueue<Block, Area, FluidType>::m_groupStart;
		return;
	}
	uint32_t priority = getPriority(*FluidQueue<Block, Area, FluidType>::m_groupStart);
	for(FluidQueue<Block, Area, FluidType>::m_groupEnd = FluidQueue<Block, Area, FluidType>::m_groupStart + 1; FluidQueue<Block, Area, FluidType>::m_groupEnd != FluidQueue<Block, Area, FluidType>::m_queue.end(); ++FluidQueue<Block, Area, FluidType>::m_groupEnd)
		if(getPriority(*FluidQueue<Block, Area, FluidType>::m_groupEnd) != priority)
			break;
}
