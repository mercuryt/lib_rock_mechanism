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
void FluidGroup::setUnstable()
{
	m_stable = false;
	m_drainQueue.front().block->m_area->m_unstableFluidGroups.insert(this);
}
void FluidGroup::addFluid(uint32_t volume)
{
	m_excessVolume += volume;
	setUnstable();
}
void FluidGroup::removeFluid(uint32_t volume)
{
	m_excessVolume -= volume;
	setUnstable();
}
//TODO: Reduce repetitive checks for removal from fill queue by seperating this logic when used by normal flow or by addBlock.
void FluidGroup::addBlockInternal(Block* block)
{
	if(m_blocks.contains(block))
		return;
	m_blocks.insert(block);
	m_drainQueue.emplace_back(block);
	if(block->fluidCanEnterCurrently(m_fluidType))
		m_fillQueue.emplace_back(block);
	else
		std::erase_if(m_fillQueue, [&](FutureFluidBlock& futureFluidBlock){ return futureFluidBlock.block == block; });
	block->m_fluids[m_fluidType].second = this;
	m_emptyAdjacents.erase(block);
}
void FluidGroup::addBlock(Block* block, bool checkMerge)
{
	m_stable = false;
	addBlockInternal(block);
	// Add adjacent if fluid can enter.
	std::unordered_set<FluidGroup*> toMerge;
	for(Block* adjacent : block->m_adjacents)
	{
		if(adjacent == nullptr || !adjacent->fluidCanEnterEver() || !adjacent->fluidCanEnterEver(m_fluidType) || 
				(adjacent->m_fluids.contains(m_fluidType) && adjacent->m_fluids.at(m_fluidType).second == this)
		  )
			continue;
		if(adjacent->fluidCanEnterCurrently(m_fluidType))
		{
			m_fillQueue.emplace_back(adjacent);
			m_emptyAdjacents.insert(adjacent);
		}
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
	setUnstable();
	removeBlockInternal(block);
	//TODO: check for empty adjacent to remove.
	//TODO: check for group split.
}
void FluidGroup::removeBlockAdjacent(Block* block)
{
	m_emptyAdjacents.erase(block);
	std::erase_if(m_fillQueue, [&](FutureFluidBlock futureFluidBlock){ return futureFluidBlock.block == block; });
}

// To be run before applying async future data.
void FluidGroup::absorb(FluidGroup* fluidGroup)
{
	// Mark as absorbed rather then destroying right away so as not to interfear with iteration.
	fluidGroup->m_absorbed = true;
	// Add excess volume.
	m_excessVolume += fluidGroup->m_excessVolume;
	// Insert other group drain queue at this groups drain group end so the two drain groups are contiguous.
	uint32_t drainGroupCount = fluidGroup->m_drainGroupEnd - fluidGroup->m_drainGroupBegin;
	std::erase_if(fluidGroup->m_drainQueue, [&](FutureFluidBlock& futureFluidBlock){ return m_blocks.contains(futureFluidBlock.block); });
	m_drainQueue.insert(m_drainGroupEnd, fluidGroup->m_drainQueue.begin(), fluidGroup->m_drainQueue.end());
	m_drainGroupEnd += drainGroupCount;

	// And same for fill, but we might have overlaps here, discard them and add delta to excessVolume
	std::unordered_set<Block*> fillSet;
	for(FutureFluidBlock& futureFluidBlock : m_fillQueue)
	{
		fillSet.insert(futureFluidBlock.block);
		fluidGroup->m_futureNewEmptyAdjacents.erase(futureFluidBlock.block);
		fluidGroup->m_futureNewUnfullButNotEmpty.erase(futureFluidBlock.block);
	}
	uint32_t fillGroupCount = fluidGroup->m_fillGroupEnd - fluidGroup->m_fillGroupBegin;
	std::erase_if(fluidGroup->m_fillQueue, [&](FutureFluidBlock& futureFluidBlock){
		if(!fillSet.contains(futureFluidBlock.block))
		{
			m_futureNewEmptyAdjacents.erase(futureFluidBlock.block);
			m_futureNewUnfullButNotEmpty.erase(futureFluidBlock.block);
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
	m_fillQueue.insert(m_fillGroupEnd, fluidGroup->m_fillQueue.begin(), fluidGroup->m_fillQueue.end());
	m_fillGroupEnd += fillGroupCount;

	// Merge blocks.
	m_blocks.insert(fluidGroup->m_blocks.begin(), fluidGroup->m_blocks.end());
	// TODO: change from blocks to future blocks?
	//m_futureBlocks.insert(fluidGroup->m_futureBlocks.begin(), fluidGroup->m_futureBlocks.end());
	// Set fluidGroup for merged blocks.
	for(Block* block : fluidGroup->m_blocks)
		block->m_fluids.at(m_fluidType).second = this;
	// Remove future blocks from other group's empty adjacents.
	std::erase_if(fluidGroup->m_emptyAdjacents, [&](Block* block){ return m_futureBlocks.contains(block); });
	// Merge empty adjacents.
	m_emptyAdjacents.insert(fluidGroup->m_emptyAdjacents.begin(), fluidGroup->m_emptyAdjacents.end());
	// Merge future empty.
	m_futureEmpty.insert(fluidGroup->m_futureEmpty.begin(), fluidGroup->m_futureEmpty.end());
	// Remove this group's futureEmptyAdjacents from other groups future unfull. Empty blocks don't belong in unfull.
	std::erase_if(fluidGroup->m_futureNewUnfullButNotEmpty, [&](Block* block){ return m_futureNewEmptyAdjacents.contains(block); });
	// Merge future unfull.
	m_futureNewUnfullButNotEmpty.insert(fluidGroup->m_futureNewUnfullButNotEmpty.begin(), fluidGroup->m_futureNewUnfullButNotEmpty.end());
	// Remove this group's future blocks from other group's future empty adajcent.
	std::erase_if(fluidGroup->m_futureNewEmptyAdjacents, [&](Block* block){ return m_futureBlocks.contains(block); });
	// Merge future empty adjacent.
	m_futureNewEmptyAdjacents.insert(fluidGroup->m_futureNewEmptyAdjacents.begin(), fluidGroup->m_futureNewEmptyAdjacents.end());
	// Remove this group's future newly adjacent from other group's future remove from adjacent.
	std::erase_if(fluidGroup->m_futureRemoveFromEmptyAdjacents, [&](Block* block){ return m_futureNewEmptyAdjacents.contains(block); });
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
	// Merge other groups ment to merge with fluidGroup with this group.
	for(FluidGroup* otherFG : fluidGroup->m_futureMerge)
		if(otherFG != this && otherFG->m_futureMerge.contains(fluidGroup))
			absorb(otherFG);
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
	// Record future capacity and delta.
	for(auto iter = m_drainGroupBegin; iter != m_drainGroupEnd; ++iter)
	{
		iter->capacity -= flowPerBlock;
		iter->delta -= flowPerBlock;
	}
	// If the first drain block did not have fill capacity before this step then add this group to newlyUnfull.
	if(!m_drainGroupBegin->block->fluidCanEnterCurrently(m_fluidType))
		for(auto iter = m_drainGroupBegin; iter != m_drainGroupEnd; ++iter)
			m_futureNewUnfullButNotEmpty.insert(iter->block);
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
	m_futureFull.clear();
	m_futureNewlyAdded.clear();
	m_futureNewUnfullButNotEmpty.clear();
	m_futureEmpty.clear();
	m_futureNewEmptyAdjacents.clear();
	m_futureGroups.clear();
	m_futureMerge.clear();
	m_futureNotifyUnfull.clear();
	m_futureRemoveFromEmptyAdjacents.clear();
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
		// If drain is less then 2 units above fill then end loop.
		if(m_drainGroupBegin->block->m_z < m_fillGroupBegin->block->m_z || 
				(m_drainGroupBegin->block->m_z == m_fillGroupBegin->block->m_z &&
				 m_drainGroupBegin->capacity - fillLevel < 2)
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
	std::unordered_set<Block*> possiblyNoLongerAdjacent;
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
				adjacent->fluidCanEnterEver(m_fluidType) && !m_futureBlocks.contains(adjacent)
			  )
				m_futureNewEmptyAdjacents.insert(adjacent);
	// -update empty adjacent
	std::erase_if(m_emptyAdjacents, [&](Block* block){ 
			return m_futureNewlyAdded.contains(block) || m_futureRemoveFromEmptyAdjacents.contains(block); });
	m_emptyAdjacents.insert(m_futureNewEmptyAdjacents.begin(), m_futureNewEmptyAdjacents.end());
	// -Find potential merge targets.
	for(Block* block : m_emptyAdjacents)
		// Block is adjacent to member block and contains the same fluid, potentialy merge groups.
		// Keep block in futureEmptyAdjacent, in case the merge doesn't happen due to the other group no longer being present in block.
		if(block->m_fluids.contains(m_fluidType) && block->getFluidGroup(m_fluidType) != this)
			m_futureMerge.insert(block->getFluidGroup(m_fluidType));
	// -Find blocks adjacent to m_futureNewUnfullButNotEmpty to notify adjacent groups of different fluid type.
	std::unordered_set<Block*> adjacents;
	for(Block* block : m_futureNewUnfullButNotEmpty)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver()) // note: not sending m_fluidType here because we want to know if any fluid can enter.
				adjacents.insert(adjacent);
	// Record FluidGroups of adjacent blocks with another type of fluid.
	for(Block* block : adjacents)
		for(auto& [fluidType, pair] : block->m_fluids)
			if(fluidType != m_fluidType)
				for(Block* adjacent : block->m_adjacents)
					// Add all adjacents which are members of this group to notify.
					if(adjacent != nullptr && adjacent->m_fluids.contains(m_fluidType) && adjacent->m_fluids[m_fluidType].second == this)
						m_futureNotifyUnfull[pair.second].insert(adjacent);
	// -if there are any groups remove the largest one, it will continue to exist as this FluidGroup object.
	if(!m_futureGroups.empty())
		m_futureGroups.pop_back();
}

void FluidGroup::writeStep()
{
	// Fluid groups may be marked absorbed during write iteration due to merge.
	if(m_absorbed)
		return;
	// Record merge. Both groups must have recorded merge with each other at the end of their step to avoid merging with a group which is no longer where it was. First group absorbs subsequent groups.
	for(FluidGroup* fluidGroup : m_futureMerge)
		if(fluidGroup->m_futureMerge.contains(this))
			//TODO: Does absorb handle the merge groups of absorbed FGs?
			absorb(fluidGroup);
	// Record new fluid levels for drain group.
	for(auto iter = m_drainQueue.begin(); iter != m_drainGroupEnd; ++iter)
	{
		// TODO: prevent iterating blocks with no delta, rewind drain group end in read step?
		if(iter->delta == 0)
			break;
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
			break;
		iter->block->m_fluids[m_fluidType].first += iter->delta;
		iter->block->m_fluids[m_fluidType].second = this;
		iter->block->m_totalFluidVolume += iter->delta;
		if(iter->block->m_totalFluidVolume > MAX_BLOCK_VOLUME)
			overfullBlocks.insert(iter->block);
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
	{
		m_blocks.insert(block);
		m_drainQueue.emplace_back(block);
	}
	//TODO: Assert that futures aren't contradictory.
	// Record removed blocks.
	removeBlocksInternal(m_futureEmpty);
	// Record removed empty adjacent and futureFull.
	std::erase_if(m_fillQueue, [&](FutureFluidBlock& futureFluidBlock){ return m_futureRemoveFromEmptyAdjacents.contains(futureFluidBlock.block) || m_futureFull.contains(futureFluidBlock.block); });
	// Record added unfull blocks.
	for(Block* block : m_futureNewUnfullButNotEmpty)
		m_fillQueue.emplace_back(block);
	// Record added empty adjacent.
	for(Block* block : m_futureNewEmptyAdjacents)
		m_fillQueue.emplace_back(block);
	// Record newly unfull blocks for adjacent fluid groups of another type.
	for(auto& [fluidGroup, blocks] : m_futureNotifyUnfull)
		for(Block* block : blocks)
			if(!(block->m_fluids.contains(fluidGroup->m_fluidType) && block->getFluidGroup(fluidGroup->m_fluidType) == fluidGroup) && block->fluidCanEnterCurrently(fluidGroup->m_fluidType))
			{
				fluidGroup->m_fillQueue.emplace_back(block);
				fluidGroup->m_emptyAdjacents.insert(block);
			}
	// Resolve overfull blocks.
	for(Block* block : overfullBlocks)
		block->resolveFluidOverfull();
}
