/*
 * A set of blocks which are adjacent to eachother and contain the same fluid.
 * Intented to be run in series in a pool thread.
 * Maintains two priority queues:
 * 	m_blocksByZandTotalFluidHeight is all blocks which have fluid. It is sorted by high to low. This is the source of flow.
 * 	m_adjacentAndUnfullBlocksByInverseZandTotalFluidHeight is all blocks which have some fluid but aren't full, as well as blocks with no fluid that fluid can enter and are adjacent. It is sorted by low to high. This is the destination of flow.
 * The flow for the step is computed in readStep and applied in writeStep so that readStep can run concurrently with other read only tasks.
 */

#include <queue>
#include <cassert>

#include "util.h"

//TODO: reuse blocks as m_blocks.
FluidGroup::FluidGroup(const FluidType* ft, std::unordered_set<Block*> blocks):
	m_stable(false), m_destroy(false), m_absorbed(false), m_fluidType(ft), m_excessVolume(0)
{
	for(Block* block : blocks)
		addBlock(block);
}
void FluidGroup::addFluid(uint32_t volume)
{
	m_excessVolume += volume;
	m_stable = false;
}
void FluidGroup::removeFluid(uint32_t volume)
{
	m_excessVolume -= volume;
	m_stable = false;
}
void FluidGroup::addBlock(Block* block, bool checkMerge)
{
	if(m_blocks.contains(block))
		return;
	m_stable = false;
	m_blocks.insert(block);
	assert(!queueContains(m_drainQueue, block));
	m_drainQueue.emplace_back(block);
	if(block->fluidCanEnterCurrently(m_fluidType))
		m_potentiallyAddToFillQueueFromSyncronusStep.insert(block);
	else
		std::erase_if(m_fillQueue, [&](FutureFluidBlock& futureFluidBlock){ return futureFluidBlock.block == block; });
	block->m_fluids[m_fluidType].second = this;
	// Add adjacent if fluid can enter.
	std::unordered_set<FluidGroup*> toMerge;
	for(Block* adjacent : block->m_adjacents)
	{
		if(adjacent == nullptr || !adjacent->fluidCanEnterEver() || !adjacent->fluidCanEnterEver(m_fluidType) || 
				(adjacent->m_fluids.contains(m_fluidType) && adjacent->m_fluids.at(m_fluidType).second == this)
		  )
			continue;
		if(adjacent->fluidCanEnterCurrently(m_fluidType))
			m_potentiallyAddToFillQueueFromSyncronusStep.insert(adjacent);
		// Merge groups if needed.
		if(checkMerge && adjacent->m_fluids.contains(m_fluidType))
			toMerge.insert(adjacent->getFluidGroup(m_fluidType));
	}
	for(FluidGroup* oldGroup : toMerge)
	{
		absorb(oldGroup);
		delete oldGroup;
	}
}
void FluidGroup::split(std::unordered_set<Block*>& blocks)
{
	std::erase_if(m_blocks, [&](Block* block){ return blocks.contains(block); });
	std::erase_if(m_drainQueue, [&](FutureFluidBlock& futureFluidBlock){ return blocks.contains(futureFluidBlock.block); });
	std::unordered_set<Block*> adjacents;
	for(Block* block : blocks)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(m_fluidType) && 
					!blocks.contains(adjacent)
			  )
				adjacents.insert(adjacent);
	std::unordered_set<Block*> noLongerAdjacent;
	for(Block* block : adjacents)
	{
		for(Block* adjacent : block->m_adjacents)
			if(m_blocks.contains(adjacent))
				continue;
		noLongerAdjacent.insert(block);
	}
	std::erase_if(m_fillQueue, [&](FutureFluidBlock& futureFluidBlock){ return blocks.contains(futureFluidBlock.block) || noLongerAdjacent.contains(futureFluidBlock.block); });
	//TODO: Opitimize by passing adjacents to create, merge adjacents with blocks instead of using noLongerAdjacent?
	m_drainQueue.front().block->m_area->createFluidGroup(m_fluidType, blocks);
}
void FluidGroup::removeBlock(Block* block)
{
	m_stable = false;
	m_blocks.erase(block);
	auto ifBlock = [&](FutureFluidBlock& futureFluidBlock){ return futureFluidBlock.block == block; };
	std::erase_if(m_drainQueue, ifBlock);
	std::erase_if(m_fillQueue, ifBlock);
	//Check for empty adjacent to remove.
	m_potentiallyNoLongerAdjacentFromSyncronusStep.insert(block);
	//Check for group split.
	m_potentiallySplitFromSyncronusStep.insert(block);
}
void FluidGroup::removeBlockAdjacent(Block* block)
{
	std::erase_if(m_fillQueue, [&](FutureFluidBlock& futureFluidBlock){ return futureFluidBlock.block == block; });
}

// To be run before applying async future data.
void FluidGroup::absorb(FluidGroup* fluidGroup)
{
	// Mark as absorbed rather then destroying right away so as not to interfear with iteration.
	fluidGroup->m_absorbed = true;
	m_stable = false;
	// Add excess volume.
	m_excessVolume += fluidGroup->m_excessVolume;
	// Insert other group drain queue at this groups drain group end so the two drain groups are contiguous.
	uint32_t drainGroupCount = fluidGroup->m_drainGroupEnd - fluidGroup->m_drainQueue.begin();
	uint32_t drainGroupEndIndex = m_drainGroupEnd - m_drainQueue.begin();
	std::erase_if(fluidGroup->m_drainQueue, [&](FutureFluidBlock& futureFluidBlock){ return m_futureBlocks.contains(futureFluidBlock.block); });
	assert(m_drainQueue.end() >= m_drainGroupEnd);
	assert(m_drainQueue.begin() <= m_drainGroupEnd);
	m_drainQueue.insert(m_drainGroupEnd, fluidGroup->m_drainQueue.begin(), fluidGroup->m_drainQueue.end());
	// Recalculate end iterator invalidated by insert.
	m_drainGroupEnd = m_drainQueue.begin() + drainGroupEndIndex + drainGroupCount;
	assert(m_drainQueue.end() >= m_drainGroupEnd);

	// And same for fill, but we might have overlaps here, discard them and add delta to excessVolume
	std::unordered_set<Block*> fillSet;
	for(FutureFluidBlock& futureFluidBlock : m_fillQueue)
	{
		fillSet.insert(futureFluidBlock.block);
		fluidGroup->m_futureNewEmptyAdjacents.erase(futureFluidBlock.block);
		fluidGroup->m_futureNewUnfull.erase(futureFluidBlock.block);
	}
	uint32_t fillGroupCount = fluidGroup->m_fillGroupEnd - fluidGroup->m_fillQueue.begin();
	uint32_t fillGroupEndIndex = m_fillGroupEnd - m_fillQueue.begin();
	std::erase_if(fluidGroup->m_fillQueue, [&](FutureFluidBlock& futureFluidBlock){
		if(!fillSet.contains(futureFluidBlock.block))
		{
			m_futureNewEmptyAdjacents.erase(futureFluidBlock.block);
			m_futureNewUnfull.erase(futureFluidBlock.block);
			fillSet.insert(futureFluidBlock.block);
			return false;
		}
		if(&futureFluidBlock < &(*(fluidGroup->m_fillGroupEnd)))
		{
			--fillGroupCount;
			m_excessVolume += futureFluidBlock.delta;
		}
		return true;
	});
	assert(m_fillQueue.end() >= m_fillGroupEnd);
	assert(m_fillQueue.begin() <= m_fillGroupEnd);
	m_fillQueue.insert(m_fillGroupEnd, fluidGroup->m_fillQueue.begin(), fluidGroup->m_fillQueue.end());
	m_fillGroupEnd = m_fillQueue.begin() + fillGroupEndIndex + fillGroupCount;

	// Merge add to drain queue.
	m_futureAddToDrainQueue.insert(fluidGroup->m_futureAddToDrainQueue.begin(), fluidGroup->m_futureAddToDrainQueue.end());
	// Remove future blocks from remove from drain queue.
	std::erase_if(m_futureRemoveFromDrainQueue, [&](Block* block){ return fluidGroup->m_futureBlocks.contains(block); });
	std::erase_if(fluidGroup->m_futureRemoveFromDrainQueue, [&](Block* block){ return m_futureBlocks.contains(block); });
	// Merge remove from drain queue.
	m_futureRemoveFromDrainQueue.insert(fluidGroup->m_futureRemoveFromDrainQueue.begin(), fluidGroup->m_futureRemoveFromDrainQueue.end());
	auto willBeFull = [&](Block* block, FluidGroup* fluidGroup){ 
		return fluidGroup->m_futureFull.contains(block) || (block->fluidCanEnterCurrently(m_fluidType) == false && !fluidGroup->m_futureNewUnfull.contains(block));
	};
	// Remove full from addToFillQueue.
	std::erase_if(m_futureAddToFillQueue, [&](Block* block){ return willBeFull(block, fluidGroup); });
	std::erase_if(fluidGroup->m_futureAddToFillQueue, [&](Block* block){ return willBeFull(block, this); });
	// Merge add to fill queue.
	m_futureAddToFillQueue.insert(fluidGroup->m_futureAddToFillQueue.begin(), fluidGroup->m_futureAddToFillQueue.end());
	// Remove unfull blocks which are still adjacent to futureBlocks.
	std::erase_if(m_futureRemoveFromFillQueue, [&](Block* block){ 
		if(willBeFull(block, fluidGroup))
			return false;
		for(Block* adjacent : block->m_adjacents)
			if(fluidGroup->m_futureBlocks.contains(adjacent))
				return false;
		return true;		
	});
	std::erase_if(fluidGroup->m_futureRemoveFromFillQueue, [&](Block* block){ 
		if(willBeFull(block, this))
			return false;
		for(Block* adjacent : block->m_adjacents)
			if(m_futureBlocks.contains(adjacent))
				return false;
		return true;		
	});
	// Merge remove from fill queue.
	m_futureRemoveFromFillQueue.insert(fluidGroup->m_futureRemoveFromFillQueue.begin(), fluidGroup->m_futureRemoveFromFillQueue.end());

	// Merge blocks.
	m_blocks.insert(fluidGroup->m_blocks.begin(), fluidGroup->m_blocks.end());
	// TODO: change from blocks to future blocks?
	m_futureBlocks.insert(fluidGroup->m_futureBlocks.begin(), fluidGroup->m_futureBlocks.end());
	// Set fluidGroup for merged blocks.
	for(Block* block : fluidGroup->m_blocks)
		block->m_fluids.at(m_fluidType).second = this;

	// We need to merge future full and unfull in order to support recursive merge.
	// Merge future full.
	m_futureFull.insert(fluidGroup->m_futureFull.begin(), fluidGroup->m_futureFull.end());
	// Merge future unfull.
	m_futureNewUnfull.insert(fluidGroup->m_futureNewUnfull.begin(), fluidGroup->m_futureNewUnfull.end());

	// We need to merge future new empty adjacents for setting m_emptyAdjacentsAddedLastTurn at end of writeStep.
	std::erase_if(fluidGroup->m_futureRemoveFromEmptyAdjacents, [&](Block* block){ return m_futureBlocks.contains(block); });
	std::erase_if(m_futureRemoveFromEmptyAdjacents, [&](Block* block){ return fluidGroup->m_futureBlocks.contains(block); });
	m_futureNewEmptyAdjacents.insert(fluidGroup->m_futureNewEmptyAdjacents.begin(), fluidGroup->m_futureRemoveFromEmptyAdjacents.end());

	// Merge future groups.
	m_futureGroups.insert(m_futureGroups.end(), fluidGroup->m_futureGroups.begin(), fluidGroup->m_futureGroups.end());
	// Merge other fluid groups ment to merge with fluidGroup with this group instead.
	for(auto& [otherFG, blocks] : fluidGroup->m_futurePotentialMerge)
	{
		if(otherFG == this || otherFG->m_absorbed)
		       continue;
		for(Block* block : blocks)
			if(otherFG->m_futureBlocks.contains(block))
			{
				absorb(otherFG);
				continue;
			}
	}
}
// Syncronus. Only fluids with a lower density disolve.
// TODO: Transfer ownership to adjacent fluid groups with a lower density then this one but higher then disolved group.
void FluidGroup::disperseDisolved()
{
	std::vector<FluidGroup*> dispersed;
	for(FluidGroup* fluidGroup : m_disolvedInThisGroup)
	{
		assert(fluidGroup->m_blocks.empty());
		for(FutureFluidBlock& futureFluidBlock : m_fillQueue)
		{
			Block* block = futureFluidBlock.block;
			if(block->fluidCanEnterEver(fluidGroup->m_fluidType) && block->fluidCanEnterCurrently(fluidGroup->m_fluidType))
			{
				uint32_t capacity = block->volumeOfFluidTypeCanEnter(fluidGroup->m_fluidType);
				assert(fluidGroup->m_excessVolume > 0);
				uint32_t flow = std::min(capacity, (uint32_t)fluidGroup->m_excessVolume);
				block->addFluid(flow, fluidGroup->m_fluidType);
				fluidGroup->m_excessVolume -= flow;
				if(fluidGroup->m_excessVolume == 0)
				{
					dispersed.push_back(fluidGroup);
					fluidGroup->m_destroy = true;
					break;
				}
			}
		}
	}
	for(FluidGroup* fluidGroup : dispersed)
		m_disolvedInThisGroup.erase(fluidGroup);
}
void FluidGroup::fillGroupFindEnd()
{
	if(m_fillQueue.end() == m_fillGroupBegin)
	{
		m_fillGroupEnd = m_fillGroupBegin;
		return;
	}
	//TODO: use std::find_end?
	uint32_t priority = fillPriority(*m_fillGroupBegin);
	for(m_fillGroupEnd = m_fillGroupBegin + 1; m_fillGroupEnd != m_fillQueue.end(); ++m_fillGroupEnd)
		if(fillPriority(*m_fillGroupEnd) != priority)
			break;
}
void FluidGroup::drainGroupFindEnd()
{
	if(m_drainQueue.end() == m_drainGroupBegin)
	{
		m_drainGroupEnd = m_drainGroupBegin;
		return;
	}
	//TODO: use std::find_end?
	uint32_t priority = drainPriority(*m_drainGroupBegin);
	for(m_drainGroupEnd = m_drainGroupBegin + 1; m_drainGroupEnd != m_drainQueue.end(); ++m_drainGroupEnd)
		if(drainPriority(*m_drainGroupEnd) != priority)
			break;
}
void FluidGroup::recordFill(uint32_t flowPerBlock, uint32_t flowCapacity, uint32_t flowTillNextStep)
{
	assert(flowPerBlock != 0);
	assert(m_fillGroupEnd - m_fillGroupBegin > 0);
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
	if(flowPerBlock == flowCapacity)
	{
		for(auto iter = m_fillGroupBegin; iter != m_fillGroupEnd; ++iter)
			m_futureFull.insert(iter->block);
		m_fillGroupBegin = m_fillGroupEnd;
		if(m_fillGroupBegin != m_fillQueue.end())
			fillGroupFindEnd();
	}
	// If the blocks are not full but are now at the same level as some other blocks with the same m_z then extend the fill group to include these new blocks.
	else if(flowPerBlock == flowTillNextStep && m_fillGroupEnd != m_fillQueue.end())
		fillGroupFindEnd();
}
void FluidGroup::recordDrain(uint32_t flowPerBlock, uint32_t flowCapacity, uint32_t flowTillNextStep)
{
	assert(flowPerBlock != 0);
	assert(m_drainGroupEnd - m_drainGroupBegin > 0);
	// Record future capacity and delta.
	for(auto iter = m_drainGroupBegin; iter != m_drainGroupEnd; ++iter)
	{
		iter->capacity -= flowPerBlock;
		iter->delta -= flowPerBlock;
	}
	// If the first drain block did not have fill capacity before this step then add this group to newlyUnfull.
	if(!m_drainGroupBegin->block->fluidCanEnterCurrently(m_fluidType))
		for(auto iter = m_drainGroupBegin; iter != m_drainGroupEnd; ++iter)
			m_futureNewUnfull.insert(iter->block);
	// If the group is now empty then record it in futureEmpty and get the next one.
	if(flowCapacity == flowPerBlock)
	{
		for(auto iter = m_drainGroupBegin; iter != m_drainGroupEnd; ++iter)
			m_futureEmpty.insert(iter->block);
		m_drainGroupBegin = m_drainGroupEnd;
		if(m_drainGroupBegin != m_drainQueue.end())
			//TODO: pass starting point
			drainGroupFindEnd();
	}
	// If the blocks are not full but are now at the same level as some other blocks with the same m_z then extend the drain group to include these new blocks.
	else if(flowPerBlock == flowTillNextStep && m_drainGroupEnd != m_drainQueue.end())
		drainGroupFindEnd();
}
uint32_t FluidGroup::drainPriority(FutureFluidBlock& futureFluidBlock) const
{
	// Double to differentiate between one block at level x and full from one at level x+1 and empty.
	return (futureFluidBlock.block->m_z * MAX_BLOCK_VOLUME * 2) + futureFluidBlock.capacity;
}
uint32_t FluidGroup::fillPriority(FutureFluidBlock& futureFluidBlock) const
{
	// Add one to z to prevent negitive number.
	return ((futureFluidBlock.block->m_z + 1) * MAX_BLOCK_VOLUME * 2) - futureFluidBlock.capacity;
}

void FluidGroup::readStep()
{
	assert(!m_absorbed);
	assert(!m_destroy);
	assert(!m_stable);
	assert(!m_disolved);
	assert(!m_blocks.empty());
	assert(queueIsUnique(m_drainQueue));
	assert(queueIsUnique(m_fillQueue));
	assert(m_drainQueue.size() == m_blocks.size());
	m_futureFull.clear();
	m_futureNewlyAdded.clear();
	m_futureNewUnfull.clear();
	m_futureEmpty.clear();
	m_futureNewEmptyAdjacents.clear();
	m_futureGroups.clear();
	m_futurePotentialMerge.clear();
	m_futureNotifyPotentialUnfullAdjacent.clear();
	m_futureRemoveFromEmptyAdjacents.clear();
	// Potentially insert into fill queue.
	if(!m_potentiallyAddToFillQueueFromSyncronusStep.empty())
	{
		//TODO: Is it faster to maintain a persistant m_blocksInFillQueue?
		for(FutureFluidBlock& futureFluidBlock : m_fillQueue)
			m_potentiallyAddToFillQueueFromSyncronusStep.erase(futureFluidBlock.block);
		for(Block* block : m_potentiallyAddToFillQueueFromSyncronusStep)
			if(block->fluidCanEnterCurrently(m_fluidType))
			{
				assert(!queueContains(m_fillQueue, block));
				m_fillQueue.emplace_back(block);
			}
		m_potentiallyAddToFillQueueFromSyncronusStep.clear();
	}
	// If there is no where to flow into there is nothing to do.
	if(m_fillQueue.empty() && m_excessVolume >= 0)
	{
		m_stable = true;
		m_fillGroupEnd = m_fillQueue.begin();
		m_drainGroupEnd = m_drainQueue.begin();
		// Swap these now so write step will swap them back. ( questionable )
		m_futureBlocks.swap(m_blocks);
		return;
	}
#ifndef NDEBUG
	for(FutureFluidBlock& futureFluidBlock : m_drainQueue)
		assert(m_blocks.contains(futureFluidBlock.block));
#endif
	for(FutureFluidBlock& futureFluidBlock : m_drainQueue)
	{
		futureFluidBlock.capacity = futureFluidBlock.block->m_fluids[m_fluidType].first;
		futureFluidBlock.delta = 0;
	}
	auto isHigher = [&](FutureFluidBlock& a, FutureFluidBlock& b){ return drainPriority(a) > drainPriority(b); };
	std::ranges::sort(m_drainQueue, isHigher);
	m_drainGroupBegin = m_drainQueue.begin();
	drainGroupFindEnd();

	for(FutureFluidBlock& futureFluidBlock : m_fillQueue)
	{
		futureFluidBlock.capacity = futureFluidBlock.block->volumeOfFluidTypeCanEnter(m_fluidType);
		futureFluidBlock.delta = 0;
	}
	auto isLower = [&](FutureFluidBlock& a, FutureFluidBlock& b){ return fillPriority(a) < fillPriority(b); };
	std::ranges::sort(m_fillQueue, isLower);
	m_fillGroupBegin = m_fillQueue.begin();
	fillGroupFindEnd();

	int32_t viscosity = m_fluidType->viscosity;
	// Disperse m_excessVolume.
	while(m_excessVolume > 0 && m_fillGroupBegin != m_fillGroupEnd)
	{
		uint32_t fillGroupCount = m_fillGroupEnd - m_fillGroupBegin;
		// If there isn't enough to spread evenly then do nothing.
		if((uint32_t)m_excessVolume < fillGroupCount)
			break;
		// How much fluid is there space for total.
		uint32_t flowCapacity = m_fillGroupBegin->capacity;
		// How much can be filled before the next low block(s).
		uint32_t flowTillNextStep= (
				(m_fillGroupEnd == m_fillQueue.end() || m_fillGroupBegin->block->m_z != m_fillGroupEnd->block->m_z)?
				UINT32_MAX :
				(m_fillGroupEnd->capacity - m_fillGroupBegin->capacity)
		);
		uint32_t excessPerBlock = (uint32_t)(m_excessVolume / fillGroupCount);
		uint32_t flowPerBlock = std::min({flowTillNextStep, flowCapacity, excessPerBlock});
		m_excessVolume -= flowPerBlock * fillGroupCount;
		recordFill(flowPerBlock, flowCapacity, flowTillNextStep);
	}
	while(m_excessVolume < 0 && m_drainGroupBegin != m_drainGroupEnd)
	{
		uint32_t drainGroupCount = m_drainGroupEnd - m_drainGroupBegin;
		// If there isn't enough to spread evenly then do nothing.
		if((uint32_t)(-1 * m_excessVolume) < drainGroupCount)
			break;
		// How much is avaliable to drain total.
		uint32_t flowCapacity = m_drainGroupBegin->capacity;
		// How much can be drained before the next high block(s).
		uint32_t flowTillNextStep = (
			(m_drainGroupEnd == m_drainQueue.end() || m_drainGroupBegin->block->m_z != m_drainGroupEnd->block->m_z)?
			UINT32_MAX :
			(m_drainGroupBegin->capacity - m_drainGroupEnd->capacity)
		);
		uint32_t excessPerBlock = (uint32_t)((-1 * m_excessVolume) / drainGroupCount);
		uint32_t flowPerBlock = std::min({flowCapacity, flowTillNextStep, excessPerBlock});
		m_excessVolume += flowPerBlock * drainGroupCount;
		recordDrain(flowPerBlock, flowCapacity, flowTillNextStep);
	}
	// Do primary flow.
	// If we have reached the end of either queue the loop ends.
	while(viscosity > 0 && m_drainGroupBegin != m_drainGroupEnd && m_fillGroupBegin != m_fillGroupEnd)
	{
		uint32_t drainLevel = m_drainGroupBegin->capacity;
		// TODO: fill level does not take into account other fluids of greater or equal density.
		uint32_t fillLevel = m_fillGroupBegin->delta;
		if(m_fillGroupBegin->block->m_fluids.contains(m_fluidType))
			fillLevel += m_fillGroupBegin->block->m_fluids[m_fluidType].first;
		//assert(m_drainGroupBegin->block->m_z > m_fillGroupBegin->block->m_z || drainLevel >= fillLevel);
		// If drain is less then 2 units above fill then end loop.
		if(m_drainGroupBegin->block->m_z < m_fillGroupBegin->block->m_z || 
				(m_drainGroupBegin->block->m_z == m_fillGroupBegin->block->m_z &&
				 //TODO: using fillLevel > drainLevel here rather then the above assert feels like a hack.
				 (fillLevel > drainLevel || drainLevel - fillLevel < 2) 
				)
		  )
		{
			// if no new fill blocks have been added this step then set stable
			if(m_futureNewEmptyAdjacents.empty())
				m_stable = true;
			break;
		}
		uint32_t drainGroupCount = m_drainGroupEnd - m_drainGroupBegin;
		uint32_t fillGroupCount = m_fillGroupEnd - m_fillGroupBegin;
		// How much fluid is there space for total.
		uint32_t flowCapacityFill = m_fillGroupBegin->capacity;
		// How much can be filled before the next low block(s).
		uint32_t flowTillNextStepFill = (
				(m_fillGroupEnd == m_fillQueue.end() || m_fillGroupBegin->block->m_z != m_fillGroupEnd->block->m_z)?
				UINT32_MAX :
				(m_fillGroupEnd->capacity - m_fillGroupBegin->capacity)
		);
		// Viscosity is applied only to fill.
		uint32_t perBlockFill = std::min({flowCapacityFill, flowTillNextStepFill, (uint32_t)viscosity});
		// How much is avaliable to drain total.
		uint32_t flowCapacityDrain = m_drainGroupBegin->capacity;
		// How much can be drained before the next high block(s).
		uint32_t flowTillNextStepDrain = (
			(m_drainGroupEnd == m_drainQueue.end() || m_drainGroupBegin->block->m_z != m_drainGroupEnd->block->m_z)?
			UINT32_MAX :
			(m_drainGroupBegin->capacity - m_drainGroupEnd->capacity)
		);
		// How much can drain before equalization. Only applies if the groups are on the same z.
		uint32_t maxDrainForEqualibrium, maxFillForEquilibrium;
		if(m_fillGroupBegin->block->m_z == m_drainGroupBegin->block->m_z)
		{
			uint32_t totalLevel = (fillLevel * fillGroupCount) + (drainLevel * drainGroupCount);
			uint32_t totalCount = fillGroupCount + drainGroupCount;
			// We want to round down here so default truncation is fine.
			uint32_t equalibriumLevel = totalLevel / totalCount;
			maxDrainForEqualibrium = drainLevel - equalibriumLevel;
			maxFillForEquilibrium = equalibriumLevel - fillLevel;
		}
		else
			maxDrainForEqualibrium = UINT32_MAX;
		uint32_t perBlockDrain = std::min({flowCapacityDrain, flowTillNextStepDrain, maxDrainForEqualibrium});
		uint32_t totalFill = perBlockFill * fillGroupCount;
		uint32_t totalDrain = perBlockDrain * drainGroupCount;
		if(totalFill < totalDrain)
		{
			perBlockDrain = std::ceil(totalFill / (float)drainGroupCount);
			totalDrain = perBlockDrain * drainGroupCount;
		}
		else if(totalFill > totalDrain)
		{
			// If we are steping into equilibrium.
			if(maxDrainForEqualibrium == perBlockDrain)
				perBlockFill = maxFillForEquilibrium;
			else
				// truncate to round down
				perBlockFill = totalDrain / (float)fillGroupCount;
			totalFill = perBlockFill * fillGroupCount;
		}
		// Viscosity is consumed by flow.
		viscosity -= perBlockFill;
		// Record changes.
		recordDrain(perBlockDrain, flowCapacityDrain, flowTillNextStepDrain);
		recordFill(perBlockFill, flowCapacityFill, flowTillNextStepFill);
		m_excessVolume += totalDrain - totalFill;
		// If we are at equilibrium then stop looping.
		// Don't mark stable because there may be newly added adjacent to flow into next tick.
		if(perBlockDrain == maxDrainForEqualibrium)
			break;
	}
	// Flow loops completed, analyze results.
	// -Create new m_blocks;
	m_futureBlocks = m_blocks;
	std::erase_if(m_futureBlocks, [&](Block* block){ return m_futureEmpty.contains(block); });
	m_futureBlocks.insert(m_futureNewlyAdded.begin(), m_futureNewlyAdded.end());
	if(m_futureBlocks.empty())
	{
		// Mark to be destroyed by area.writeStep.
		m_destroy = true;
		return;
	}
	// -Find any newly created groups.
	// Collect blocks adjacent to newly empty which are not empty.
	std::unordered_set<Block*> potentialNewGroups;
	potentialNewGroups.swap(m_potentiallySplitFromSyncronusStep);
	std::unordered_set<Block*> possiblyNoLongerAdjacent;
	possiblyNoLongerAdjacent.swap(m_potentiallyNoLongerAdjacentFromSyncronusStep);
	// Collect all adjacent to futureEmpty which fluid can enter ever.
	std::unordered_set<Block*> adjacentToFutureEmpty;
	for(Block* block : m_futureEmpty)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(m_fluidType))
				adjacentToFutureEmpty.insert(adjacent);
	for(Block* block : adjacentToFutureEmpty)
		// If block won't be empty then check for forming a new group as it may be detached.
		if(m_futureBlocks.contains(block))
			potentialNewGroups.insert(block);
		// Else check for removal from empty adjacent queue.
		else
			possiblyNoLongerAdjacent.insert(block);
	// Seperate into contiguous groups. Each block in potentialNewGroups might be in a seperate group.
	std::unordered_set<Block*> closed;
	for(Block* block : potentialNewGroups)
	{
		if(closed.contains(block))
			continue;
		auto condition = [&](Block* block){ return m_futureBlocks.contains(block); };
		std::unordered_set<Block*> adjacents = util::collectAdjacentsWithCondition(condition, block);
		// Add whole group to closed. There is only one iteration per group as others will be rejected by the closed guard.
		closed.insert(adjacents.begin(), adjacents.end());
		m_futureGroups.push_back(adjacents);
	}
	std::unordered_set<Block*>* largestGroup;
	if(m_futureGroups.empty())
		largestGroup = &m_blocks;
	else
	{
		// Sort by size.
		auto condition = [](std::unordered_set<Block*> a, std::unordered_set<Block*> b){ return a.size() < b.size(); };
		std::ranges::sort(m_futureGroups, condition);
		largestGroup = &m_futureGroups.back();
	}
	// -Update futureNewEmptyAdjacents with futureEmpty.
	for(Block* block : m_futureEmpty)
		// If at least one block adjacent to future empty is still a member then add to empty adjacent.
		for(Block* adjacent : block->m_adjacents)
			if(adjacent->m_fluids.contains(m_fluidType) && adjacent->m_fluids[m_fluidType].second == this)
			{
				m_futureNewEmptyAdjacents.insert(block);
				break;
			}
	// -Find adjacent empty to remove from fill queue and emptyAdjacent.
	for(Block* block : possiblyNoLongerAdjacent)
	{
		bool stillAdjacent = false;
		for(Block* adjacent : block->m_adjacents)
		{
			if(adjacent == nullptr || !adjacent->fluidCanEnterEver())
				continue;
			// If adjacent to the largest group (the one that will continue to use this object).
			if(largestGroup->contains(adjacent))
			{
				stillAdjacent = true;
				break;
			}
		}
		if(!stillAdjacent)
			m_futureRemoveFromEmptyAdjacents.insert(block);
	}
	// -Find future blocks for futureEmptyAdjacent.
	for(Block* block : m_futureNewlyAdded)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver() && 
				adjacent->fluidCanEnterEver(m_fluidType) && !m_futureBlocks.contains(adjacent) && !m_blocks.contains(adjacent)
			  )
				m_futureNewEmptyAdjacents.insert(adjacent);
	// -Find potential merge targets.
	// TODO: Make loops share bodies.
	for(Block* block : m_futureNewEmptyAdjacents)
		// Block is adjacent to a future member block and contains the same fluid, potentialy merge groups.
		// Keep block in futureEmptyAdjacent, in case the merge doesn't happen due to the other group no longer being present in block.
		if(block->m_fluids.contains(m_fluidType))
		{
			FluidGroup* fluidGroup = block->getFluidGroup(m_fluidType);
			if(fluidGroup != this)
				m_futurePotentialMerge[fluidGroup].insert(block);
		}
	// Same for empty adjacents added last turn.
	// This fixes an edge case where two groups flow into previously empty adjacent blocks in the same step.
	for(Block* block : m_emptyAdjacentsAddedLastTurn)
		if(block->m_fluids.contains(m_fluidType) && !m_futureRemoveFromEmptyAdjacents.contains(block))
		{
			FluidGroup* fluidGroup = block->getFluidGroup(m_fluidType);
			if(fluidGroup != this)
				m_futurePotentialMerge[fluidGroup].insert(block);
		}
	
	// -Find blocks adjacent to m_futureNewUnfull to notify adjacent groups of different fluid types.
	// If fluid type could not enter futureNewUnfull block then notify that it now can.
	// TODO: What if it can't due to another fluid flowing into the block at the same time? Will overfull resolve automatically?
	// What if the same block is inserted into a fill queue by multiple groups?
	// TODO: Make loops share bodies.
	// TODO: Can we merge newUnfullButNotEmpty with newEmpty? Maybe also with m_potentiallyAddToFillQueueFromSyncronusStep.
	for(Block* block : m_futureNewUnfull)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver())
				for(auto& [fluidType, pair] : adjacent->m_fluids)
					if(fluidType != m_fluidType )
						m_futureNotifyPotentialUnfullAdjacent[pair.second].insert(block);
	for(Block* block : m_futureNewEmptyAdjacents)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver())
				for(auto& [fluidType, pair] : adjacent->m_fluids)
					if(fluidType != m_fluidType )
						m_futureNotifyPotentialUnfullAdjacent[pair.second].insert(block);
	// -if there are any groups remove the largest one, it will continue to exist as this FluidGroup object.
	if(!m_futureGroups.empty())
		m_futureGroups.pop_back();
	// Convert descriptive future to proscriptive future, does more work in async code and makes absorb simpler.
	m_futureAddToDrainQueue = m_futureNewlyAdded;
	m_futureRemoveFromDrainQueue = m_futureEmpty;
	//m_futureAddToFillQueue = (m_futureNewUnfull - m_futureRemoveFromEmptyAdjacents) + m_futureNewEmptyAdjacents;
	m_futureAddToFillQueue.clear();
	for(Block* block : m_futureNewEmptyAdjacents)
		if(!m_blocks.contains(block) && !m_futureBlocks.contains(block))
		{
			assert(!queueContains(m_fillQueue, block));
			m_futureAddToFillQueue.insert(block);
		}
	for(Block* block : m_futureNewUnfull)
		if(!m_futureRemoveFromEmptyAdjacents.contains(block))
		{
			assert(!queueContains(m_fillQueue, block));
			m_futureAddToFillQueue.insert(block);
		}
	//m_futureRemoveFromFillQueue = m_futureRemoveFromEmptyAdjacents + m_futureFull;
	m_futureRemoveFromFillQueue = m_futureRemoveFromEmptyAdjacents;
	m_futureRemoveFromFillQueue.insert(m_futureFull.begin(), m_futureFull.end());
}
void FluidGroup::mergeStep()
{
	// Fluid groups may be marked absorbed during merge iteration.
	if(m_absorbed)
		return;
	// Record merge. Check that at least one block still contains the group to be merged with. First group absorbs subsequent groups.
	for(auto& [fluidGroup, blocks] : m_futurePotentialMerge)
		for(Block* block : blocks)
			if(block->m_fluids.contains(m_fluidType) && block->m_fluids.at(m_fluidType).second == fluidGroup)
			{
				absorb(fluidGroup);
				continue;
			}
}
void FluidGroup::writeStep()
{
	assert(!m_absorbed);
	assert(!m_disolved);
	// Record new fluid levels for drain group.
	for(auto iter = m_drainQueue.begin(); iter != m_drainGroupEnd; ++iter)
	{
		// TODO: prevent iterating blocks with no delta, rewind drain group end in read step?
		if(iter->delta == 0)
			continue;
		assert(iter->delta < 0 && (-1 * iter->delta) <= MAX_BLOCK_VOLUME);
		assert(m_blocks.contains(iter->block));
		iter->block->m_fluids[m_fluidType].first += iter->delta;
		iter->block->m_totalFluidVolume += iter->delta;
		if(iter->block->m_fluids[m_fluidType].first == 0)
			iter->block->m_fluids.erase(m_fluidType);
	}
	std::unordered_set<Block*> overfullBlocks;
	// Record new fluid levels for fill group and note overfull.
	for(auto iter = m_fillQueue.begin(); iter != m_fillGroupEnd; ++iter)
	{
		if(iter->delta == 0)
			continue;
		assert(iter->delta > 0 && iter->delta <= MAX_BLOCK_VOLUME);
		assert(!iter->block->m_fluids.contains(m_fluidType) || m_blocks.contains(iter->block));
		iter->block->m_fluids[m_fluidType].first += iter->delta;
		iter->block->m_fluids[m_fluidType].second = this;
		iter->block->m_totalFluidVolume += iter->delta;
		if(iter->block->m_totalFluidVolume > MAX_BLOCK_VOLUME)
			overfullBlocks.insert(iter->block);
	}
	m_blocks.swap(m_futureBlocks);
	// Update queues.
#ifndef NDEBUG
	for(Block* block : m_futureAddToFillQueue)
		assert(!m_futureRemoveFromFillQueue.contains(block));
	for(Block* block : m_futureAddToDrainQueue)
		assert(!m_futureRemoveFromDrainQueue.contains(block));
#endif
	std::erase_if(m_fillQueue, [&](FutureFluidBlock& futureFluidBlock){ return m_futureRemoveFromFillQueue.contains(futureFluidBlock.block); });
	for(Block* block : m_futureAddToFillQueue)
	{
		assert(!queueContains(m_fillQueue, block));
		m_fillQueue.emplace_back(block);
	}
	std::erase_if(m_drainQueue, [&](FutureFluidBlock& futureFluidBlock){ return m_futureRemoveFromDrainQueue.contains(futureFluidBlock.block); });
	for(Block* block : m_futureAddToDrainQueue)
	{
		assert(!queueContains(m_drainQueue, block));
		m_drainQueue.emplace_back(block);
	}
	// Split new groups created by flow.
	for(std::unordered_set<Block*> futureGroup : m_futureGroups)
		split(futureGroup);
	// Record potential newly unfull blocks for adjacent fluid groups of another type.
	for(auto& [fluidGroup, blocks] : m_futureNotifyPotentialUnfullAdjacent)
		fluidGroup->m_potentiallyAddToFillQueueFromSyncronusStep.insert(blocks.begin(), blocks.end());
	// Resolve overfull blocks.
	for(Block* block : overfullBlocks)
		block->resolveFluidOverfull();
	m_emptyAdjacentsAddedLastTurn.swap(m_futureNewEmptyAdjacents);
	disperseDisolved();
}

bool queueIsUnique(std::vector<FutureFluidBlock>& queue)
{
	std::unordered_set<Block*> blocks;
	for(FutureFluidBlock& futureFluidBlock : queue)
	{
		if(blocks.contains(futureFluidBlock.block))
			return false;
		blocks.insert(futureFluidBlock.block);
	}
	return true;
}

bool queueContains(std::vector<FutureFluidBlock>& queue, Block* block)
{
	for(FutureFluidBlock& futureFluidBlock : queue)
		if(block == futureFluidBlock.block)
			return true;
	return false;
}
