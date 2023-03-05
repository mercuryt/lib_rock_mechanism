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

//TODO: reuse blocks as m_fillQueue.m_set.
FluidGroup::FluidGroup(const FluidType* ft, std::unordered_set<Block*> blocks):
	m_stable(false), m_destroy(false), m_merged(false), m_fluidType(ft), m_excessVolume(0),
	m_fillQueue(this), m_drainQueue(this)
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
	if(m_drainQueue.m_set.contains(block))
		return;
	m_stable = false;
	m_drainQueue.addBlock(block);
	if(!block->fluidCanEnterCurrently(m_fluidType))
		m_fillQueue.removeBlock(block);
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
			m_fillQueue.addBlock(adjacent);
		// Merge groups if needed.
		if(checkMerge && adjacent->m_fluids.contains(m_fluidType))
			toMerge.insert(adjacent->getFluidGroup(m_fluidType));
	}
	for(FluidGroup* oldGroup : toMerge)
	{
		merge(oldGroup);
		delete oldGroup;
	}
}
void FluidGroup::removeBlock(Block* block)
{
	m_stable = false;
	m_drainQueue.removeBlock(block);
	m_fillQueue.removeBlock(block);
	//Check for empty adjacent to remove.
	m_potentiallyNoLongerAdjacentFromSyncronusStep.insert(block);
	//Check for group split.
	m_potentiallySplitFromSyncronusStep.insert(block);
}
void FluidGroup::split(std::unordered_set<Block*>& blocks, std::vector<FluidGroup*>& newlySplit)
{
	m_drainQueue.removeBlocks(blocks);
	m_fillQueue.removeBlocks(blocks);
	// Remove no longer adjacent from m_fillQueue.
	std::unordered_set<Block*> adjacents;
	for(Block* block : blocks)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(m_fluidType) && 
				!(adjacent->m_fluids.contains(m_fluidType) && adjacent->m_fluids[m_fluidType].second == this)
			  )
				adjacents.insert(adjacent);
	std::unordered_set<Block*> noLongerAdjacent;
	for(Block* block : adjacents)
	{
		for(Block* adjacent : block->m_adjacents)
			if(adjacent->m_fluids.contains(m_fluidType) && adjacent->m_fluids[m_fluidType].second == this)
				continue;
		noLongerAdjacent.insert(block);
	}
	m_fillQueue.removeBlocks(noLongerAdjacent);
	//TODO: Opitimize by passing adjacents to create, merge adjacents with blocks instead of using noLongerAdjacent?
	newlySplit.push_back(m_drainQueue.m_queue.front().block->m_area->createFluidGroup(m_fluidType, blocks));
}
// To be run before applying async future data.
void FluidGroup::merge(FluidGroup* fluidGroup)
{
	// Mark as merged rather then destroying right away so as not to interfear with iteration.
	fluidGroup->m_merged = true;
	m_stable = false;
	// Add excess volume.
	m_excessVolume += fluidGroup->m_excessVolume;
	// Merge queues.
	m_drainQueue.merge(fluidGroup->m_drainQueue);
	m_fillQueue.merge(fluidGroup->m_fillQueue);

	// Set fluidGroup for merged blocks.
	for(Block* block : fluidGroup->m_drainQueue.m_set)
		block->m_fluids.at(m_fluidType).second = this;

	// Merge future groups.
	m_futureGroups.insert(m_futureGroups.end(), fluidGroup->m_futureGroups.begin(), fluidGroup->m_futureGroups.end());

	// Merge other fluid groups ment to merge with fluidGroup with this group instead.
	for(Block* block: fluidGroup->m_futureNewEmptyAdjacents)
	{
		if(!block->m_fluids.contains(m_fluidType))
			continue;
		FluidGroup* fluidGroup = block->m_fluids.at(m_fluidType).second;
		if(block->m_fluids.contains(m_fluidType) && fluidGroup != this)
		{
			merge(fluidGroup);
			continue;
		}
	}
}
void FluidGroup::readStep()
{
	assert(!m_merged);
	assert(!m_destroy);
	assert(!m_stable);
	assert(!m_disolved);
	assert(!m_drainQueue.m_set.empty());
	m_futureNewEmptyAdjacents.clear();
	m_futureGroups.clear();
	m_futureNotifyPotentialUnfullAdjacent.clear();
	// Potentially insert into fill queue.
	if(!m_potentiallyAddToFillQueueFromSyncronusStep.empty())
	{
		for(Block* block : m_potentiallyAddToFillQueueFromSyncronusStep)
			if(block->fluidCanEnterCurrently(m_fluidType))
				m_fillQueue.addBlock(block);
		m_potentiallyAddToFillQueueFromSyncronusStep.clear();
	}
	// If there is no where to flow into there is nothing to do.
	if(m_fillQueue.m_set.empty() && m_excessVolume >= 0)
	{
		m_stable = true;
		m_fillQueue.noChange();
		m_drainQueue.noChange();
		return;
	}
	m_drainQueue.initalizeForStep();
	m_fillQueue.initalizeForStep();
	int32_t viscosity = m_fluidType->viscosity;
	// Disperse m_excessVolume.
	while(m_excessVolume > 0 && m_fillQueue.m_groupStart != m_fillQueue.m_queue.end())
	{
		// If there isn't enough to spread evenly then do nothing.
		if((uint32_t)m_excessVolume < m_fillQueue.groupSize())
			break;
		// How much fluid is there space for total.
		uint32_t flowCapacity = m_fillQueue.groupCapacityPerBlock();
		// How much can be filled before the next low block(s).
		uint32_t flowTillNextStep = m_fillQueue.groupFlowTillNextStepPerBlock();
		uint32_t excessPerBlock = (uint32_t)(m_excessVolume / m_fillQueue.groupSize());
		uint32_t flowPerBlock = std::min({flowTillNextStep, flowCapacity, excessPerBlock});
		m_excessVolume -= flowPerBlock * m_fillQueue.groupSize();
		m_fillQueue.recordDelta(flowPerBlock);
	}
	while(m_excessVolume < 0 && m_drainQueue.m_groupStart != m_drainQueue.m_queue.end())
	{
		// If there isn't enough to spread evenly then do nothing.
		if((uint32_t)(-1 * m_excessVolume) < m_drainQueue.groupSize())
			break;
		// How much is avaliable to drain total.
		uint32_t flowCapacity = m_drainQueue.groupCapacityPerBlock();
		// How much can be drained before the next high block(s).
		uint32_t flowTillNextStep = m_drainQueue.groupFlowTillNextStepPerBlock();
		uint32_t excessPerBlock = (uint32_t)((-1 * m_excessVolume) / m_drainQueue.groupSize());
		uint32_t flowPerBlock = std::min({flowCapacity, flowTillNextStep, excessPerBlock});
		m_excessVolume += flowPerBlock * m_drainQueue.groupSize();
		m_drainQueue.recordDelta(flowPerBlock);
	}
	// Do primary flow.
	// If we have reached the end of either queue the loop ends.
	while(viscosity > 0 && m_drainQueue.m_groupStart != m_drainQueue.m_groupEnd && m_fillQueue.m_groupStart != m_fillQueue.m_groupEnd)
	{
		uint32_t drainLevel = m_drainQueue.groupLevel();
		// TODO: fill level does not take into account other fluids of greater or equal density.
		uint32_t fillLevel = m_fillQueue.groupLevel();
		//assert(m_drainQueue.m_groupStart->block->m_z > m_fillQueue.m_groupStart->block->m_z || drainLevel >= fillLevel);
		uint32_t drainZ = m_drainQueue.m_groupStart->block->m_z;
		uint32_t fillZ = m_fillQueue.m_groupStart->block->m_z;
		// If drain is less then 2 units above fill then end loop.
		//TODO: using fillLevel > drainLevel here rather then the above assert feels like a hack.
		if(drainZ < fillZ || (drainZ == fillZ && (fillLevel > drainLevel || drainLevel - fillLevel < 2)))
		{
			// if no new blocks have been added this step then set stable
			if(m_fillQueue.m_futureNoLongerEmpty.empty())
				m_stable = true;
			break;
		}
		// How much fluid is there space for total.
		uint32_t flowCapacityFill = m_fillQueue.groupCapacityPerBlock();
		// How much can be filled before the next low block(s).
		uint32_t flowTillNextStepFill = m_fillQueue.groupFlowTillNextStepPerBlock();
		// Viscosity is applied only to fill.
		uint32_t perBlockFill = std::min({flowCapacityFill, flowTillNextStepFill, (uint32_t)viscosity});
		// How much is avaliable to drain total.
		uint32_t flowCapacityDrain = m_drainQueue.groupCapacityPerBlock();
		// How much can be drained before the next high block(s).
		uint32_t flowTillNextStepDrain = m_drainQueue.groupFlowTillNextStepPerBlock();
		// How much can drain before equalization. Only applies if the groups are on the same z.
		uint32_t maxDrainForEqualibrium, maxFillForEquilibrium;
		if(m_fillQueue.m_groupStart->block->m_z == m_drainQueue.m_groupStart->block->m_z)
		{
			uint32_t totalLevel = (fillLevel * m_fillQueue.groupSize()) + (drainLevel * m_drainQueue.groupSize());
			uint32_t totalCount = m_fillQueue.groupSize() + m_drainQueue.groupSize();
			// We want to round down here so default truncation is fine.
			uint32_t equalibriumLevel = totalLevel / totalCount;
			maxDrainForEqualibrium = drainLevel - equalibriumLevel;
			maxFillForEquilibrium = equalibriumLevel - fillLevel;
		}
		else
			maxDrainForEqualibrium = UINT32_MAX;
		uint32_t perBlockDrain = std::min({flowCapacityDrain, flowTillNextStepDrain, maxDrainForEqualibrium});
		uint32_t totalFill = perBlockFill * m_fillQueue.groupSize();
		uint32_t totalDrain = perBlockDrain * m_drainQueue.groupSize();
		if(totalFill < totalDrain)
		{
			perBlockDrain = std::ceil(totalFill / (float)m_drainQueue.groupSize());
			totalDrain = perBlockDrain * m_drainQueue.groupSize();
		}
		else if(totalFill > totalDrain)
		{
			// If we are steping into equilibrium.
			if(maxDrainForEqualibrium == perBlockDrain)
				perBlockFill = maxFillForEquilibrium;
			else
				// truncate to round down
				perBlockFill = totalDrain / (float)m_fillQueue.groupSize();
			totalFill = perBlockFill * m_fillQueue.groupSize();
		}
		// Viscosity is consumed by flow.
		viscosity -= perBlockFill;
		// Record changes.
		m_drainQueue.recordDelta(perBlockDrain);
		m_fillQueue.recordDelta(perBlockFill);
		m_excessVolume += totalDrain - totalFill;
		// If we are at equilibrium then stop looping.
		// Don't mark stable because there may be newly added adjacent to flow into next tick.
		if(perBlockDrain == maxDrainForEqualibrium)
			break;
	}
	// Flow loops completed, analyze results.
	std::unordered_set<Block*> futureBlocks(m_drainQueue.m_set);
	std::erase_if(futureBlocks, [&](Block* block){ return m_drainQueue.m_futureEmpty.contains(block); });
	futureBlocks.insert(m_fillQueue.m_futureNoLongerEmpty.begin(), m_fillQueue.m_futureNoLongerEmpty.end());
	if(futureBlocks.empty())
		m_destroy = true;
		// We can't return here because we need to convert descriptive future into proscriptive future :(
	// -Find any potental newly created groups.
	// Collect blocks adjacent to newly empty which are not empty.
	std::unordered_set<Block*> potentialNewGroups;
	potentialNewGroups.swap(m_potentiallySplitFromSyncronusStep);
	std::unordered_set<Block*> possiblyNoLongerAdjacent;
	possiblyNoLongerAdjacent.swap(m_potentiallyNoLongerAdjacentFromSyncronusStep);
	// Collect all adjacent to futureEmpty which fluid can enter ever.
	std::unordered_set<Block*> adjacentToFutureEmpty;
	for(Block* block : m_drainQueue.m_futureEmpty)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(m_fluidType))
				adjacentToFutureEmpty.insert(adjacent);
	for(Block* block : adjacentToFutureEmpty)
		// If block won't be empty then check for forming a new group as it may be detached.
		if(futureBlocks.contains(block))
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
		auto condition = [&](Block* block){ return futureBlocks.contains(block); };
		std::unordered_set<Block*> adjacents = util::collectAdjacentsWithCondition(condition, block);
		// Add whole group to closed. There is only one iteration per group as others will be rejected by the closed guard.
		closed.insert(adjacents.begin(), adjacents.end());
		m_futureGroups.push_back(adjacents);
	}
	std::unordered_set<Block*>* largestGroup;
	if(m_futureGroups.empty())
		largestGroup = &futureBlocks;
	else
	{
		// Sort by size.
		auto condition = [](std::unordered_set<Block*> a, std::unordered_set<Block*> b){ return a.size() < b.size(); };
		std::ranges::sort(m_futureGroups, condition);
		largestGroup = &m_futureGroups.back();
	}
	// -Update futureNewEmptyAdjacents with futureEmpty.
	for(Block* block : m_drainQueue.m_futureEmpty)
		// If at least one block adjacent to future empty is still a member then add to empty adjacent.
		for(Block* adjacent : block->m_adjacents)
			if(largestGroup->contains(adjacent))
			{
				m_futureNewEmptyAdjacents.insert(block);
				break;
			}
	// -Find adjacent empty to remove from fill queue and emptyAdjacent.
	std::unordered_set<Block*> futureRemoveFromEmptyAdjacents;
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
			futureRemoveFromEmptyAdjacents.insert(block);
	}
	// -Find future blocks for futureEmptyAdjacent.
	for(Block* block : m_fillQueue.m_futureNoLongerEmpty)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver() && 
				adjacent->fluidCanEnterEver(m_fluidType) && !m_drainQueue.m_set.contains(adjacent)
			  )
				m_futureNewEmptyAdjacents.insert(adjacent);
	// -Find blocks adjacent to m_futureNewUnfull to notify adjacent groups of different fluid types.
	// If fluid type could not enter futureNewUnfull block then notify that it now can.
	// TODO: What if it can't due to another fluid flowing into the block at the same time? Will overfull resolve automatically?
	// What if the same block is inserted into a fill queue by multiple groups?
	// TODO: Make loops share bodies.
	// TODO: Can we merge newUnfullButNotEmpty with newEmpty? Maybe also with m_potentiallyAddToFillQueueFromSyncronusStep.
	for(Block* block : m_drainQueue.m_futureNoLongerFull)
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
	// Convert descriptive future to proscriptive future.
	m_futureAddToDrainQueue = m_fillQueue.m_futureNoLongerEmpty;
	m_futureRemoveFromDrainQueue = m_drainQueue.m_futureEmpty;
	//m_futureAddToFillQueue = (m_futureNewUnfull - futureRemoveFromEmptyAdjacents) + m_futureNewEmptyAdjacents;
	m_futureAddToFillQueue = m_futureNewEmptyAdjacents;
	for(Block* block : m_drainQueue.m_futureNoLongerFull)
	{
		// If still will have some volume.
		if(futureBlocks.contains(block))
			m_futureAddToFillQueue.insert(block);
		else
		{
			// If adjacent to a block which has some volume.
			bool stillAdjacent = false;
			for(Block* adjacent : block->m_adjacents)
				if(futureBlocks.contains(adjacent))
				{
					stillAdjacent = true;
					continue;
				}
			if(stillAdjacent)
				m_futureAddToFillQueue.insert(block);
		}
	}
	//m_futureRemoveFromFillQueue = futureRemoveFromEmptyAdjacents + m_futureFull;
	m_futureRemoveFromFillQueue = futureRemoveFromEmptyAdjacents;
	m_futureRemoveFromFillQueue.insert(m_fillQueue.m_futureFull.begin(), m_fillQueue.m_futureFull.end());
}
void FluidGroup::writeStep()
{
	assert(!m_merged);
	assert(!m_disolved);
	m_drainQueue.applyDelta();
	m_fillQueue.applyDelta();
	// Update queues.
#ifndef NDEBUG
	for(Block* block : m_futureAddToFillQueue)
		assert(!m_futureRemoveFromFillQueue.contains(block));
	for(Block* block : m_futureAddToDrainQueue)
		assert(!m_futureRemoveFromDrainQueue.contains(block));
#endif
	m_fillQueue.removeBlocks(m_futureRemoveFromFillQueue);
	m_fillQueue.addBlocks(m_futureAddToFillQueue);
	m_drainQueue.removeBlocks(m_futureRemoveFromDrainQueue);
	m_drainQueue.addBlocks(m_futureAddToDrainQueue);
	// Record potential newly unfull blocks for adjacent fluid groups of another type.
	for(auto& [fluidGroup, blocks] : m_futureNotifyPotentialUnfullAdjacent)
		fluidGroup->m_potentiallyAddToFillQueueFromSyncronusStep.insert(blocks.begin(), blocks.end());
	// Resolve overfull blocks.
	for(Block* block : m_fillQueue.m_overfull)
		block->resolveFluidOverfull();
	// Disperse disolved.
	// TODO: Transfer ownership to adjacent fluid groups with a lower density then this one but higher then disolved group.
	std::vector<FluidGroup*> dispersed;
	for(FluidGroup* fluidGroup : m_disolvedInThisGroup)
	{
		assert(fluidGroup->m_drainQueue.m_set.empty());
		for(FutureFlowBlock& futureFlowBlock : m_fillQueue.m_queue)
		{
			Block* block = futureFlowBlock.block;
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
// Do all merge prior to doing all split.
void FluidGroup::mergeStep()
{
	assert(!m_disolved);
	assert(!m_destroy);
	// Fluid groups may be marked merged during merge iteration.
	if(m_merged)
		return;
	// Record merge. First group consumes subsequent groups.
	for(Block* block: m_futureNewEmptyAdjacents)
	{
		if(!block->m_fluids.contains(m_fluidType))
			continue;
		FluidGroup* fluidGroup = block->m_fluids.at(m_fluidType).second;
		if(block->m_fluids.contains(m_fluidType) && fluidGroup != this)
		{
			merge(fluidGroup);
			continue;
		}
	}
}
void FluidGroup::splitStep(std::vector<FluidGroup*>& newlySplit)
{
	assert(!m_merged);
	assert(!m_disolved);
	assert(!m_destroy);
	// Split new groups created by flow.
	for(std::unordered_set<Block*>& futureGroup : m_futureGroups)
		split(futureGroup, newlySplit);
}
