DrainQueue::DrainQueue(FluidGroup& fluidGroup) : FluidQueue(fluidGroup) {}
void DrainQueue::buildFor(std::unordered_set<Block*>& members)
{
	FluidQueue::m_set = members;
	for(Block* block : members)
		FluidQueue::m_queue.emplace_back(block);
}
void DrainQueue::initalizeForStep()
{
	for(FutureFlowBlock& futureFlowBlock : FluidQueue::m_queue)
	{
		assert((futureFlowBlock.block->m_fluids.contains(&FluidQueue::m_fluidGroup.m_fluidType)));
		assert(futureFlowBlock.block->m_totalFluidVolume <= Config::maxBlockVolume);
		futureFlowBlock.delta = 0;
		futureFlowBlock.capacity = futureFlowBlock.block->m_fluids.at(&FluidQueue::m_fluidGroup.m_fluidType).first;
	}
	std::ranges::sort(FluidQueue::m_queue.begin(), FluidQueue::m_queue.end(), [&](FutureFlowBlock& a, FutureFlowBlock& b){
		return getPriority(a) > getPriority(b);
	});
	FluidQueue::m_groupStart = FluidQueue::m_queue.begin();
	findGroupEnd();
	m_futureEmpty.clear();
	m_futureNoLongerFull.clear();
	m_futurePotentialNoLongerAdjacent.clear();
}
void DrainQueue::recordDelta(uint32_t volume, uint32_t flowCapacity, uint32_t flowTillNextStep)
{
	assert(volume != 0);
	assert((FluidQueue::m_groupStart != FluidQueue::m_groupEnd));
	assert((FluidQueue::m_groupStart->capacity >= volume));
	assert((FluidQueue::m_groupStart >= FluidQueue::m_queue.begin() && FluidQueue::m_groupStart <= FluidQueue::m_queue.end()));
	assert((FluidQueue::m_groupEnd >= FluidQueue::m_queue.begin() && FluidQueue::m_groupEnd <= FluidQueue::m_queue.end()));
	// Record no longer full.
	if(FluidQueue::m_groupStart->block->m_totalFluidVolume == Config::maxBlockVolume && !m_futureNoLongerFull.contains((FluidQueue::m_groupEnd-1)->block))
		for(auto iter = FluidQueue::m_groupStart; iter != FluidQueue::m_groupEnd; ++iter)
			m_futureNoLongerFull.insert(iter->block);
	// Record fluid level changes.
	for(auto iter = FluidQueue::m_groupStart; iter != FluidQueue::m_groupEnd; ++iter)
	{
		iter->delta += volume;
		assert(iter->delta <= Config::maxBlockVolume);
		iter->capacity -= volume;
	}
	// Record empty blocks and get next group.
	if(volume == flowCapacity)
	{
		for(auto iter = FluidQueue::m_groupStart; iter != FluidQueue::m_groupEnd; ++iter)
			m_futureEmpty.insert(iter->block);
		FluidQueue::m_groupStart = FluidQueue::m_groupEnd;
		findGroupEnd();
	} 
	// Expand group for new higher level.
	else if(volume == flowTillNextStep)
		findGroupEnd();
}
void DrainQueue::applyDelta()
{
	assert((FluidQueue::m_groupStart >= FluidQueue::m_queue.begin() && FluidQueue::m_groupStart <= FluidQueue::m_queue.end()));
	assert((FluidQueue::m_groupEnd >= FluidQueue::m_queue.begin() && FluidQueue::m_groupEnd <= FluidQueue::m_queue.end()));
	std::unordered_set<Block*> drainedFromAndAdjacent;
	for(auto iter = FluidQueue::m_queue.begin(); iter != FluidQueue::m_groupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert((iter->block->m_fluids.contains(&FluidQueue::m_fluidGroup.m_fluidType)));
		assert((iter->block->m_fluids.at(&FluidQueue::m_fluidGroup.m_fluidType).first >= iter->delta));
		assert(iter->block->m_totalFluidVolume >= iter->delta);
		auto found = iter->block->m_fluids.find(&FluidQueue::m_fluidGroup.m_fluidType);
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
			if(fluidType != FluidQueue::m_fluidGroup.m_fluidType)
				pair.second->m_stable = false;
}
uint32_t DrainQueue::groupLevel() const
{
	assert((FluidQueue::m_groupStart != FluidQueue::m_groupEnd));
	return FluidQueue::m_groupStart->block->m_fluids.at(&FluidQueue::m_fluidGroup.m_fluidType).first - FluidQueue::m_groupStart->delta;
}
uint32_t DrainQueue::getPriority(FutureFlowBlock& futureFlowBlock) const
{
	return futureFlowBlock.block->m_z * Config::maxBlockVolume * 2 + futureFlowBlock.capacity;
}
void DrainQueue::findGroupEnd()
{
	if(FluidQueue::m_groupStart == FluidQueue::m_queue.end())
	{
		FluidQueue::m_groupEnd = FluidQueue::m_groupStart;
		return;
	}
	uint32_t priority = getPriority(*FluidQueue::m_groupStart);
	for(FluidQueue::m_groupEnd = FluidQueue::m_groupStart + 1; FluidQueue::m_groupEnd != FluidQueue::m_queue.end(); ++FluidQueue::m_groupEnd)
		if(getPriority(*FluidQueue::m_groupEnd) != priority)
			break;
}
