/*
 * A set of blocks which are adjacent to eachother and contain the same fluid.
 * Intented to be run in series in a pool thread.
 * Maintains two priority queues:
 * 	m_blocksByZandTotalFluidHeight is all blocks which have fluid. It is sorted by high to low. This is the source of flow.
 * 	m_adjacentAndUnfullBlocksByInverseZandTotalFluidHeight is all blocks which have some fluid but aren't full, as well as blocks with no fluid that fluid can enter and are adjacent. It is sorted by low to high. This is the destination of flow.
 * The flow for the step is computed in readStep and applied in writeStep so that readStep can run concurrently with other read only tasks.
 */

#include "fluidGroup.h"
#include "blocks/blocks.h"
#include "fluidType.h"
#include "types.h"
#include "util.h"
#include "config.h"
#include "area.h"
#include "simulation.h"
#include "vectorContainers.h"

#include <queue>
#include <cassert>
#include <numeric>

//TODO: reuse blocks as m_fillQueue.m_set.
FluidGroup::FluidGroup(const FluidTypeId& ft, BlockIndices& blocks, Area& area, bool checkMerge) :
	m_fluidType(ft)
{
	for(BlockIndex block : blocks)
		if(area.getBlocks().fluid_contains(block, m_fluidType))
			addBlock(area, block, checkMerge);
}
void FluidGroup::addFluid(Area& area, const CollisionVolume& volume)
{
	m_excessVolume += volume.get();
	setUnstable(area);
}
void FluidGroup::removeFluid(Area& area, const CollisionVolume& volume)
{
	m_excessVolume -= volume.get();
	setUnstable(area);
}
void FluidGroup::addBlock(Area& area, const BlockIndex& block, bool checkMerge)
{
	assert(!m_merged);
	assert(area.getBlocks().fluid_contains(block, m_fluidType));
	// Already recorded this one.
	if(m_drainQueue.m_set.contains(block))
		return;
	setUnstable(area);
	// Cannot be in fill queue if full. Must be otherwise.
	FluidData* found = area.getBlocks().fluid_getData(block, m_fluidType);
	if(found && found->volume < Config::maxBlockVolume)
		m_fillQueue.maybeAddBlock(block);
	else
		m_fillQueue.maybeRemoveBlock(block);
	// Set group membership in Blocks.
	found = area.getBlocks().fluid_getData(block, m_fluidType);
	FluidGroup* oldGroup = found->group;
	if(oldGroup != nullptr && oldGroup != this)
		oldGroup->removeBlock(area, block);
	found->group = this;
	m_drainQueue.maybeAddBlock(block);
	if constexpr(Config::fluidsSeepDiagonalModifier != 0)
		m_diagonalBlocks.maybeRemove(block);
	// Add adjacent if fluid can enter.
	SmallSet<FluidGroup*> toMerge;
	for(BlockIndex adjacent : area.getBlocks().getDirectlyAdjacent(block))
		if(adjacent.exists())
		{
			if(!area.getBlocks().fluid_canEnterEver(adjacent))
				continue;
			FluidData* found = area.getBlocks().fluid_getData(adjacent, m_fluidType);
			// Merge groups if needed.
			if(found && checkMerge)
			{
				assert(!found->group->m_merged);
				if(found->group == this)
					continue;
				toMerge.insert(found->group);
			}
			if(!found || found->volume < Config::maxBlockVolume)
				m_fillQueue.maybeAddBlock(adjacent);
		}
	// TODO: (performance) collect all groups to merge recursively and then merge all into the largest.
	FluidGroup* larger = this;
	for(FluidGroup* oldGroup : toMerge)
		// oldGroup may have been merged by recursion in a previous iteration.
		if(!oldGroup->m_merged)
			larger = larger->merge(area, oldGroup);
	if constexpr (Config::fluidsSeepDiagonalModifier != 0)
		larger->addDiagonalsFor(area, block);
	larger->addMistFor(area, block);
}
void FluidGroup::removeBlock(Area& area, const BlockIndex& block)
{
	setUnstable(area);
	m_drainQueue.removeBlock(block);
	m_potentiallyNoLongerAdjacentFromSyncronusStep.add(block);
	for(BlockIndex adjacent : area.getBlocks().getDirectlyAdjacent(block))
		if(adjacent.exists() && area.getBlocks().fluid_canEnterEver(adjacent))
		{
			//Check for group split.
			auto found = area.getBlocks().fluid_getData(adjacent, m_fluidType);
			if(found && found->group == this)
				m_potentiallySplitFromSyncronusStep.maybeAdd(adjacent);
			else
				//Check for empty adjacent to remove.
				m_potentiallyNoLongerAdjacentFromSyncronusStep.maybeAdd(adjacent);
		}
	if constexpr (Config::fluidsSeepDiagonalModifier != 0)
	{
		for(BlockIndex diagonal : area.getBlocks().getEdgeAndCornerAdjacentOnly(block))
			if(area.getBlocks().fluid_canEnterEver(diagonal) && m_diagonalBlocks.contains(diagonal))
			{
				//TODO: m_potentiallyNoLongerDiagonalFromSyncronusCode
				bool found = false;
				for(BlockIndex doubleDiagonal : area.getBlocks().getEdgeAndCornerAdjacentOnly(diagonal))
					if(m_drainQueue.m_set.contains(doubleDiagonal))
					{
						found = true;
						continue;
					}
				if(!found)
					m_diagonalBlocks.maybeRemove(diagonal);
			}
	}
}
void FluidGroup::setUnstable(Area& area)
{
	m_stable = false;
	area.m_hasFluidGroups.markUnstable(*this);
}
void FluidGroup::addDiagonalsFor(Area& area, const BlockIndex& block)
{
	auto& blocks = area.getBlocks();
	for(BlockIndex diagonal : blocks.getEdgeAdjacentOnSameZLevelOnly(block))
		if(blocks.fluid_canEnterEver(diagonal) && !m_fillQueue.m_set.contains(diagonal) &&
				!m_drainQueue.m_set.contains(diagonal) && !m_diagonalBlocks.contains(diagonal))
		{
			Point3D blockCoordinates = blocks.getCoordinates(block);
			Point3D diagonalCoordinates = blocks.getCoordinates(diagonal);
			// Check if there is a 'pressure reducing diagonal' created by two solid blocks.
			int diffX = diagonalCoordinates.x.get() - blockCoordinates.x.get();
			int diffY = diagonalCoordinates.y.get() - blockCoordinates.y.get();
			if(blocks.solid_is(blocks.offset(block,diffX, 0, 0)) && blocks.solid_is(blocks.offset(block, 0, diffY, 0)))
				m_diagonalBlocks.add(diagonal);
		}
}
void FluidGroup::addMistFor(Area& area, const BlockIndex& block)
{
	auto& blocks = area.getBlocks();
	if(FluidType::getMistDuration(m_fluidType) != 0 &&
		(
			!blocks.getBlockBelow(block).exists() ||
			!blocks.solid_is(blocks.getBlockBelow(block))
		)
	)
		for(BlockIndex adjacent : blocks.getAdjacentOnSameZLevelOnly(block))
			if(blocks.fluid_canEnterEver(adjacent))
				blocks.fluid_spawnMist(adjacent, m_fluidType);
}
FluidGroup* FluidGroup::merge(Area& area, FluidGroup* smaller)
{
	assert(smaller != this);
	assert(smaller->m_fluidType == m_fluidType);
	assert(!smaller->m_merged);
	assert(!m_merged);
	assert(!smaller->m_disolved);
	assert(!m_disolved);
	assert(!smaller->m_destroy);
	assert(!m_destroy);

	auto& blocks = area.getBlocks();
	FluidGroup* larger = this;
	if(smaller->m_drainQueue.m_set.size() > larger->m_drainQueue.m_set.size())
		std::swap(smaller, larger);
	// Mark as merged rather then destroying right away so as not to interfear with iteration.
	smaller->m_merged = true;
	larger->setUnstable(area);
	// Add excess volume.
	larger->m_excessVolume += smaller->m_excessVolume;
	// Merge queues.
	larger->m_drainQueue.merge(smaller->m_drainQueue);
	larger->m_fillQueue.merge(smaller->m_fillQueue);
	// Set fluidGroup for merged blocks.
	for(BlockIndex block : smaller->m_drainQueue.m_set)
	{
		assert(blocks.fluid_contains(block, m_fluidType));
		blocks.fluid_getData(block, m_fluidType)->group = larger;
		assert(larger->m_drainQueue.m_set.contains(block));
	}
	// Merge diagonal seep if enabled.
	if constexpr (Config::fluidsSeepDiagonalModifier != 0)
	{
		smaller->m_diagonalBlocks.erase_if([&](const BlockIndex& block){
			return !larger->m_drainQueue.m_set.contains(block) && !larger->m_fillQueue.m_set.contains(block);
		});
		larger->m_diagonalBlocks.erase_if([&](const BlockIndex& block){
			return smaller->m_drainQueue.m_set.contains(block) || smaller->m_fillQueue.m_set.contains(block);
		});
		larger->m_diagonalBlocks.merge(smaller->m_diagonalBlocks);
	}
	// Merge other fluid groups ment to merge with smaller with larger instead.
	for(BlockIndex block : smaller->m_futureNewEmptyAdjacents)
	{
		auto found = blocks.fluid_getData(block, m_fluidType);
		if(!found)
			continue;
		if(!found->group->m_merged && found->group != larger)
		{
			larger->merge(area, found->group);
			if(larger->m_merged)
				larger = found->group;
		}
	}
	return larger;
}
void FluidGroup::readStep(Area& area)
{
	assert(!m_merged);
	assert(!m_destroy);
	assert(!m_stable);
	assert(!m_disolved);
	auto& blocks = area.getBlocks();
	m_futureNewEmptyAdjacents.clear();
	m_futureGroups.clear();
	m_futureNotifyPotentialUnfullAdjacent.clear();
	m_viscosity = FluidType::getViscosity(m_fluidType);
	m_drainQueue.initalizeForStep(area, *this);
	m_fillQueue.initalizeForStep(area, *this);
	validate(area);
	// If there is no where to flow into there is nothing to do.
	if(m_diagonalBlocks.empty() && (m_fillQueue.m_set.empty() ||
		((m_fillQueue.groupSize() == 0 || m_fillQueue.groupCapacityPerBlock() == 0) && m_excessVolume >= 0 ) ||
		((m_drainQueue.groupSize() == 0 || m_drainQueue.groupCapacityPerBlock() == 0) && m_excessVolume <= 0))
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
		CollisionVolume flowCapacity = m_fillQueue.groupCapacityPerBlock();
		// How much can be filled before the next low block(s).
		CollisionVolume flowTillNextStep = m_fillQueue.groupFlowTillNextStepPerBlock(area);
		if(flowTillNextStep.empty())
			// If unset then set to max in order to exclude from std::min
			flowTillNextStep = CollisionVolume::max();
		CollisionVolume excessPerBlock = CollisionVolume::create(m_excessVolume / m_fillQueue.groupSize());
		CollisionVolume flowPerBlock = std::min({flowTillNextStep, flowCapacity, excessPerBlock});
		m_excessVolume -= flowPerBlock.get() * m_fillQueue.groupSize();
		m_fillQueue.recordDelta(area, *this, flowPerBlock, flowCapacity, flowTillNextStep);
	}
	while(m_excessVolume < 0 && m_drainQueue.groupSize() != 0 && m_drainQueue.m_groupStart != m_drainQueue.m_queue.end())
	{
		// If there isn't enough to spread evenly then do nothing.
		if((uint32_t)(-1 * m_excessVolume) < m_drainQueue.groupSize())
			break;
		// How much is avaliable to drain total.
		CollisionVolume flowCapacity = m_drainQueue.groupCapacityPerBlock();
		// How much can be drained before the next high block(s).
		CollisionVolume flowTillNextStep = m_drainQueue.groupFlowTillNextStepPerBlock(area);
		if(flowTillNextStep.empty())
			// If unset then set to max in order to exclude from std::min
			flowTillNextStep = CollisionVolume::max();
		CollisionVolume excessPerBlock = CollisionVolume::create((-1 * m_excessVolume) / m_drainQueue.groupSize());
		CollisionVolume flowPerBlock = std::min({flowCapacity, flowTillNextStep, excessPerBlock});
		m_excessVolume += (flowPerBlock * m_drainQueue.groupSize()).get();
		m_drainQueue.recordDelta(area, flowPerBlock, flowCapacity, flowTillNextStep);
	}
	// Do primary flow.
	// If we have reached the end of either queue the loop ends.
	while(m_viscosity > 0 && m_drainQueue.groupSize() != 0 && m_fillQueue.groupSize() != 0)
	{
		assert(m_fillQueue.m_groupEnd == m_fillQueue.m_queue.end() ||
				blocks.getZ(m_fillQueue.m_groupStart->block) != blocks.getZ(m_fillQueue.m_groupEnd->block) ||
				m_fillQueue.m_groupStart->capacity != m_fillQueue.m_groupEnd->capacity);
		CollisionVolume drainVolume = m_drainQueue.groupLevel(area, *this);
		assert(drainVolume != 0);
		CollisionVolume fillVolume = m_fillQueue.groupLevel(area, *this);
		assert(fillVolume < Config::maxBlockVolume);
		//uint32_t fillInverseCapacity = Config::maxBlockVolume - m_fillQueue.m_groupStart->capacity;
		//assert(m_drainQueue.m_groupStart->block->m_z > m_fillQueue.m_groupStart->block->m_z || drainVolume >= fillVolume);
		//uint32_t drainZ = m_drainQueue.m_groupStart->block->m_z;
		//uint32_t fillZ = m_fillQueue.m_groupStart->block->m_z;
		//TODO: using fillVolume > drainVolume here rather then the above assert feels like a hack.
		//if(drainZ < fillZ || (drainZ == fillZ && (fillVolume >= drainVolume || (drainVolume == 1 && fillVolume == 0))))
		[[maybe_unused]] bool stopHere = area.m_simulation.m_step == 6 && fillVolume == 0 && drainVolume == 2;
		if(dispositionIsStable(area, fillVolume, drainVolume))
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
		CollisionVolume flowCapacityFill = m_fillQueue.groupCapacityPerBlock();
		// How much can be filled before the next low block(s).
		CollisionVolume flowTillNextStepFill = m_fillQueue.groupFlowTillNextStepPerBlock(area);
		if(flowTillNextStepFill.empty())
			// If the next step is on another z level then this value will be empty. Set it to max to exclude it from the std::min call later.
			flowTillNextStepFill = CollisionVolume::max();
		// How much is avaliable to drain total.
		CollisionVolume flowCapacityDrain = m_drainQueue.groupCapacityPerBlock();
		// How much can be drained before the next high block(s).
		CollisionVolume flowTillNextStepDrain = m_drainQueue.groupFlowTillNextStepPerBlock(area);
		if(flowTillNextStepDrain.empty())
			// If the next step is on another z level then this value will be empty. Set it to max to exclude it from the std::min call later.
			flowTillNextStepDrain = CollisionVolume::max();
		// How much can drain before equalization. Only applies if the groups are on the same z.
		CollisionVolume maxDrainForEqualibrium, maxFillForEquilibrium;
		if(blocks.getZ(m_fillQueue.m_groupStart->block) == blocks.getZ(m_drainQueue.m_groupStart->block))
		{
			CollisionVolume totalLevel = (fillVolume * m_fillQueue.groupSize()) + (drainVolume * m_drainQueue.groupSize());
			uint32_t totalCount = m_fillQueue.groupSize() + m_drainQueue.groupSize();
			// We want to round down here so default truncation is fine.
			CollisionVolume equalibriumLevel = totalLevel / totalCount;
			maxDrainForEqualibrium = drainVolume - equalibriumLevel;
			maxFillForEquilibrium = equalibriumLevel - fillVolume;
		}
		else
			maxDrainForEqualibrium = maxFillForEquilibrium = CollisionVolume::max();
		// Viscosity is applied only to fill.
		// Fill is allowed to be 0. If there is not enough volume in drain group to put at least one in each of fill group then we let all of drain group go to excess volume.
		CollisionVolume perBlockFill = std::min({flowCapacityFill, flowTillNextStepFill, maxFillForEquilibrium, CollisionVolume::create(m_viscosity)});
		CollisionVolume perBlockDrain = std::min({flowCapacityDrain, flowTillNextStepDrain, maxDrainForEqualibrium});
		assert(perBlockDrain != 0);
		CollisionVolume totalFill = perBlockFill * m_fillQueue.groupSize();
		CollisionVolume totalDrain = perBlockDrain * m_drainQueue.groupSize();
		if(totalFill < totalDrain)
		{
			if(maxFillForEquilibrium == perBlockFill)
				perBlockDrain = maxDrainForEqualibrium;
			else
				perBlockDrain = CollisionVolume::create(std::ceil((float)totalFill.get() / (float)m_drainQueue.groupSize()));
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
		m_viscosity -= perBlockFill.get();
		// Record changes.
		m_drainQueue.recordDelta(area, perBlockDrain, flowCapacityDrain, flowTillNextStepDrain);
		if(perBlockFill != 0)
			m_fillQueue.recordDelta(area, *this, perBlockFill, flowCapacityFill, flowTillNextStepFill);
		m_excessVolume += (int)totalDrain.get() - (int)totalFill.get();
		// If we are at equilibrium then stop looping.
		// Don't mark stable because there may be newly added adjacent to flow into next tick.
		if(perBlockDrain == maxDrainForEqualibrium)
			break;
	}
	// Flow loops completed, analyze results.
	BlockIndices futureBlocks;
	//futureBlocks.reserve(m_drainQueue.m_set.size() + m_fillQueue.m_futureNoLongerEmpty.size());
	for(BlockIndex block : m_drainQueue.m_set)
		if(!m_drainQueue.m_futureEmpty.contains(block))
			futureBlocks.add(block);
	futureBlocks.merge(m_fillQueue.m_futureNoLongerEmpty);
	if(futureBlocks.empty())
	{
		assert(m_excessVolume < 1);
		m_destroy = true;
	}
	// We can't return here because we need to convert descriptive future into proscriptive future.
	// Find future blocks for futureEmptyAdjacent.
	for(BlockIndex block : m_fillQueue.m_futureNoLongerEmpty)
	{
		for(BlockIndex adjacent : blocks.getDirectlyAdjacent(block))
			if(adjacent.exists() && area.getBlocks().fluid_canEnterEver(adjacent) && !m_drainQueue.m_set.contains(adjacent) && !m_fillQueue.m_set.contains(adjacent)
			  )
			{
				auto found = area.getBlocks().fluid_getData(adjacent, m_fluidType);
				if(!found || found->volume < Config::maxBlockVolume || found->group != this)
					m_futureNewEmptyAdjacents.maybeAdd(adjacent);
			}
		if(Config::fluidsSeepDiagonalModifier != 0)
			addDiagonalsFor(area, block);
	}
	// -Find any potental newly created groups.
	// Collect blocks adjacent to newly empty which are !empty.
	// Also collect possiblyNoLongerAdjacent.
	BlockIndices potentialNewGroups;
	potentialNewGroups.swap(m_potentiallySplitFromSyncronusStep);
	BlockIndices possiblyNoLongerAdjacent;
	[[maybe_unused]] bool stopHere = area.m_simulation.m_step == 3 && m_fluidType == FluidType::byName("CO2");
	possiblyNoLongerAdjacent.swap(m_potentiallyNoLongerAdjacentFromSyncronusStep);
	// Collect all adjacent to futureEmpty which fluid can enter ever.
	BlockIndices adjacentToFutureEmpty;
	//adjacentToFutureEmpty.reserve(m_drainQueue.m_futureEmpty.size() * 6);
	//TODO: Avoid this declaration if diagonal seep is disabled.
	BlockIndices possiblyNoLongerDiagonal;
	//possiblyNoLongerDiagonal.reserve(m_drainQueue.m_futureEmpty.size() * 4);
	for(BlockIndex block : m_drainQueue.m_futureEmpty)
	{
		for(BlockIndex adjacent : blocks.getDirectlyAdjacent(block))
			if(adjacent.exists() && area.getBlocks().fluid_canEnterEver(adjacent))
				adjacentToFutureEmpty.maybeAdd(adjacent);
		if constexpr (Config::fluidsSeepDiagonalModifier != 0)
		{
			for(BlockIndex adjacent : area.getBlocks().getEdgeAndCornerAdjacentOnly(block))
				if(area.getBlocks().fluid_canEnterEver(adjacent))
					possiblyNoLongerDiagonal.maybeAdd(adjacent);
		}
	}
	if constexpr (Config::fluidsSeepDiagonalModifier != 0)
		for(BlockIndex block : possiblyNoLongerDiagonal)
			for(BlockIndex doubleDiagonal : area.getBlocks().getEdgeAdjacentOnSameZLevelOnly(block))
				if(futureBlocks.contains(doubleDiagonal))
					m_diagonalBlocks.maybeRemove(block);
	for(BlockIndex block : adjacentToFutureEmpty)
		// If block won't be empty then check for forming a new group as it may be detached.
		if(futureBlocks.contains(block))
			potentialNewGroups.maybeAdd(block);
		// Else check for removal from empty adjacent queue.
		else
			possiblyNoLongerAdjacent.maybeAdd(block);
	// Seperate into contiguous groups. Each block in potentialNewGroups might be in a seperate group.
	// If there is only one potential new group there can not be a split: there needs to be another group to split from.
	potentialNewGroups.erase_if([&](const BlockIndex& block){ return !futureBlocks.contains(block); });
	if(potentialNewGroups.size() > 1)
	{
		BlockIndices closed;
		//closed.reserve(futureBlocks.size());
		for(BlockIndex block : potentialNewGroups)
		{
			if(closed.contains(block))
				continue;
			auto condition = [&](BlockIndex& block){ return futureBlocks.contains(block); };
			BlockIndices adjacents = blocks.collectAdjacentsWithCondition(block, condition);
			// Add whole group to closed. There is only one iteration per group as others will be rejected by the closed guard.
			closed.merge(adjacents);
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
		for(BlockIndex block : m_futureNewEmptyAdjacents)
			for(BlockIndex adjacent : blocks.getDirectlyAdjacent(block))
				if(adjacent.exists() && area.getBlocks().fluid_canEnterEver(adjacent))
				{
					for(FluidGroupSplitData& fluidGroupSplitData : m_futureGroups)
						if(fluidGroupSplitData.members.contains(adjacent))
							fluidGroupSplitData.futureAdjacent.maybeAdd(block);
				}
	}
	// -Find no longer adjacent empty to remove from fill queue and newEmptyAdjacent.
	BlockIndices futureRemoveFromEmptyAdjacents;
	//futureRemoveFromEmptyAdjacents.reserve(possiblyNoLongerAdjacent.size());
	for(BlockIndex block : possiblyNoLongerAdjacent)
	{
		if(futureBlocks.contains(block))
			continue;
		bool stillAdjacent = false;
		for(BlockIndex adjacent : blocks.getDirectlyAdjacent(block))
			if(adjacent.exists() && area.getBlocks().fluid_canEnterEver(adjacent) && futureBlocks.contains(adjacent))
			{
				stillAdjacent = true;
				break;
			}
		if(!stillAdjacent)
			futureRemoveFromEmptyAdjacents.add(block);
	}
	// Convert descriptive future to proscriptive future.
	m_futureAddToDrainQueue = m_fillQueue.m_futureNoLongerEmpty;
	m_futureRemoveFromDrainQueue = m_drainQueue.m_futureEmpty;
	//m_futureAddToFillQueue = (m_futureNoLongerFull - futureRemoveFromEmptyAdjacents) + m_futureNewEmptyAdjacents;
	//m_futureAddToFillQueue.reserve(m_futureNewEmptyAdjacents.size() + m_drainQueue.m_futureNoLongerFull.size());
	m_futureAddToFillQueue = m_futureNewEmptyAdjacents;
	for(BlockIndex block : m_drainQueue.m_futureNoLongerFull)
		if(!futureRemoveFromEmptyAdjacents.contains(block))
			m_futureAddToFillQueue.add(block);
	//m_futureRemoveFromFillQueue = futureRemoveFromEmptyAdjacents + m_futureFull;
	//m_futureRemoveFromFillQueue.reserve(futureRemoveFromEmptyAdjacents.size() + m_fillQueue.m_futureFull.size());
	m_futureRemoveFromFillQueue = futureRemoveFromEmptyAdjacents;
	m_futureRemoveFromFillQueue.merge(m_fillQueue.m_futureFull);
	validate(area);
}
void FluidGroup::writeStep(Area& area)
{
	assert(!m_merged);
	assert(!m_disolved);
	assert(!m_destroy);
	validate(area);
	auto& blocks = area.getBlocks();
	area.m_hasFluidGroups.validateAllFluidGroups();
	m_drainQueue.applyDelta(area, *this);
	m_fillQueue.applyDelta(area, *this);
	// Update queues.
	validate(area);
	for([[maybe_unused]] BlockIndex block : m_futureAddToFillQueue)
		assert(!m_futureRemoveFromFillQueue.contains(block));
	// Don't add to drain queue if taken by another fluid group already.
	m_futureAddToDrainQueue.erase_if([&](const BlockIndex& block){
		auto found = blocks.fluid_getData(block, m_fluidType);
		return found && found->group != this;
	});
	for([[maybe_unused]] BlockIndex block : m_futureAddToDrainQueue)
		assert(!m_futureRemoveFromDrainQueue.contains(block));
	for(BlockIndex block : m_futureRemoveFromFillQueue)
	{
		bool tests = (
			blocks.fluid_contains(block, m_fluidType) ||
			blocks.fluid_volumeOfTypeContains(block, m_fluidType) == Config::maxBlockVolume ||
			blocks.fluid_getGroup(block, m_fluidType) != this
		);
		assert(tests || !m_futureGroups.empty());
		if(!tests && !m_futureGroups.empty())
		{
			[[maybe_unused]] bool found = false;
			for(FluidGroupSplitData& fluidGroupSplitData : m_futureGroups)
				if(fluidGroupSplitData.members.contains(block))
				{
					found = true;
					break;
				}
			assert(found);
		}
	}
	for(BlockIndex block : m_futureRemoveFromDrainQueue)
	{
		bool tests = (
			!blocks.fluid_contains(block, m_fluidType) ||
			blocks.fluid_getGroup(block, m_fluidType) != this
		);
		assert(tests || !m_futureGroups.empty());
		if(!tests && !m_futureGroups.empty())
		{
			[[maybe_unused]] bool found = false;
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
	m_fillQueue.maybeAddBlocks(m_futureAddToFillQueue);
	m_drainQueue.removeBlocks(m_futureRemoveFromDrainQueue);
	m_drainQueue.maybeAddBlocks(m_futureAddToDrainQueue);
	validate(area);
	area.m_hasFluidGroups.validateAllFluidGroups();
}
void FluidGroup::afterWriteStep(Area& area)
{
	// Any fluid group could be marked as disolved or destroyed during iteration.
	if(m_disolved || m_destroy)
		return;
	validate(area);
	assert(!m_merged);
	auto& blocks = area.getBlocks();
	// Do seeping through corners if enabled.
	if constexpr(Config::fluidsSeepDiagonalModifier != 0)
	{
		if(m_viscosity != 0 && !m_drainQueue.m_set.empty() && !m_diagonalBlocks.empty())
		{
			for(BlockIndex diagonal : m_diagonalBlocks)
			{
				if(blocks.fluid_canEnterCurrently(diagonal, m_fluidType))
				{
					uint32_t diagonalPriority = (blocks.getZ(diagonal).get() * Config::maxBlockVolume.get() * 2) + blocks.fluid_volumeOfTypeContains(diagonal, m_fluidType).get();
					BlockIndex topBlock = m_drainQueue.m_queue[0].block;
					uint32_t topPriority = (blocks.getZ(topBlock).get() * Config::maxBlockVolume.get() * 2) + blocks.fluid_volumeOfTypeContains(topBlock, m_fluidType).get() + m_excessVolume;
					if(topPriority > diagonalPriority)
					{
						CollisionVolume flow = CollisionVolume::create(std::min({
							m_viscosity,
							(topPriority - diagonalPriority)/2,
							blocks.fluid_volumeOfTypeCanEnter(diagonal, m_fluidType).get(),
							std::max(1u, FluidType::getViscosity(m_fluidType) / Config::fluidsSeepDiagonalModifier)
						}));
						m_excessVolume -= flow.get();
						m_viscosity -= flow.get();
						if(m_viscosity <= 0)
							break;
						blocks.fluid_add(diagonal, flow, m_fluidType);
					}
				}
			}
		}
	}
	// Resolve overfull blocks.
	validate(area);
	for(BlockIndex block : m_fillQueue.m_overfull)
		if(blocks.fluid_getTotalVolume(block) > Config::maxBlockVolume)
			blocks.fluid_resolveOverfull(block);
	validate(area);
	for(BlockIndex block : m_fillQueue.m_futureNoLongerEmpty)
		addMistFor(area, block);
	validate(area);
}
void FluidGroup::splitStep(Area& area)
{
	assert(!m_merged);
	assert(!m_disolved);
	assert(!m_destroy);
	validate(area);
	// Disperse disolved, split off fluidGroups of another fluidType.
	// TODO: Transfer ownership to adjacent fluid groups with a lower density then this one but higher then disolved group.
	auto& blocks = area.getBlocks();
	std::vector<FluidTypeId> dispersed;
	for(auto& [fluidType, disolvedFluidGroup] : m_disolvedInThisGroup)
	{
		assert(disolvedFluidGroup->m_drainQueue.m_set.empty());
		assert(m_fluidType != fluidType);
		assert(disolvedFluidGroup->m_fluidType == fluidType);
		assert(disolvedFluidGroup->m_excessVolume > 0);
		for(FutureFlowBlock& futureFlowBlock : m_fillQueue.m_queue)
			if(blocks.fluid_undisolveInternal(futureFlowBlock.block, *disolvedFluidGroup))
			{
				dispersed.push_back(fluidType);
				break;
			}
	}
	validate(area);
	for(FluidTypeId fluidType : dispersed)
		m_disolvedInThisGroup.erase(fluidType);
	// Split off future groups of this fluidType.
	if(m_futureGroups.empty() || m_futureGroups.size() == 1)
		return;
	BlockIndices formerMembers;
	for(BlockIndex block : m_drainQueue.m_set)
		if(!m_futureGroups.back().members.contains(block))
		{
			formerMembers.add(block);
			// Unset recorded group from blocks which are about to be split off so the fluidGroup constructor does not try to remove them again.
			blocks.fluid_unsetGroupInternal(block, m_fluidType);
		}
	m_drainQueue.removeBlocks(formerMembers);
	BlockIndices formerFill;
	for(BlockIndex block : m_fillQueue.m_set)
		if(!m_drainQueue.m_set.contains(block) && !blocks.isAdjacentToAny(block, m_drainQueue.m_set))
			formerFill.add(block);
	m_fillQueue.removeBlocks(formerFill);
	m_futureNewEmptyAdjacents = m_futureGroups.back().futureAdjacent;
	if constexpr (Config::fluidsSeepDiagonalModifier != 0)
		m_diagonalBlocks.erase_if([&](const BlockIndex& block){
			for(BlockIndex diagonal : area.getBlocks().getEdgeAdjacentOnSameZLevelOnly(block))
				if(m_futureGroups.back().members.contains(diagonal))
					return false;
			return true;
		});
	// Remove largest group, it will remain as this instance.
	m_futureGroups.pop_back();
	// Create new groups.
	for(auto& [members, newAdjacent] : m_futureGroups)
	{
		FluidGroup* fluidGroup = area.m_hasFluidGroups.createFluidGroup(m_fluidType, members, false);
		fluidGroup->m_futureNewEmptyAdjacents.swap(newAdjacent);
	}
	validate(area);
}
// Do all split prior to doing merge.
void FluidGroup::mergeStep(Area& area)
{
	assert(!m_disolved);
	assert(!m_destroy);
	validate(area);
	auto& blocks = area.getBlocks();
	// Record merge. First group consumes subsequent groups.
	for(BlockIndex block : m_futureNewEmptyAdjacents)
	{
		// Fluid groups may be marked merged during merge iteration.
		if(m_merged)
			return;
		auto found = blocks.fluid_getData(block, m_fluidType);
		if(!found)
			continue;
		found->group->validate(area);
		assert(!found->group->m_merged);
		assert(found->group->m_fluidType == m_fluidType);
		if(found->group != this)
		{
			merge(area, found->group);
			continue;
		}
	}
	validate(area);
}
CollisionVolume FluidGroup::totalVolume(Area& area) const
{
	CollisionVolume output = CollisionVolume::create(m_excessVolume);
	for(BlockIndex block : m_drainQueue.m_set)
		output += area.getBlocks().fluid_volumeOfTypeContains(block, m_fluidType);
	return output;
}
bool FluidGroup::dispositionIsStable(Area& area, const CollisionVolume& fillVolume, const CollisionVolume& drainVolume) const
{
	auto& blocks = area.getBlocks();
	DistanceInBlocks drainZ = blocks.getZ(m_drainQueue.m_groupStart->block);
	DistanceInBlocks fillZ = blocks.getZ(m_fillQueue.m_groupStart->block);
	if(drainZ < fillZ)
		return true;
	if(drainZ == fillZ &&
		(
			fillVolume >= drainVolume ||
			(
				fillVolume == 0 &&
				(
					drainVolume == 1 ||
					drainVolume * m_drainQueue.groupSize() < (m_fillQueue.groupSize() + m_drainQueue.groupSize())
				)
			)
		)
	)
		return true;
	return false;
}
void FluidGroup::validate(Area& area) const
{
	assert(area.m_hasFluidGroups.contains(*this));
	if(m_merged || m_destroy || m_disolved)
		return;
	for(BlockIndex block : m_drainQueue.m_set)
	{
		assert(block.exists());
		for(const FluidData& fluidData : area.getBlocks().fluid_getAll(block))
		{
			if(fluidData.group == this)
				continue;
			assert(fluidData.group->m_fluidType == fluidData.type);
			assert(fluidData.group->m_drainQueue.m_set.contains(block));
		}
	}
}
void FluidGroup::validate(Area& area, SmallSet<FluidGroup*> toErase) const
{
	if(m_merged || m_destroy || m_disolved)
		return;
	for(BlockIndex block : m_drainQueue.m_set)
		for(const FluidData& fluidData : area.getBlocks().fluid_getAll(block))
		{
			if(fluidData.group == this)
				continue;
			assert(fluidData.group->m_fluidType == fluidData.type);
			assert(fluidData.group->m_drainQueue.m_set.contains(block));
			assert(!toErase.contains(fluidData.group));
		}
}
void FluidGroup::log(Area& area) const
{
	Blocks& blocks = area.getBlocks();
	for(const FutureFlowBlock& futureFlowBlock : m_drainQueue.m_queue)
	{
		if(&*m_drainQueue.m_groupEnd == &futureFlowBlock)
			std::cout << "drain group end\n";
		blocks.getCoordinates(futureFlowBlock.block).log();
		std::cout <<
			"d:" << futureFlowBlock.delta.get() << " " <<
			"c:" << futureFlowBlock.capacity.get() << " " <<
			"v:" << blocks.fluid_volumeOfTypeContains(futureFlowBlock.block, m_fluidType).get() << " " <<
			"t:" << blocks.fluid_getTotalVolume(futureFlowBlock.block).get();
		std::cout << '\n';
	}
	std::cout << "total:" << totalVolume(area).get() << '\n';
	if(m_excessVolume)
		std::cout << "excess:" << m_excessVolume << '\n';
	if(m_merged)
		std::cout << "merged" << '\n';
	if(m_destroy)
		std::cout << "destroy" << '\n';
	if(m_disolved)
		std::cout << "disolved" << '\n';
	if(m_stable)
		std::cout << "stable" << '\n';
	std::cout << std::endl;
}
void FluidGroup::logFill(Area& area) const
{
	Blocks& blocks = area.getBlocks();
	for(const FutureFlowBlock& futureFlowBlock : m_fillQueue.m_queue)
	{
		if(&*m_fillQueue.m_groupEnd == &futureFlowBlock)
			std::cout << "fill group end\n";
		blocks.getCoordinates(futureFlowBlock.block).log();
		std::cout <<
			"d:" << futureFlowBlock.delta.get() << " " <<
			"c:" << futureFlowBlock.capacity.get() << " " <<
			"v:" << blocks.fluid_volumeOfTypeContains(futureFlowBlock.block, m_fluidType).get() << " " <<
			"t:" << blocks.fluid_getTotalVolume(futureFlowBlock.block).get();
		std::cout << std::endl;
	}
}