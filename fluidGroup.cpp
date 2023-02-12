/*
 * A set of blocks which are adjacent to eachother and contain the same fluid.
 * Intented to be run in series in a pool thread.
 * Maintains two priority queues:
 * 	m_blocksByZandTotalFluidHeight is all blocks which have fluid. It is sorted by high to low. This is the source of flow.
 * 	m_adjacentAndUnfullBlocksByInverseZandTotalFluidHeight is all blocks which have some fluid but aren't full, as well as blocks with no fluid that fluid can enter and are adjacent. It is sorted by low to high. This is the destination of flow.
 * The flow for the step is computed in readStep and applied in writeStep so that readStep can run concurrently with other read only tasks.
 */

#include <queue>

#include "util.h"

//TODO: reuse blocks as m_blocks.
FluidGroup::FluidGroup(FluidType* ft, std::unordered_set<Block*> blocks):
	m_fluidType(ft), m_stable(false), m_destroy(false), m_excessVolume(0)
{
	for(Block* block : blocks)
		addBlock(block);
}

void FluidGroup::addBlockInternal(Block* block)
{
	if(m_blocks.contains(block))
		return;
	m_blocks.insert(block);
	m_drainQueue.emplace_back(block, 0, 0);
	if(block->fluidCanEnterCurrently(m_fluidType))
		m_fillQueue.emplace_back(block, 0, 0);
}
void FluidGroup::addBlock(Block* block, bool checkMerge)
{
	m_stable = false;
	addBlockInternal(block);
	// Add adjacent if fluid can enter.
	std::unordered_set<FluidGroup*> toMerge;
	for(Block* adjacent : block->m_adjacents)
	{
		if(adjacent == nullptr || !adjacent->fluidCanEnterEver(m_fluidType) || m_blocks.contains(adjacent))
		       continue;
		if(adjacent->fluidCanEnterCurrently(m_fluidType))
			m_fillQueue.emplace_back(adjacent, 0, 0);
		// Merge groups if needed.
		if(checkMerge && adjacent->m_fluids.contains(m_fluidType))
			toMerge.insert(adjacent->getFluidGroup(m_fluidType));
	}
	for(FluidGroup* oldGroup : toMerge)
	{
		for(Block* block : oldGroup->m_blocks)
			addBlock(block, false);
		delete oldGroup;
	}
}
void FluidGroup::removeBlockInternal(Block* block)
{
	m_blocks.erase(block);
	auto ifBlock = [&](FutureFluidBlock& futureFluidBlock){ return futureFluidBlock.block == block; };
	std::erase_if(m_drainQueue, ifBlock);
	std::erase_if(m_fillQueue, ifBlock);
}
void FluidGroup::removeBlocksInternal(std::unordered_set<Block*>& blocks)
{
	std::erase_if(m_blocks, [&](Block* block){ return blocks.contains(block); });
	std::erase_if(m_drainQueue, [&](FutureFluidBlock futureFluidBlock){ return blocks.contains(futureFluidBlock.block); });
	std::erase_if(m_fillQueue, [&](FutureFluidBlock futureFluidBlock){ return blocks.contains(futureFluidBlock.block); });
}
void FluidGroup::removeBlock(Block* block)
{
	m_stable = false;
	removeBlockInternal(block);
	//TODO: check for empty adjacent to remove.
	//TODO: check for group split.
}

// To be run before applying async future data.
void FluidGroup::absorb(FluidGroup* fluidGroup)
{
	// Insert other group drain queue at this groups drain group end so the two drain groups are contiguous.
	uint32_t drainGroupCount = std::distance(fluidGroup->m_drainGroupBegin, fluidGroup->m_drainGroupEnd);
	std::erase_if(fluidGroup->m_drainQueue, [&](FutureFluidBlock& futureFluidBlock){ return m_blocks.contains(futureFluidBlock.block); });
	m_drainQueue.insert(m_drainGroupEnd, fluidGroup->m_drainQueue.begin(), fluidGroup->m_drainQueue.end());
	m_drainGroupEnd += drainGroupCount;

	// And same for fill, but we might have overlaps here, discard them and add delta to excessVolume
	std::unordered_set<Block*> fillSet;
	for(FutureFluidBlock& futureFluidBlock : m_fillQueue)
		fillSet.insert(futureFluidBlock.block);
	uint32_t fillGroupCount = std::distance(fluidGroup->m_fillGroupBegin, fluidGroup->m_fillGroupEnd);
	std::erase_if(fluidGroup->m_fillQueue, [&](FutureFluidBlock& futureFluidBlock){ 
		if(!fillSet.contains(futureFluidBlock.block))
			return false;
		if(&futureFluidBlock < &(*(fluidGroup->m_fillGroupEnd)))
		{
			--fillGroupCount;
			m_excessVolume += futureFluidBlock.delta;
		}
		return true;
	});
	m_fillQueue.insert(m_fillGroupEnd, fluidGroup->m_fillQueue.begin(), fluidGroup->m_fillQueue.end());
	m_fillGroupEnd += fillGroupCount;

	// Merge future empty.
	m_futureEmpty.insert(fluidGroup->m_futureEmpty.begin(), fluidGroup->m_futureEmpty.end());
	// Merge future unfull.
	m_futureUnfull.insert(fluidGroup->m_futureUnfull.begin(), fluidGroup->m_futureUnfull.end());
	// Remove this group's future blocks from other group's future empty adajcent.
	std::erase_if(fluidGroup->m_futureEmptyAdjacents, [&](Block* block){ return  m_futureNewlyAdded.contains(block); });
	// Merge future empty adjacent.
	m_futureEmptyAdjacents.insert(fluidGroup->m_futureEmptyAdjacents.begin(), fluidGroup->m_futureEmptyAdjacents.end());
	// Remove this group's future newly adjacent from other group's future remove from adjacent.
	std::erase_if(fluidGroup->m_futureRemoveFromEmptyAdjacents, [&](Block* block){ return m_futureRemoveFromEmptyAdjacents.contains(block); });
	// Remove this group's adjacent from other group's future remove from adjacent.
	for(FutureFluidBlock& futureFluidBlock : m_fillQueue)
		fluidGroup->m_futureRemoveFromEmptyAdjacents.erase(futureFluidBlock.block);
	// Merge future empty adjacent.
	m_futureRemoveFromEmptyAdjacents.insert(fluidGroup->m_futureRemoveFromEmptyAdjacents.begin(), fluidGroup->m_futureRemoveFromEmptyAdjacents.end());
	// Merge future groups.
	m_futureGroups.insert(m_futureGroups.end(), fluidGroup->m_futureGroups.begin(), fluidGroup->m_futureGroups.end());
	// Merge future full.
	m_futureFull.insert(fluidGroup->m_futureFull.begin(), fluidGroup->m_futureFull.end());
	// Merge newly added.
	m_futureNewlyAdded.insert(fluidGroup->m_futureNewlyAdded.begin(), fluidGroup->m_futureNewlyAdded.end());
}
template<typename T>
void FluidGroup::recordFill(uint32_t flowPerBlock, uint32_t flowMaximum, uint32_t flowCapacity, uint32_t flowTillNextStep, T isEqualFill)
{
	// record future capacity and delta.
	for(auto iter = m_fillGroupBegin; iter != m_fillGroupEnd; ++iter)
	{
		iter->capacity -= flowPerBlock;
		iter->delta += flowPerBlock;
	}
	// If the first fill block did not contain fluid of this type before this step then add all of fill group to m_futureNewlyAdded.
	if(!m_fillGroupBegin->block->m_fluids.contains(m_fluidType))
		for(auto iter = m_fillGroupBegin; iter != m_fillGroupEnd; ++iter)
			m_futureNewlyAdded.insert(iter->block);
	// If the fill group is now full then record it in m_futureFull and get the next one.
	if(flowMaximum == flowCapacity)
	{
		for(auto iter = m_fillGroupBegin; iter != m_fillGroupEnd; ++iter)
			m_futureFull.insert(iter->block);
		m_fillGroupBegin = m_fillGroupEnd;
		m_fillGroupEnd = std::find_end(m_fillGroupBegin, m_fillGroupBegin + 1, m_fillGroupBegin, m_fillQueue.end(), isEqualFill);
	}
	// If the blocks are not full but are now at the same level as some other blocks with the same m_z then extend the fill group to include these new blocks.
	else if(flowMaximum == flowTillNextStep && m_fillGroupEnd != m_fillQueue.end())
		m_fillGroupEnd = std::find_end(m_fillGroupBegin, m_fillGroupBegin + 1, (m_fillGroupEnd - 1), m_fillQueue.end(), isEqualFill);
}
template<typename T>
void FluidGroup::recordDrain(uint32_t flowPerBlock, uint32_t flowMaximum, uint32_t flowCapacity, uint32_t flowTillNextStep, T isEqualDrain)
{
	// Record future capacity and delta.
	for(auto iter = m_drainGroupBegin; iter != m_drainGroupEnd; ++iter)
	{
		iter->capacity -= flowPerBlock;
		iter->delta -= flowPerBlock;
	}
	// If the first drain block did not have fill capacity before this step then add this group to newlyUnfull.
	if(!m_drainGroupBegin->block->fluidCanEnterCurrently(m_fluidType))
		for(auto iter = m_drainGroupBegin; iter != m_drainGroupEnd; ++iter)
			m_futureUnfull.insert(iter->block);
	// If the group is now empty then record it in futureEmpty and get the next one.
	if(flowCapacity == flowMaximum)
	{
		for(auto iter = m_drainGroupBegin; iter != m_drainGroupEnd; ++iter)
			m_futureEmpty.insert(iter->block);
		m_drainGroupBegin = m_drainGroupEnd;
		m_drainGroupEnd = std::find_end(m_drainGroupBegin, m_drainGroupBegin + 1, m_drainGroupBegin, m_drainQueue.end(), isEqualDrain);
	}
	// If the blocks are not full but are now at the same level as some other blocks with the same m_z then extend the drain group to include these new blocks.
	else if(flowMaximum == flowTillNextStep && m_drainGroupEnd != m_drainQueue.end())
		m_drainGroupEnd = std::find_end(m_drainGroupBegin, m_drainGroupBegin + 1, (m_drainGroupEnd - 1), m_drainQueue.end(), isEqualDrain);
}
uint32_t FluidGroup::drainPriority(FutureFluidBlock& futureFluidBlock) const
{
	return (futureFluidBlock.block->m_z * MAX_BLOCK_VOLUME) + futureFluidBlock.capacity;
}
uint32_t FluidGroup::fillPriority(FutureFluidBlock& futureFluidBlock) const
{
	// Add one to z to prevent negitive number.
	return ((futureFluidBlock.block->m_z + 1) * MAX_BLOCK_VOLUME) - futureFluidBlock.capacity;
}

void FluidGroup::readStep()
{
	m_futureFull.clear();
	m_futureNewlyAdded.clear();
	m_futureUnfull.clear();
	m_futureEmpty.clear();
	m_futureEmptyAdjacents.clear();
	m_futureGroups.clear();
	m_futureMerge.clear();
	m_futureNotifyUnfull.clear();
	// If there is no where to flow into there is nothing to do.
	if(m_fillQueue.empty())
	{
		m_stable = true;
		return;
	}
	for(FutureFluidBlock& futureFluidBlock : m_drainQueue)
	{
		futureFluidBlock.capacity = futureFluidBlock.block->m_fluids[m_fluidType].first;
		futureFluidBlock.delta = 0;
	}
	auto isHigher = [&](FutureFluidBlock a, FutureFluidBlock b){ return drainPriority(a) > drainPriority(b); };
	std::ranges::sort(m_drainQueue, isHigher);
	auto m_drainGroupBegin = m_drainQueue.begin();
	auto isEqualDrain = [&](FutureFluidBlock a, FutureFluidBlock b){ return drainPriority(a) == drainPriority(b); };
	auto m_drainGroupEnd = std::find_end(m_drainGroupBegin, m_drainGroupBegin + 1, m_drainQueue.begin(), m_drainQueue.end(), isEqualDrain);

	for(FutureFluidBlock& futureFluidBlock : m_fillQueue)
	{
		futureFluidBlock.capacity = futureFluidBlock.block->volumeOfFluidTypeCanEnter(m_fluidType);
		futureFluidBlock.delta = 0;
	}
	auto isLower = [&](FutureFluidBlock a, FutureFluidBlock b){ return fillPriority(a) < fillPriority(b); };
	std::ranges::sort(m_fillQueue, isLower);
	auto m_fillGroupBegin = m_fillQueue.begin();
	auto isEqualFill = [&](FutureFluidBlock a, FutureFluidBlock b){ return fillPriority(a) == fillPriority(b); };
	auto m_fillGroupEnd = std::find_end(m_fillGroupBegin, m_fillGroupBegin + 1, m_fillQueue.begin(), m_fillQueue.end(), isEqualFill);
	uint32_t viscosity = m_fluidType->viscosity;
	// Disperse m_excessVolume.
	while(m_excessVolume > 0)
	{
		uint32_t fillGroupCount = m_fillGroupEnd - m_fillGroupBegin;
		// If there isn't enough to spread evenly then do nothing.
		if(m_excessVolume < fillGroupCount)
			break;
		// How much fluid is there space for total.
		uint32_t flowCapacity = m_fillGroupBegin->capacity * fillGroupCount;
		// How much can be filled before the next low block(s).
		uint32_t flowTillNextStep= (
				m_fillGroupEnd == m_fillQueue.end()? 
				UINT32_MAX :
				(m_fillGroupEnd->capacity - m_fillGroupBegin->capacity) * fillGroupCount
		);
		uint32_t flowMaximum = std::min({flowTillNextStep, flowCapacity, m_excessVolume});
		uint32_t flowPerBlock = flowMaximum / fillGroupCount;
		uint32_t flow = flowPerBlock * fillGroupCount;
		uint32_t roundingExcess = flowMaximum - flow;
		m_excessVolume -= flow;
		m_excessVolume += roundingExcess;
		recordFill(flowPerBlock, flowMaximum, flowCapacity, flowTillNextStep, isEqualFill);
	}
	while(m_excessVolume < 0)
	{
		uint32_t drainGroupCount = m_drainGroupEnd - m_drainGroupBegin;
		// If there isn't enough to spread evenly then do nothing.
		if((-1 * m_excessVolume) < drainGroupCount)
			break;
		// How much is avaliable to drain total.
		uint32_t flowCapacity = drainGroupCount * m_drainGroupBegin->capacity;
		// How much can be drained before the next high block(s).
		uint32_t flowTillNextStep = (m_drainGroupEnd == m_drainQueue.end())?
			UINT32_MAX :
			(m_drainGroupBegin->capacity - m_drainGroupEnd->capacity) * drainGroupCount;
		uint32_t flowMaximum = std::min({flowCapacity, flowTillNextStep, m_excessVolume * -1});
		uint32_t flowPerBlock = flowMaximum / drainGroupCount;
		uint32_t flow = flowPerBlock * drainGroupCount;
		uint32_t roundingExcess = flowMaximum - flow;
		m_excessVolume += flow;
		m_excessVolume -= roundingExcess;
		recordDrain(flowPerBlock, flowMaximum, flowCapacity, flowTillNextStep, isEqualDrain);
	}
	// Do primary flow.
	while(viscosity > 0)
	{
		// If we have reached the end of either queue the step ends.
		if(m_drainGroupBegin == m_drainQueue.end() || m_fillGroupBegin == m_fillQueue.end())
			break;
		uint32_t drainGroupCount = m_drainGroupEnd - m_drainGroupBegin;
		uint32_t fillGroupCount = m_fillGroupEnd - m_fillGroupBegin;
		// If there isn't enough to spread evenly then do nothing.
		// How much fluid is there space for total.
		uint32_t flowCapacityFill = m_fillGroupBegin->capacity * fillGroupCount;
		// How much can be filled before the next low block(s).
		uint32_t flowTillNextStepFill = (
				m_fillGroupEnd == m_fillQueue.end()? 
				UINT32_MAX :
				(m_fillGroupEnd->capacity - m_fillGroupBegin->capacity) * fillGroupCount
		);
		// How much is avaliable to drain total.
		uint32_t flowCapacityDrain = drainGroupCount * m_drainGroupBegin->capacity;
		// How much can be drained before the next high block(s).
		uint32_t flowTillNextStepDrain = (m_drainGroupEnd == m_drainQueue.end()?
			UINT32_MAX :
			(m_drainGroupBegin->capacity - m_drainGroupEnd->capacity) * drainGroupCount
		);
		// How much can flow before equalization. Only applies if the groups are on the same z.
		uint32_t maxFillForEqualibrium, maxDrainForEqualibrium;
		if(m_fillGroupBegin->block->m_z == m_drainGroupBegin->block->m_z)
		{
			uint32_t fillLevel = m_fillGroupBegin->capacity;
			uint32_t drainLevel = m_drainGroupBegin->block->volumeOfFluidTypeCanEnter(m_fluidType)  - m_drainGroupBegin->delta;
			uint32_t totalLevel = (fillLevel * fillGroupCount) + (drainLevel * drainGroupCount);
			uint32_t totalCount = fillGroupCount + drainGroupCount;
			uint32_t equalibriumLevel = std::floor(totalLevel / totalCount);
			uint32_t maxDrainForEqualibrium = (drainLevel - equalibriumLevel) * drainGroupCount;
			uint32_t maxFillForEqualibrium = (fillLevel - equalibriumLevel) * fillGroupCount;
		}
		else
			maxFillForEqualibrium = maxDrainForEqualibrium = UINT32_MAX;
		// Find the lowest of these 6.
		uint32_t flowMaximum = std::min({maxDrainForEqualibrium, maxFillForEqualibrium, flowCapacityDrain, flowCapacityFill, flowTillNextStepDrain, flowTillNextStepFill});
		// Viscosity is consumed by flow.
		viscosity -= flowMaximum / fillGroupCount;
		if(viscosity < 0)
			flowMaximum += viscosity * fillGroupCount;
		// Record changes.
		uint32_t flowPerBlockDrain = flowMaximum / drainGroupCount;
		uint32_t flow = flowPerBlockDrain * drainGroupCount;
		recordDrain(flowPerBlockDrain, flowMaximum, flowCapacityDrain, flowTillNextStepDrain, isEqualDrain);
		uint32_t flowPerBlockFill = flow / fillGroupCount;
		uint32_t fillGroupTotalFlowActual = flowPerBlockFill * fillGroupCount;
		uint32_t roundingExcess = flow - fillGroupTotalFlowActual;
		m_excessVolume += roundingExcess;
		recordFill(flowPerBlockFill, flowMaximum, flowCapacityFill, flowTillNextStepFill, isEqualFill);
	}
	// Flow loops completed, anilyze results.
	// Create new m_blocks;
	m_futureBlocks = m_blocks;
	std::erase_if(m_futureBlocks, [&](Block* block){ return m_futureEmpty.contains(block); });
	m_futureBlocks.insert(m_futureNewlyAdded.begin(), m_futureNewlyAdded.end());
	// Find any newly created groups.
	// Collect blocks adjacent to newly empty which are not empty.
	std::unordered_set<Block*> adjacentsForNewGroups;
	std::unordered_set<Block*> adjacentsForPossibleRemovalFromEmptyAdjacentsQueue;
	// Collect all adjacent to futureEmpty which fluid can enter ever.
	std::unordered_set<Block*> adjacentToFutureEmpty;
	for(Block* block : m_futureEmpty)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver(m_fluidType))
				adjacentToFutureEmpty.insert(adjacent);
	for(Block* block : adjacentToFutureEmpty)
		// If they won't be empty then check for forming a new group as they may be detached.
		if(m_futureBlocks.contains(block))
			adjacentsForNewGroups.insert(block);
		// Else check for removal from empty adjacent queue.
		else
			adjacentsForPossibleRemovalFromEmptyAdjacentsQueue.insert(block);
	// Seperate into contiguous groups. Each block in adjacentsForNewGroups might be in a seperate group.
	std::unordered_set<Block*> closed;
	for(Block* block : adjacentsForNewGroups)
	{
		if(closed.contains(block))
			continue;
		auto condition = [&](Block* block){ return m_futureBlocks.contains(block); };
		std::unordered_set<Block*> adjacents = util::collectAdjacentsWithCondition(condition, block);
		// Add whole group to closed. There is only one iteration per group as others will be rejected by the closed guard.
		closed.insert(adjacents.begin(), adjacents.end());
		m_futureGroups.push_back(adjacents);
	}
	// Sort by size.
	auto condition = [](std::unordered_set<Block*> a, std::unordered_set<Block*> b){ return a.size() > b.size(); };
	std::sort(m_futureGroups.begin(), m_futureGroups.end(), condition);
	// Find adjacent empty to remove from queue.
	m_futureRemoveFromEmptyAdjacents.clear();
	for(Block* block : adjacentsForPossibleRemovalFromEmptyAdjacentsQueue)
	{
		for(Block* adjacent : block->m_adjacents)
			// If adjacent to the largest group (the one that will continue to use this object).
			if(adjacent == nullptr || m_futureGroups.front().contains(adjacent))
				continue;
		m_futureRemoveFromEmptyAdjacents.insert(block);
	}
	// Find future blocks for adjacentEmptyQueue and groups to merge with.
	m_futureEmptyAdjacents.clear();
	m_futureMerge.clear();
	std::unordered_set<Block*> adjacentToNewlyAdded;
	for(Block* block : m_futureNewlyAdded)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver(m_fluidType))
				adjacentToNewlyAdded.insert(adjacent);
	for(Block* block : adjacentToNewlyAdded)
		if(!m_futureBlocks.contains(block))
			m_futureEmptyAdjacents.insert(block);
		else if(!m_futureGroups.front().contains(block))
			// Block is adjacent to a newly added block and contains the same fluid, merge groups.
			m_futureMerge.insert(block->getFluidGroup(m_fluidType));
	// Find blocks adjacent to m_futureUnfull.
	std::unordered_set<Block*> adjacents;
	for(Block* block : m_futureUnfull)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver()) // note: not sending m_fluidType here because we want to know if any fluid can enter.
				adjacents.insert(adjacent);
	// Record FluidGroups of adjacent blocks with another type of fluid.
	for(Block* adjacent : adjacents)
		for(auto& [fluidType, pair] : adjacent->m_fluids)
			if(fluidType != m_fluidType)
				m_futureNotifyUnfull[pair.second].insert(adjacent);
	// Discard the largest future group (it continues to exist as this group).
	m_futureGroups.pop_back();
}

void FluidGroup::writeStep()
{
	// Set new block volumes.
	if(m_stable or m_destroy)
		return;
	// Record merge. Both groups must have recorded merge with each other at the end of their step to avoid merging with a group which is no longer where it was.
	for(FluidGroup* fluidGroup : m_futureMerge)
		if(fluidGroup->m_futureMerge.contains(this))
		{
			absorb(fluidGroup);
			// Mark as destroyed rather then destroying right away so as not to interfear with iteration.
			fluidGroup->m_destroy = true;
		}
	// record new fluid levels
	for(auto& iter = m_drainGroupBegin; iter != m_drainGroupEnd; ++iter)
	{
		iter->block->m_fluids[m_fluidType].first += iter->delta;
		iter->block->m_totalFluidVolume += iter->delta;
	}
	for(auto& iter = m_fillGroupBegin; iter != m_fillGroupEnd; ++iter)
	{
		iter->block->m_fluids[m_fluidType].first += iter->delta;
		iter->block->m_totalFluidVolume += iter->delta;
	}
	// Split new groups created by flow.
	for(std::unordered_set<Block*> futureGroup : m_futureGroups)
	{
		m_drainQueue.front().block->m_area->createFluidGroup(m_fluidType, futureGroup);
		removeBlocksInternal(futureGroup);
	}
	//TODO: std::move m_futureBlocks to m_blocks.
	// Record added blocks.
	for(Block* block : m_futureNewlyAdded)
		addBlockInternal(block);
	//TODO: Assert that futures aren't contradictory.
	// Record removed blocks.
	removeBlocksInternal(m_futureEmpty);
	// Record removed empty adjacent and futureFull.
	std::erase_if(m_fillQueue, [&](FutureFluidBlock& futureFluidBlock){ return m_futureRemoveFromEmptyAdjacents.contains(futureFluidBlock.block) || m_futureFull.contains(futureFluidBlock.block); });
	// Record added unfull blocks.
	for(Block* block : m_futureUnfull)
		m_fillQueue.emplace_back(block, 0, 0);
	// Record added empty adjacent.
	for(Block* block : m_futureEmptyAdjacents)
		m_fillQueue.emplace_back(block, 0, 0);
	// Record newly unfull blocks for adjacent fluid groups of another type.
	for(auto& [fluidGroup, blocks] : m_futureNotifyUnfull)
		for(Block* block : blocks)
			if(block->fluidCanEnterCurrently(fluidGroup->m_fluidType))
				fluidGroup->m_fillQueue.emplace_back(block, 0, 0);
}
