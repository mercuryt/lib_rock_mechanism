/*
 * A set of blocks which are adjacent to eachother and contain the same fluid.
 * Intented to be run in series in a pool thread.
 * Maintains two priority queues:
 * 	m_blocksByZandTotalFluidHeight is all blocks which have fluid. It is sorted by high to low. This is the source of flow.
 * 	m_adjacentAndUnfullBlocksByInverseZandTotalFluidHeight is all blocks which have some fluid but aren't full, as well as blocks with no fluid that fluid can enter and are adjacent. It is sorted by low to high. This is the destination of flow.
 * The flow for the step is computed in readStep and applied in writeStep so that readStep can run concurrently with other read only tasks.
 */

#include "fluidGroup.h"
#include "util.h"
#include "config.h"
#include "block.h"
#include "area.h"

#include <queue>
#include <cassert>
#include <numeric>

//TODO: reuse blocks as m_fillQueue.m_set.
FluidGroup::FluidGroup(const FluidType& ft, std::unordered_set<Block*>& blocks, Area& area, bool checkMerge) :
	m_stable(false), m_destroy(false), m_merged(false), m_disolved(false), m_fluidType(ft), m_excessVolume(0),
	m_fillQueue(*this), m_drainQueue(*this), m_area(area)
{
	for(Block* block : blocks)
		if(block->m_fluids.contains(&m_fluidType))
			addBlock(*block, checkMerge);
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
void FluidGroup::addBlock(Block& block, bool checkMerge)
{
	assert(block.m_fluids.contains(&m_fluidType));
	if(m_drainQueue.m_set.contains(&block))
		return;
	setUnstable();
	auto found = block.m_fluids.find(&m_fluidType);
	if(found != block.m_fluids.end() && found->second.first >= Config::maxBlockVolume)
		m_fillQueue.removeBlock(&block);
	else
		m_fillQueue.addBlock(&block);
	found = block.m_fluids.find(&m_fluidType);
	if(found->second.second != nullptr && found->second.second != this)
		found->second.second->removeBlock(block);
	found->second.second = this;
	m_drainQueue.addBlock(&block);
	if constexpr(Config::fluidsSeepDiagonalModifier != 0)
		m_diagonalBlocks.erase(&block);
	// Add adjacent if fluid can enter.
	std::unordered_set<FluidGroup*> toMerge;
	for(Block* adjacent : block.m_adjacentsVector)
	{
		if(!adjacent->fluidCanEnterEver())
			continue;
		auto found = adjacent->m_fluids.find(&m_fluidType);
		if(found != adjacent->m_fluids.end())
		{
			assert(!found->second.second->m_merged);
			if(found->second.second == this)
				continue;
			// Merge groups if needed.
			if(checkMerge)
				toMerge.insert(found->second.second);
		}
		if(found == adjacent->m_fluids.end() || found->second.first < Config::maxBlockVolume)
			m_fillQueue.addBlock(adjacent);
	}
	for(FluidGroup* oldGroup : toMerge)
		merge(oldGroup);
	if constexpr (Config::fluidsSeepDiagonalModifier != 0)
		addDiagonalsFor(block);
	addMistFor(block);
}
void FluidGroup::removeBlock(Block& block)
{
	setUnstable();
	m_drainQueue.removeBlock(&block);
	m_potentiallyNoLongerAdjacentFromSyncronusStep.insert(&block);
	for(Block* adjacent : block.m_adjacentsVector)
		if(adjacent->fluidCanEnterEver())
		{
			//Check for group split.
			auto found = adjacent->m_fluids.find(&m_fluidType);
			if(found != adjacent->m_fluids.end() && found->second.second == this)
				m_potentiallySplitFromSyncronusStep.insert(adjacent);
			//Check for empty adjacent to remove.
			m_potentiallyNoLongerAdjacentFromSyncronusStep.insert(adjacent);
		}
	if constexpr (Config::fluidsSeepDiagonalModifier != 0)
	{
		for(Block* diagonal : block.getEdgeAndCornerAdjacentOnly())
			if(diagonal->fluidCanEnterEver() && m_diagonalBlocks.contains(diagonal))
			{
				//TODO: m_potentiallyNoLongerDiagonalFromSyncronusCode
				bool found = false;
				for(Block* doubleDiagonal : diagonal->getEdgeAndCornerAdjacentOnly())
					if(m_drainQueue.m_set.contains(doubleDiagonal))
					{
						found = true;
						continue;
					}
				if(!found)
					m_diagonalBlocks.erase(diagonal);
			}
	}
}
void FluidGroup::setUnstable()
{
	m_stable = false;
	m_area.m_unstableFluidGroups.insert(this);
}
void FluidGroup::addDiagonalsFor(Block& block)
{
	for(Block* diagonal : block.getEdgeAdjacentOnSameZLevelOnly())
		if(diagonal->fluidCanEnterEver() && !m_fillQueue.m_set.contains(diagonal) && 
				!m_drainQueue.m_set.contains(diagonal) && !m_diagonalBlocks.contains(diagonal))
		{
			// Check if there is a 'pressure reducing diagonal' created by two solid blocks.
			int32_t diffX = diagonal->m_x - block.m_x;
			int32_t diffY = diagonal->m_y - block.m_y;
			if(block.offset(diffX, 0, 0)->isSolid() && block.offset(0, diffY, 0)->isSolid())
				m_diagonalBlocks.insert(diagonal);
		}
}
void FluidGroup::addMistFor(Block& block)
{
	if(m_fluidType.mistDuration != 0 && (block.m_adjacents[0] == nullptr || !block.m_adjacents[0]->isSolid()))
		for(Block* adjacent : block.getAdjacentOnSameZLevelOnly())
			if(adjacent->fluidCanEnterEver())
				adjacent->spawnMist(m_fluidType);
}
void FluidGroup::merge(FluidGroup* smaller)
{
	assert(smaller != this);
	assert(smaller->m_fluidType == m_fluidType);
	assert(!smaller->m_merged);
	assert(!m_merged);
	assert(!smaller->m_disolved);
	assert(!m_disolved);
	assert(!smaller->m_destroy);
	assert(!m_destroy);

	FluidGroup* larger = this;
	if(smaller->m_drainQueue.m_set.size() > larger->m_drainQueue.m_set.size())
		std::swap(smaller, larger);
	// Mark as merged rather then destroying right away so as not to interfear with iteration.
	smaller->m_merged = true;
	larger->setUnstable();
	// Add excess volume.
	larger->m_excessVolume += smaller->m_excessVolume;
	// Merge queues.
	larger->m_drainQueue.merge(smaller->m_drainQueue);
	larger->m_fillQueue.merge(smaller->m_fillQueue);
	// Set fluidGroup for merged blocks.
	for(Block* block : smaller->m_drainQueue.m_set)
	{
		block->m_fluids.at(&m_fluidType).second = larger;
		assert(larger->m_drainQueue.m_set.contains(block));
	}
	// Merge diagonal seep if enabled.
	if constexpr (Config::fluidsSeepDiagonalModifier != 0)
	{
		std::erase_if(smaller->m_diagonalBlocks, [&](Block* block){
			return !larger->m_drainQueue.m_set.contains(block) && !larger->m_fillQueue.m_set.contains(block);
		});
		std::erase_if(larger->m_diagonalBlocks, [&](Block* block){
			return smaller->m_drainQueue.m_set.contains(block) || smaller->m_fillQueue.m_set.contains(block);
		});
		larger->m_diagonalBlocks.insert(smaller->m_diagonalBlocks.begin(), smaller->m_diagonalBlocks.end());
	}
	// Merge other fluid groups ment to merge with smaller with larger instead.
	for(Block* block: smaller->m_futureNewEmptyAdjacents)
	{
		auto found = block->m_fluids.find(&m_fluidType);
		if(found == block->m_fluids.end())
			continue;
		FluidGroup* fluidGroup = found->second.second;
		if(!fluidGroup->m_merged && fluidGroup != larger)
		{
			larger->merge(fluidGroup);
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
	m_futureNewEmptyAdjacents.clear();
	m_futureGroups.clear();
	m_futureNotifyPotentialUnfullAdjacent.clear();
	m_viscosity = m_fluidType.viscosity;
	m_drainQueue.initalizeForStep();
	m_fillQueue.initalizeForStep();
	validate();
	// If there is no where to flow into there is nothing to do.
	if(m_diagonalBlocks.empty() && (m_fillQueue.m_set.empty() || 
				((m_fillQueue.groupSize() == 0 || m_fillQueue.groupCapacityPerBlock() == 0) && m_excessVolume >= 0 )||
				((m_drainQueue.groupSize() == 0 || m_drainQueue.groupCapacityPerBlock() == 0) && m_excessVolume <= 0)
				)
	  )
	{
		m_stable = true;
		m_fillQueue.noChange();
		m_drainQueue.noChange();
		return;
	}
	// Disperse m_excessVolume.
	while(m_excessVolume > 0 && m_fillQueue.groupSize() != 0 && m_fillQueue.m_groupStart != m_fillQueue.m_queue.end())
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
		m_fillQueue.recordDelta(flowPerBlock, flowCapacity, flowTillNextStep);
	}
	while(m_excessVolume < 0 && m_drainQueue.groupSize() != 0 && m_drainQueue.m_groupStart != m_drainQueue.m_queue.end())
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
		m_drainQueue.recordDelta(flowPerBlock, flowCapacity, flowTillNextStep);
	}
	// Do primary flow.
	// If we have reached the end of either queue the loop ends.
	while(m_viscosity > 0 && m_drainQueue.groupSize() != 0 && m_fillQueue.groupSize() != 0)
	{
		assert(m_fillQueue.m_groupEnd == m_fillQueue.m_queue.end() || 
				m_fillQueue.m_groupStart->block->m_z != m_fillQueue.m_groupEnd->block->m_z ||
				m_fillQueue.m_groupStart->capacity != m_fillQueue.m_groupEnd->capacity);
		uint32_t drainVolume = m_drainQueue.groupLevel();
		assert(drainVolume != 0);
		uint32_t fillVolume = m_fillQueue.groupLevel();
		assert(fillVolume < Config::maxBlockVolume);
		//uint32_t fillInverseCapacity = Config::maxBlockVolume - m_fillQueue.m_groupStart->capacity;
		//assert(m_drainQueue.m_groupStart->block->m_z > m_fillQueue.m_groupStart->block->m_z or drainVolume >= fillVolume);
		uint32_t drainZ = m_drainQueue.m_groupStart->block->m_z;
		uint32_t fillZ = m_fillQueue.m_groupStart->block->m_z;
		// If drain is less then 2 units above fill then end loop.
		//TODO: using fillVolume > drainVolume here rather then the above assert feels like a hack.
		if(drainZ < fillZ || (drainZ == fillZ && (fillVolume >= drainVolume || (drainVolume == 1 &&  fillVolume == 0))))
		{
			// if no new blocks have been added this step then set stable
			if(m_fillQueue.m_futureNoLongerEmpty.empty() && m_disolvedInThisGroup.empty())
			{
				if constexpr (Config::fluidsSeepDiagonalModifier == 0)
					m_stable = true;
				else if(m_diagonalBlocks.empty())
					m_stable = true;
			}
			break;
		}
		for(auto itd = m_drainQueue.m_groupStart; itd != m_drainQueue.m_groupEnd; ++itd)
			if(itd->delta != 0)
				for(auto itf = m_fillQueue.m_groupStart; itf != m_fillQueue.m_groupEnd; ++itf)
					if(itf->delta != 0)
						assert(itf->block != itd->block);
		for(auto itf = m_fillQueue.m_groupStart; itf != m_fillQueue.m_groupEnd; ++itf)
			if(itf->delta != 0)
				for(auto itd = m_drainQueue.m_groupStart; itd != m_drainQueue.m_groupEnd; ++itd)
					if(itd->delta != 0)
						assert(itf->block != itd->block);
		// How much fluid is there space for total.
		uint32_t flowCapacityFill = m_fillQueue.groupCapacityPerBlock();
		// How much can be filled before the next low block(s).
		uint32_t flowTillNextStepFill = m_fillQueue.groupFlowTillNextStepPerBlock();
		// How much is avaliable to drain total.
		uint32_t flowCapacityDrain = m_drainQueue.groupCapacityPerBlock();
		// How much can be drained before the next high block(s).
		uint32_t flowTillNextStepDrain = m_drainQueue.groupFlowTillNextStepPerBlock();
		// How much can drain before equalization. Only applies if the groups are on the same z.
		uint32_t maxDrainForEqualibrium, maxFillForEquilibrium;
		if(m_fillQueue.m_groupStart->block->m_z == m_drainQueue.m_groupStart->block->m_z)
		{
			uint32_t totalLevel = (fillVolume * m_fillQueue.groupSize()) + (drainVolume * m_drainQueue.groupSize());
			uint32_t totalCount = m_fillQueue.groupSize() + m_drainQueue.groupSize();
			// We want to round down here so default truncation is fine.
			uint32_t equalibriumLevel = totalLevel / totalCount;
			maxDrainForEqualibrium = drainVolume - equalibriumLevel;
			maxFillForEquilibrium = equalibriumLevel - fillVolume;
		}
		else
			maxDrainForEqualibrium = maxFillForEquilibrium = UINT32_MAX;
		// Viscosity is applied only to fill.
		// Fill is allowed to be 0. If there is not enough volume in drain group to put at least one in each of fill group then we let all of drain group go to excess volume.
		uint32_t perBlockFill = std::min({flowCapacityFill, flowTillNextStepFill, maxFillForEquilibrium, (uint32_t)m_viscosity});
		uint32_t perBlockDrain = std::min({flowCapacityDrain, flowTillNextStepDrain, maxDrainForEqualibrium});
		assert(perBlockDrain != 0);
		uint32_t totalFill = perBlockFill * m_fillQueue.groupSize();
		uint32_t totalDrain = perBlockDrain * m_drainQueue.groupSize();
		if(totalFill < totalDrain)
		{
			if(maxFillForEquilibrium == perBlockFill)
				perBlockDrain = maxDrainForEqualibrium;
			else
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
		assert(totalFill <= totalDrain);
		// Viscosity is consumed by flow.
		m_viscosity -= perBlockFill;
		// Record changes.
		m_drainQueue.recordDelta(perBlockDrain, flowCapacityDrain, flowTillNextStepDrain);
		if(perBlockFill != 0)
			m_fillQueue.recordDelta(perBlockFill, flowCapacityFill, flowTillNextStepFill);
		m_excessVolume += totalDrain - totalFill;
		// If we are at equilibrium then stop looping.
		// Don't mark stable because there may be newly added adjacent to flow into next tick.
		if(perBlockDrain == maxDrainForEqualibrium)
			break;
	}
	// Flow loops completed, analyze results.
	std::unordered_set<Block*> futureBlocks;
	//futureBlocks.reserve(m_drainQueue.m_set.size() + m_fillQueue.m_futureNoLongerEmpty.size());
	futureBlocks.insert(m_drainQueue.m_set.begin(), m_drainQueue.m_set.end());
	std::erase_if(futureBlocks, [&](Block* block){ return m_drainQueue.m_futureEmpty.contains(block); });
	futureBlocks.insert(m_fillQueue.m_futureNoLongerEmpty.begin(), m_fillQueue.m_futureNoLongerEmpty.end());
	if(futureBlocks.empty())
		m_destroy = true;
	// We can't return here because we need to convert descriptive future into proscriptive future :(
	// -Find future blocks for futureEmptyAdjacent.
	for(Block* block : m_fillQueue.m_futureNoLongerEmpty)
	{
		for(Block* adjacent : block->m_adjacentsVector)
			if(adjacent->fluidCanEnterEver() && !m_drainQueue.m_set.contains(adjacent) && !m_fillQueue.m_set.contains(adjacent) 
			  )
			{
				auto found = adjacent->m_fluids.find(&m_fluidType);
				if(found == adjacent->m_fluids.end() || found->second.first < Config::maxBlockVolume || found->second.second != this)
					m_futureNewEmptyAdjacents.insert(adjacent);
			}
		if(Config::fluidsSeepDiagonalModifier != 0)
			addDiagonalsFor(*block);
	}
	// -Find any potental newly created groups.
	// Collect blocks adjacent to newly empty which are !empty.
	// Also collect possiblyNoLongerAdjacent.
	std::unordered_set<Block*> potentialNewGroups;
	potentialNewGroups.swap(m_potentiallySplitFromSyncronusStep);
	std::unordered_set<Block*> possiblyNoLongerAdjacent;
	possiblyNoLongerAdjacent.swap(m_potentiallyNoLongerAdjacentFromSyncronusStep);
	// Collect all adjacent to futureEmpty which fluid can enter ever.
	std::unordered_set<Block*> adjacentToFutureEmpty;
	//adjacentToFutureEmpty.reserve(m_drainQueue.m_futureEmpty.size() * 6);
	//TODO: Avoid this declaration if diagonal seep is disabled.
	std::unordered_set<Block*> possiblyNoLongerDiagonal;
	//possiblyNoLongerDiagonal.reserve(m_drainQueue.m_futureEmpty.size() * 4);
	for(Block* block : m_drainQueue.m_futureEmpty)
	{
		for(Block* adjacent : block->m_adjacentsVector)
			if(adjacent->fluidCanEnterEver())
				adjacentToFutureEmpty.insert(adjacent);
		if constexpr (Config::fluidsSeepDiagonalModifier != 0)
		{
			for(Block* adjacent : block->getEdgeAndCornerAdjacentOnly())
				if(adjacent->fluidCanEnterEver())
					possiblyNoLongerDiagonal.insert(adjacent);
		}
	}
	if constexpr (Config::fluidsSeepDiagonalModifier != 0)
		for(Block* block : possiblyNoLongerDiagonal)
			for(Block* doubleDiagonal : block->getEdgeAdjacentOnSameZLevelOnly())
				if(futureBlocks.contains(doubleDiagonal))
					m_diagonalBlocks.erase(block);
	for(Block* block : adjacentToFutureEmpty)
		// If block won't be empty then check for forming a new group as it may be detached.
		if(futureBlocks.contains(block))
			potentialNewGroups.insert(block);
	// Else check for removal from empty adjacent queue.
		else
			possiblyNoLongerAdjacent.insert(block);
	// Seperate into contiguous groups. Each block in potentialNewGroups might be in a seperate group.
	// If there is only one potential new group there can not be a split: there needs to be another group to split from.
	std::erase_if(potentialNewGroups, [&](Block* block){ return !futureBlocks.contains(block); });
	if(potentialNewGroups.size() > 1)
	{
		std::unordered_set<Block*> closed;
		//closed.reserve(futureBlocks.size());
		for(Block* block : potentialNewGroups)
		{
			if(closed.contains(block))
				continue;
			auto condition = [&](Block& block){ return futureBlocks.contains(&block); };
			std::unordered_set<Block*> adjacents = block->collectAdjacentsWithCondition(condition);
			// Add whole group to closed. There is only one iteration per group as others will be rejected by the closed guard.
			closed.insert(adjacents.begin(), adjacents.end());
			m_futureGroups.emplace_back(adjacents);
		}
	}
	if(!m_futureGroups.empty())
	{
		// Find the largest.
		auto condition = [](FluidGroupSplitData& a, FluidGroupSplitData& b){ return a.members.size() < b.members.size(); };
		auto it = std::ranges::max_element(m_futureGroups, condition);
		// Put it at the end because it needs to be in a known place and it will be removed later.
		std::rotate(it, it + 1, m_futureGroups.end());
		// Record each group's future new adjacent.
		for(Block* block : m_futureNewEmptyAdjacents)
			for(Block* adjacent : block->m_adjacentsVector)
				if(adjacent->fluidCanEnterEver())
				{
					for(FluidGroupSplitData& fluidGroupSplitData : m_futureGroups)
						if(fluidGroupSplitData.members.contains(adjacent))
							fluidGroupSplitData.futureAdjacent.insert(block);
				}
	}
	// -Find no longer adjacent empty to remove from fill queue and newEmptyAdjacent.
	std::unordered_set<Block*> futureRemoveFromEmptyAdjacents;
	//futureRemoveFromEmptyAdjacents.reserve(possiblyNoLongerAdjacent.size());
	for(Block* block : possiblyNoLongerAdjacent)
	{
		bool stillAdjacent = false;
		for(Block* adjacent : block->m_adjacentsVector)
		{
			if(adjacent->fluidCanEnterEver() && futureBlocks.contains(adjacent))
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
	//m_futureAddToFillQueue.reserve(m_futureNewEmptyAdjacents.size() + m_drainQueue.m_futureNoLongerFull.size());
	m_futureAddToFillQueue = m_futureNewEmptyAdjacents;
	for(Block* block : m_drainQueue.m_futureNoLongerFull)
		if(!futureRemoveFromEmptyAdjacents.contains(block))
			m_futureAddToFillQueue.insert(block);
	//m_futureRemoveFromFillQueue = futureRemoveFromEmptyAdjacents + m_futureFull;
	//m_futureRemoveFromFillQueue.reserve(futureRemoveFromEmptyAdjacents.size() + m_fillQueue.m_futureFull.size());
	m_futureRemoveFromFillQueue = futureRemoveFromEmptyAdjacents;
	m_futureRemoveFromFillQueue.insert(m_fillQueue.m_futureFull.begin(), m_fillQueue.m_futureFull.end());
	validate();
}
void FluidGroup::writeStep()
{
	assert(!m_merged);
	assert(!m_disolved);
	assert(!m_destroy);
	m_area.validateAllFluidGroups();
	m_drainQueue.applyDelta();
	m_fillQueue.applyDelta();
	// Update queues.
	validate();
	for(Block* block : m_futureAddToFillQueue)
		assert(!m_futureRemoveFromFillQueue.contains(block));
	// Don't add to drain queue if taken by another fluid group already.
	std::erase_if(m_futureAddToDrainQueue, [&](Block* block){ 
			auto found = block->m_fluids.find(&m_fluidType);
			return found != block->m_fluids.end() && found->second.second != this; 
			});
	for(Block* block : m_futureAddToDrainQueue)
		assert(!m_futureRemoveFromDrainQueue.contains(block));
	for(Block* block : m_futureRemoveFromFillQueue)
	{
		bool tests = !block->m_fluids.contains(&m_fluidType) || block->m_fluids.at(&m_fluidType).first == Config::maxBlockVolume || block->m_fluids.at(&m_fluidType).second != this;
		assert(tests || !m_futureGroups.empty());
		if(!tests && !m_futureGroups.empty())
		{
			bool found = false;
			for(FluidGroupSplitData& fluidGroupSplitData : m_futureGroups)
				if(fluidGroupSplitData.members.contains(block))
				{
					found = true;
					break;
				}
			assert(found);
		}
	}
	for(Block* block : m_futureRemoveFromDrainQueue)
	{
		bool tests = !block->m_fluids.contains(&m_fluidType) || block->m_fluids.at(&m_fluidType).second != this;
		assert(tests || !m_futureGroups.empty());
		if(!tests && !m_futureGroups.empty())
		{
			bool found = false;
			for(FluidGroupSplitData& fluidGroupSplitData : m_futureGroups)
				if(fluidGroupSplitData.members.contains(block))
				{
					found = true;
					break;
				}
			assert(found);
		}
	}
	m_fillQueue.removeBlocks(m_futureRemoveFromFillQueue);
	m_fillQueue.addBlocks(m_futureAddToFillQueue);
	m_drainQueue.removeBlocks(m_futureRemoveFromDrainQueue);
	m_drainQueue.addBlocks(m_futureAddToDrainQueue);
	validate();
	m_area.validateAllFluidGroups();
}
void FluidGroup::afterWriteStep()
{
	// Any fluid group could be marked as disolved or destroyed during iteration.
	if(m_disolved || m_destroy)
		return;
	validate();
	assert(!m_merged);
	// Do seeping through corners if enabled.
	if constexpr(Config::fluidsSeepDiagonalModifier != 0)
	{
		if(m_viscosity != 0 && !m_drainQueue.m_set.empty() && !m_diagonalBlocks.empty())
		{
			for(Block* diagonal : m_diagonalBlocks)
			{
				if(diagonal->fluidCanEnterCurrently(m_fluidType))
				{
					uint32_t diagonalPriority = (diagonal->m_z * Config::maxBlockVolume * 2) + diagonal->volumeOfFluidTypeContains(m_fluidType);
					Block* topBlock = m_drainQueue.m_queue[0].block;
					uint32_t topPriority = (topBlock->m_z * Config::maxBlockVolume * 2) + topBlock->volumeOfFluidTypeContains(m_fluidType)  + m_excessVolume;
					if(topPriority > diagonalPriority)
					{
						uint32_t flow = std::min({
								m_viscosity,
								(topPriority - diagonalPriority)/2,
								diagonal->volumeOfFluidTypeCanEnter(m_fluidType),
								std::max(1u, m_fluidType.viscosity / Config::fluidsSeepDiagonalModifier)
								});
						m_excessVolume -= flow;
						m_viscosity -= flow;
						if(m_viscosity <= 0)
							break;
						diagonal->addFluid(flow, m_fluidType);
					}
				}
			}
		}
	}
	// Resolve overfull blocks.
	validate();
	for(Block* block : m_fillQueue.m_overfull)
		if(block->m_totalFluidVolume > Config::maxBlockVolume)
			block->resolveFluidOverfull();
	validate();
	for(Block* block : m_fillQueue.m_futureNoLongerEmpty)
		addMistFor(*block);
	validate();
}
void FluidGroup::splitStep()
{
	assert(!m_merged);
	assert(!m_disolved);
	assert(!m_destroy);
	validate();
	// Disperse disolved, split off fluidGroups of another fluidType.
	// TODO: Transfer ownership to adjacent fluid groups with a lower density then this one but higher then disolved group.
	std::vector<const FluidType*> dispersed;
	for(auto& [fluidType, fluidGroup] : m_disolvedInThisGroup)
	{
		assert(fluidGroup->m_drainQueue.m_set.empty());
		assert(&m_fluidType != fluidType);
		assert(&fluidGroup->m_fluidType == fluidType);
		assert(fluidGroup->m_excessVolume > 0);
		for(FutureFlowBlock& futureFlowBlock : m_fillQueue.m_queue)
		{
			Block* block = futureFlowBlock.block;
			auto found = block->m_fluids.find(fluidType);
			if(found != block->m_fluids.end() || block->fluidCanEnterCurrently(*fluidType))
			{
				uint32_t capacity = block->volumeOfFluidTypeCanEnter(*fluidType);
				uint32_t flow = std::min(capacity, (uint32_t)fluidGroup->m_excessVolume);
				block->m_totalFluidVolume += flow;
				fluidGroup->m_excessVolume -= flow;
				if(found != block->m_fluids.end())
				{
					found->second.first += flow;
					found->second.second->m_excessVolume += fluidGroup->m_excessVolume;
					fluidGroup->m_destroy = true;
				}
				else
				{
					block->m_fluids.emplace(fluidType, std::make_pair(flow, fluidGroup));
					fluidGroup->addBlock(*block, false);
					fluidGroup->m_disolved = false;
				}
				dispersed.push_back(fluidType);
				break;
			}
		}
	}
	validate();
	for(const FluidType* fluidType : dispersed)
		m_disolvedInThisGroup.erase(fluidType);
	// Split off future groups of this fluidType.
	if(m_futureGroups.empty() || m_futureGroups.size() == 1)
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
	if constexpr (Config::fluidsSeepDiagonalModifier != 0)
		std::erase_if(m_diagonalBlocks, [&](Block* block){
				for(Block* diagonal : block->getEdgeAdjacentOnSameZLevelOnly())
					if(m_futureGroups.back().members.contains(diagonal))
						return false;
				return true;
				});
	// Remove largest group, it will remain as this instance.
	m_futureGroups.pop_back();
	for(auto& [members, newAdjacent] : m_futureGroups)
	{
		FluidGroup* fluidGroup = m_area.createFluidGroup(m_fluidType, members, false);
		fluidGroup->m_futureNewEmptyAdjacents.swap(newAdjacent);
	}
	validate();
}
// Do all split prior to doing merge.
void FluidGroup::mergeStep()
{
	assert(!m_disolved);
	assert(!m_destroy);
	validate();
	// Record merge. First group consumes subsequent groups.
	for(Block* block : m_futureNewEmptyAdjacents)
	{
		// Fluid groups may be marked merged during merge iteration.
		if(m_merged)
			return;
		auto found = block->m_fluids.find(&m_fluidType);
		if(found == block->m_fluids.end())
			continue;
		FluidGroup* fluidGroup = found->second.second;
		fluidGroup->validate();
		assert(!fluidGroup->m_merged);
		assert(fluidGroup->m_fluidType == m_fluidType);
		if(fluidGroup != this)
		{
			merge(fluidGroup);
			continue;
		}
	}
	validate();
}
int32_t FluidGroup::totalVolume()
{
	int32_t output = m_excessVolume;
	for(Block* block : m_drainQueue.m_set)
		output += block->volumeOfFluidTypeContains(m_fluidType);
	return output;
}
void FluidGroup::validate() const
{
	if(m_merged || m_destroy || m_disolved)
		return;
	for(Block* block : m_drainQueue.m_set)
		for(auto [fluidType, pair] : block->m_fluids)
		{
			if(pair.second == this)
				continue;
			assert(&pair.second->m_fluidType == fluidType);
			assert(pair.second->m_drainQueue.m_set.contains(block));
		}
}
void FluidGroup::validate(std::unordered_set<FluidGroup*> toErase)
{
	for(FluidGroup* fluidGroup : toErase)
		assert(fluidGroup->m_merged || fluidGroup->m_drainQueue.m_set.empty());
	for(Block* block : m_drainQueue.m_set)
		for(auto [fluidType, pair] : block->m_fluids)
		{
			assert(&pair.second->m_fluidType == fluidType);
			assert(pair.second->m_drainQueue.m_set.contains(block));
			assert(!toErase.contains(pair.second));
		}
}
