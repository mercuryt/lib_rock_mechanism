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
FluidGroup::FluidGroup(const FluidType* ft, std::unordered_set<Block*>& blocks, Area* area, bool checkMerge) :
	m_stable(false), m_destroy(false), m_merged(false), m_disolved(false), m_fluidType(ft), m_excessVolume(0),
	m_fillQueue(this), m_drainQueue(this), m_area(area)
{
	for(Block* block : blocks)
		addBlock(block, checkMerge);
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
	if(block->m_fluids.contains(m_fluidType) && block->m_fluids.at(m_fluidType).first >= MAX_BLOCK_VOLUME)
		m_fillQueue.removeBlock(block);
	else
		m_fillQueue.addBlock(block);
	block->m_fluids[m_fluidType].second = this;
	// Add adjacent if fluid can enter.
	std::unordered_set<FluidGroup*> toMerge;
	for(Block* adjacent : block->m_adjacents)
	{
		if(adjacent == nullptr || !adjacent->fluidCanEnterEver() || !adjacent->fluidCanEnterEver(m_fluidType) ||
				(adjacent->m_fluids.contains(m_fluidType) && adjacent->m_fluids.at(m_fluidType).second == this)
		  )
			continue;
		if(!adjacent->m_fluids.contains(m_fluidType) || adjacent->m_fluids.at(m_fluidType).first < MAX_BLOCK_VOLUME)
			m_fillQueue.addBlock(adjacent);
		// Merge groups if needed.
		if(checkMerge && adjacent->m_fluids.contains(m_fluidType))
			toMerge.insert(adjacent->getFluidGroup(m_fluidType));
	}
	for(FluidGroup* oldGroup : toMerge)
		merge(oldGroup);
}
void FluidGroup::removeBlock(Block* block)
{
	m_stable = false;
	m_drainQueue.removeBlock(block);
	//Check for empty adjacent to remove.
	m_potentiallyNoLongerAdjacentFromSyncronusStep.insert(block);
	//Check for group split.
	m_potentiallySplitFromSyncronusStep.insert(block);
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
	// If there is no where to flow into there is nothing to do.
	if(m_fillQueue.m_set.empty() && m_excessVolume >= 0 && m_disolvedInThisGroup.empty())
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
		assert(drainLevel != 0);
		// TODO: fill level does not take into account other fluids of greater or equal density.
		uint32_t fillLevel = m_fillQueue.groupLevel();
		assert(fillLevel < MAX_BLOCK_VOLUME);
		//assert(m_drainQueue.m_groupStart->block->m_z > m_fillQueue.m_groupStart->block->m_z || drainLevel >= fillLevel);
		uint32_t drainZ = m_drainQueue.m_groupStart->block->m_z;
		uint32_t fillZ = m_fillQueue.m_groupStart->block->m_z;
		// If drain is less then 2 units above fill then end loop.
		//TODO: using fillLevel > drainLevel here rather then the above assert feels like a hack.
		if(drainZ < fillZ || (drainZ == fillZ && (fillLevel > drainLevel || drainLevel - fillLevel < 2)))
		{
			// if no new blocks have been added this step then set stable
			if(m_fillQueue.m_futureNoLongerEmpty.empty() && m_disolvedInThisGroup.empty())
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
	// -Find future blocks for futureEmptyAdjacent.
	for(Block* block : m_fillQueue.m_futureNoLongerEmpty)
		for(Block* adjacent : block->m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver() && 
				adjacent->fluidCanEnterEver(m_fluidType) && !m_drainQueue.m_set.contains(adjacent) && 
				!m_fillQueue.m_set.contains(adjacent) &&
				(!adjacent->m_fluids.contains(m_fluidType) || adjacent->m_fluids.at(m_fluidType).first < MAX_BLOCK_VOLUME || adjacent->m_fluids.at(m_fluidType).second != this)
			  )
				m_futureNewEmptyAdjacents.insert(adjacent);
	// -Find any potental newly created groups.
	// Collect blocks adjacent to newly empty which are not empty.
	// Also collect possiblyNoLongerAdjacent.
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
	// If there is only one potential new group there cannot be a split: there needs to be another group to split from.
	if(potentialNewGroups.size() > 1)
	{
		for(Block* block : potentialNewGroups)
		{
			if(closed.contains(block))
				continue;
			auto condition = [&](Block* block){ return futureBlocks.contains(block); };
			std::unordered_set<Block*> adjacents = util::collectAdjacentsWithCondition(condition, block);
			// Add whole group to closed. There is only one iteration per group as others will be rejected by the closed guard.
			closed.insert(adjacents.begin(), adjacents.end());
			m_futureGroups.emplace_back(adjacents);
		}
	}
	if(!m_futureGroups.empty())
	{
		// Sort by size.
		auto condition = [](FluidGroupSplitData& a, FluidGroupSplitData& b){ return a.members.size() < b.members.size(); };
		std::ranges::sort(m_futureGroups, condition);
		/*
		auto it = std::ranges::max_element(m_futureGroups, condition);
		std::rotate(it, it + 1, m_futureGroups.end());
		*/
		// Record each group's future new adjacent.
		for(Block* block : m_futureNewEmptyAdjacents)
			for(Block* adjacent : block->m_adjacents)
				if(adjacent != nullptr && adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(m_fluidType))
				{
					for(FluidGroupSplitData& fluidGroupSplitData : m_futureGroups)
						if(fluidGroupSplitData.members.contains(adjacent))
							fluidGroupSplitData.futureAdjacent.insert(block);
				}
	}
	// -Find no longer adjacent empty to remove from fill queue and newEmptyAdjacent.
	std::unordered_set<Block*> futureRemoveFromEmptyAdjacents;
	for(Block* block : possiblyNoLongerAdjacent)
	{
		bool stillAdjacent = false;
		for(Block* adjacent : block->m_adjacents)
		{
			if(adjacent != nullptr && adjacent->fluidCanEnterEver() && adjacent->fluidCanEnterEver(m_fluidType) && 
					futureBlocks.contains(adjacent))
			{
				stillAdjacent = true;
				break;
			}
		}
		if(!stillAdjacent)
			futureRemoveFromEmptyAdjacents.insert(block);
	}
	// Convert descriptive future to proscriptive future.
	m_futureAddToDrainQueue = m_fillQueue.m_futureNoLongerEmpty;
	m_futureRemoveFromDrainQueue = m_drainQueue.m_futureEmpty;
	//m_futureAddToFillQueue = (m_futureNoLongerFull - futureRemoveFromEmptyAdjacents) + m_futureNewEmptyAdjacents;
	m_futureAddToFillQueue = m_futureNewEmptyAdjacents;
	for(Block* block : m_drainQueue.m_futureNoLongerFull)
		if(!futureRemoveFromEmptyAdjacents.contains(block))
			m_futureAddToFillQueue.insert(block);
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
	// Resolve overfull blocks.
	for(Block* block : m_fillQueue.m_overfull)
		if(block->m_totalFluidVolume > MAX_BLOCK_VOLUME)
			block->resolveFluidOverfull();
}
void FluidGroup::splitStep(std::vector<FluidGroup*>& newlySplit)
{
	assert(!m_merged);
	assert(!m_disolved);
	assert(!m_destroy);
	// Disperse disolved, split off fluidGroups of another fluidType.
	// TODO: Transfer ownership to adjacent fluid groups with a lower density then this one but higher then disolved group.
	std::vector<const FluidType*> dispersed;
	for(auto& [fluidType, fluidGroup] : m_disolvedInThisGroup)
	{
		assert(fluidGroup->m_drainQueue.m_set.empty());
		for(FutureFlowBlock& futureFlowBlock : m_fillQueue.m_queue)
		{
			Block* block = futureFlowBlock.block;
			if(block->fluidCanEnterEver(fluidType) && block->fluidCanEnterCurrently(fluidType))
			{
				uint32_t capacity = block->volumeOfFluidTypeCanEnter(fluidType);
				assert(fluidGroup->m_excessVolume > 0);
				uint32_t flow = std::min(capacity, (uint32_t)fluidGroup->m_excessVolume);
				block->m_totalFluidVolume += flow;
				fluidGroup->m_excessVolume -= flow;
				if(block->m_fluids.contains(fluidType))
				{
					block->m_fluids.at(fluidType).first += flow;
					block->m_fluids.at(fluidType).second->m_excessVolume += fluidGroup->m_excessVolume;
					fluidGroup->m_destroy = true;
				}
				else
				{
					block->m_fluids.emplace(fluidType, std::make_pair(flow, fluidGroup));
					fluidGroup->addBlock(block);
					fluidGroup->m_disolved = false;
				}
				dispersed.push_back(fluidType);
				break;
			}
		}
	}
	for(const FluidType* fluidType : dispersed)
		m_disolvedInThisGroup.erase(fluidType);
	// Split off future groups of this fluidType.
	if(m_futureGroups.empty())
		return;
	std::unordered_set<Block*> formerMembers;
	for(Block* block : m_drainQueue.m_set)
		if(!m_futureGroups.back().members.contains(block))
			formerMembers.insert(block);
	m_drainQueue.removeBlocks(formerMembers);
	std::unordered_set<Block*> formerFill;
	for(Block* block : m_fillQueue.m_set)
		if(!m_drainQueue.m_set.contains(block) && !block->isAdjacentToAny(m_drainQueue.m_set))
			formerFill.insert(block);
	m_fillQueue.removeBlocks(formerFill);
	m_futureNewEmptyAdjacents = m_futureGroups.back().futureAdjacent;
	// Remove largest group, it will remain as this instance.
	m_futureGroups.pop_back();
	for(auto& [members, newAdjacent] : m_futureGroups)
	{
		newlySplit.push_back(m_area->createFluidGroup(m_fluidType, members, false));
		newlySplit.back()->m_futureNewEmptyAdjacents.swap(newAdjacent);
	}
}
// Do all split prior to doing merge.
void FluidGroup::mergeStep()
{
	assert(!m_disolved);
	assert(!m_destroy);
	// Fluid groups may be marked merged during merge iteration.
	if(m_merged)
		return;
	// Record merge. First group consumes subsequent groups.
	for(Block* block : m_futureNewEmptyAdjacents)
	{
		if(!block->m_fluids.contains(m_fluidType))
			continue;
		FluidGroup* fluidGroup = block->m_fluids.at(m_fluidType).second;
		if(fluidGroup != this)
		{
			merge(fluidGroup);
			continue;
		}
	}
}
