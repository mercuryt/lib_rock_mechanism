#include "blocks.h"
#include "../fluidType.h"
#include "../mistDisperseEvent.h"
#include "../fluidGroup.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "../types.h"
std::vector<FluidData>::iterator Blocks::fluid_getDataIterator(BlockIndex index, FluidTypeId fluidType)
{
	auto& fluid = m_fluid.at(index);
	return std::ranges::find(fluid, fluidType, &FluidData::type);
}
const FluidData* Blocks::fluid_getData(BlockIndex index, FluidTypeId fluidType) const
{
	return const_cast<Blocks*>(this)->fluid_getData(index, fluidType);
}
FluidData* Blocks::fluid_getData(BlockIndex index, FluidTypeId fluidType)
{
	auto& fluid = m_fluid.at(index);
	auto iter = fluid_getDataIterator(index, fluidType);
	if(iter == fluid.end())
		return nullptr;
	return &*iter;
}
void Blocks::fluid_destroyData(BlockIndex index, FluidTypeId fluidType)
{
	auto& fluid = m_fluid.at(index);
	auto iter = std::ranges::find(fluid, fluidType, &FluidData::type);
	assert(iter != fluid.end());
	std::swap(*iter, fluid.back());
	fluid.pop_back();
}
void Blocks::fluid_spawnMist(BlockIndex index, FluidTypeId fluidType, DistanceInBlocks maxMistSpread)
{
	FluidTypeId mist = m_mist.at(index);
	if(mist.exists() && (mist == fluidType || FluidType::getDensity(mist) > FluidType::getDensity(fluidType)))
		return;
	m_mist.at(index) = fluidType;
	m_mistInverseDistanceFromSource.at(index) = maxMistSpread != 0 ? maxMistSpread : FluidType::getMaxMistSpread(fluidType);
	MistDisperseEvent::emplace(m_area, FluidType::getMistDuration(fluidType), fluidType, index);
}
void Blocks::fluid_clearMist(BlockIndex index)
{
	m_mist.at(index).clear();
	m_mistInverseDistanceFromSource.at(index) = DistanceInBlocks::create(0);
}
DistanceInBlocks Blocks::fluid_getMistInverseDistanceToSource(BlockIndex index) const
{
	return m_mistInverseDistanceFromSource.at(index);
}
void Blocks::fluid_mistSetFluidTypeAndInverseDistance(BlockIndex index, FluidTypeId fluidType, DistanceInBlocks inverseDistance)
{
	m_mist.at(index) = fluidType;
	m_mistInverseDistanceFromSource.at(index) = inverseDistance;
}
FluidGroup* Blocks::fluid_getGroup(BlockIndex index, FluidTypeId fluidType) const
{
	auto found = fluid_getData(index, fluidType);
	if(!found)
		return nullptr;
	assert(found->type == fluidType);
	return found->group;
}
void Blocks::fluid_add(BlockIndex index, CollisionVolume volume, FluidTypeId fluidType)
{
	assert(!solid_is(index));
	// If a suitable fluid group exists already then just add to it's excessVolume.
	auto found = fluid_getData(index, fluidType);
	if(found)
	{
		assert(found->type == fluidType);
		found->group->addFluid(volume);
		return;
	}
	m_fluid.at(index).emplace_back(fluidType, nullptr, volume);
	m_totalFluidVolume.at(index) += volume;
	// Find fluid group.
	FluidGroup* fluidGroup = nullptr;
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent.exists() && fluid_canEnterEver(adjacent) && fluid_contains(adjacent, fluidType))
		{
			fluidGroup = fluid_getGroup(adjacent, fluidType);
			fluidGroup->addBlock(index, true);
			break;
		}
	// Create fluid group.
	if(fluidGroup == nullptr)
	{
		BlockIndices blocks({index});
		fluidGroup = m_area.m_hasFluidGroups.createFluidGroup(fluidType, blocks);
	}
	else
		m_area.m_hasFluidGroups.clearMergedFluidGroups();
	// Shift less dense fluids to excessVolume.
	if(m_totalFluidVolume.at(index) > Config::maxBlockVolume)
		fluid_resolveOverfull(index);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
}
void Blocks::fluid_setAllUnstableExcept(BlockIndex index, FluidTypeId fluidType)
{
	for(FluidData& fluidData : m_fluid.at(index))
		if(fluidData.type != fluidType)
			fluidData.group->m_stable = false;
}
void Blocks::fluid_drainInternal(BlockIndex index, CollisionVolume volume, FluidTypeId fluidType)
{
	auto iter = fluid_getDataIterator(index, fluidType);
	assert(iter != m_fluid.at(index).end());
	if(iter->volume == volume)
		// use Vector::erase rather then util::removeFromVectorByValueUnordered despite being slower to preserve sort order.
		m_fluid.at(index).erase(iter);
	else
		iter->volume -= volume;
	assert(m_totalFluidVolume.at(index) >= volume);
	m_totalFluidVolume.at(index) -= volume;
	//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
}
void Blocks::fluid_fillInternal(BlockIndex index, CollisionVolume volume, FluidGroup& fluidGroup)
{
	FluidData* found = fluid_getData(index, fluidGroup.m_fluidType);
	if(!found)
		m_fluid.at(index).emplace_back(fluidGroup.m_fluidType, &fluidGroup, volume);
	else
	{
		found->volume += volume;
		//TODO: should this be enforced?
		//assert(*found->group == fluidGroup);
	}
	m_totalFluidVolume.at(index) += volume;
	//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
}
bool Blocks::fluid_undisolveInternal(BlockIndex index, FluidGroup& fluidGroup)
{
	FluidData* found = fluid_getData(index, fluidGroup.m_fluidType);
	if(found || fluid_canEnterCurrently(index, fluidGroup.m_fluidType))
	{
		CollisionVolume capacity = fluid_volumeOfTypeCanEnter(index, fluidGroup.m_fluidType);
		assert(fluidGroup.m_excessVolume > 0);
		CollisionVolume flow = std::min(capacity, CollisionVolume::create(fluidGroup.m_excessVolume));
		m_totalFluidVolume.at(index) += flow;
		fluidGroup.m_excessVolume -= flow.get();
		if(found )
		{
			// Merge disolved group into found group.
			found->volume += flow;
			found->group->m_excessVolume += fluidGroup.m_excessVolume;
			fluidGroup.m_destroy = true;
		}
		else
		{
			// Undisolve group.
			m_fluid.at(index).emplace_back(fluidGroup.m_fluidType, &fluidGroup, flow);
			fluidGroup.addBlock(index, false);
			fluidGroup.m_disolved = false;
		}
		//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
		m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
		return true;
	}
	return false;
}
void Blocks::fluid_remove(BlockIndex index, CollisionVolume volume, FluidTypeId fluidType)
{
	assert(volume <= fluid_volumeOfTypeContains(index, fluidType));
	fluid_getGroup(index, fluidType)->removeFluid(volume);
}
void Blocks::fluid_removeSyncronus(BlockIndex index, CollisionVolume volume, FluidTypeId fluidType)
{
	assert(volume <= fluid_volumeOfTypeContains(index, fluidType));
	assert(m_totalFluidVolume.at(index) >= volume);
	m_totalFluidVolume.at(index) -= volume;
	if(fluid_volumeOfTypeContains(index, fluidType) > volume)
		fluid_getData(index, fluidType)->volume -= volume;
	else
	{
		FluidGroup& group = *fluid_getGroup(index, fluidType);
		fluid_destroyData(index, fluidType);
		if(group.getBlocks().size() == 1)
			m_area.m_hasFluidGroups.removeFluidGroup(group);
		else
			group.removeBlock(index);
	}
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
}
bool Blocks::fluid_canEnterCurrently(BlockIndex index, FluidTypeId fluidType) const
{
	if(m_totalFluidVolume.at(index) < Config::maxBlockVolume)
		return true;
	for(const FluidData& fluidData : m_fluid.at(index))
		if(FluidType::getDensity(fluidData.type) < FluidType::getDensity(fluidType))
			return true;
	return false;
}
bool Blocks::fluid_canEnterEver(BlockIndex index) const
{
	return !solid_is(index);
}
bool Blocks::fluid_isAdjacentToGroup(BlockIndex index, const FluidGroup* fluidGroup) const
{
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent.exists() && fluid_contains(adjacent, fluidGroup->m_fluidType) && fluid_getGroup(adjacent, fluidGroup->m_fluidType) == fluidGroup)
			return true;
	return false;
}
CollisionVolume Blocks::fluid_volumeOfTypeCanEnter(BlockIndex index, FluidTypeId fluidType) const
{
	CollisionVolume output = Config::maxBlockVolume;
	for(const FluidData& fluidData : m_fluid.at(index))
		if(FluidType::getDensity(fluidData.type) >= FluidType::getDensity(fluidType))
			output -= fluidData.volume;
	return output;
}
CollisionVolume Blocks::fluid_volumeOfTypeContains(BlockIndex index, FluidTypeId fluidType) const
{
	const auto found = fluid_getData(index, fluidType);
	if(!found)
		return CollisionVolume::create(0);
	return found->volume;
}
FluidTypeId Blocks::fluid_getTypeWithMostVolume(BlockIndex index) const
{
	assert(!m_fluid.at(index).empty());
	CollisionVolume volume = CollisionVolume::create(0);
	FluidTypeId output;
	for(const FluidData& fluidData: m_fluid.at(index))
		if(volume < fluidData.volume)
			output = fluidData.type;
	assert(output.exists());
	return output;
}
FluidTypeId Blocks::fluid_getMist(BlockIndex index) const
{
	return m_mist.at(index);
}
void Blocks::fluid_resolveOverfull(BlockIndex index)
{
	std::vector<FluidTypeId> toErase;
	// Fluid types are sorted by density.
	for(FluidData& fluidData : fluid_getAllSortedByDensityAscending(index))
	{

		assert(fluidData.type == fluidData.group->m_fluidType);
		// Displace lower density fluids.
		CollisionVolume displaced = std::min(fluidData.volume, m_totalFluidVolume.at(index) - Config::maxBlockVolume);
		assert(displaced.exists());
		m_totalFluidVolume.at(index) -= displaced;
		fluidData.volume -= displaced;
		fluidData.group->addFluid(displaced);
		if(fluidData.volume.empty())
			toErase.push_back(fluidData.type);
		if(fluidData.volume < Config::maxBlockVolume)
			fluidData.group->m_fillQueue.addBlock(index);
		if(m_totalFluidVolume.at(index) == Config::maxBlockVolume)
			break;
	}
	for(FluidTypeId fluidType : toErase)
	{
		// If the last block of a fluidGroup is displaced disolve it in the lowest density liquid which is more dense then it.
		FluidGroup* fluidGroup = fluid_getGroup(index, fluidType);
		assert(fluidGroup->m_fluidType == fluidType);
		fluidGroup->removeBlock(index);
		if(fluidGroup->m_drainQueue.m_set.empty())
		{
			for(FluidData& otherFluidData : m_fluid.at(index))
				if(FluidType::getDensity(otherFluidData.type) > FluidType::getDensity(fluidType))
				{
					//TODO: find.
					if(otherFluidData.group->m_disolvedInThisGroup.contains(fluidType))
					{
						otherFluidData.group->m_disolvedInThisGroup.at(fluidType)->m_excessVolume += fluidGroup->m_excessVolume;
						fluidGroup->m_destroy = true;
					}
					else
					{
						otherFluidData.group->m_disolvedInThisGroup[fluidType] = fluidGroup;
						fluidGroup->m_disolved = true;
					}
					break;
				}
			assert(fluidGroup->m_disolved || fluidGroup->m_destroy);
		}
		fluid_destroyData(index, fluidType);
	}
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
}
void Blocks::fluid_onBlockSetSolid(BlockIndex index)
{
	// Displace fluids.
	m_totalFluidVolume.at(index) = CollisionVolume::create(0);
	for(FluidData& fluidData : m_fluid.at(index))
	{
		fluidData.group->removeBlock(index);
		fluidData.group->addFluid(fluidData.volume);
		// If there is no where to put the fluid.
		if(fluidData.group->m_drainQueue.m_set.empty() && fluidData.group->m_fillQueue.m_set.empty())
		{
			// If fluid piston is enabled then find a place above to add to potential.
			if constexpr (Config::fluidPiston)
			{
				BlockIndex above = getBlockAbove(index);
				while(above.exists())
				{
					if(fluid_canEnterEver(above) && fluid_canEnterCurrently(above, fluidData.type))
					{
						fluidData.group->m_fillQueue.addBlock(above);
						break;
					}
					above = getBlockAbove(above);
				}
				// The only way that this 'piston' code should be triggered is if something falls, which means the top layer cannot be full.
				assert(above.exists());
			}
			else
				// Otherwise destroy the group.
				m_area.m_hasFluidGroups.removeFluidGroup(*fluidData.group);
		}
	}
	m_fluid.at(index).clear();
	// Remove from fluid fill queues.
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent.exists() && fluid_canEnterEver(adjacent))
			for(FluidData& fluidData : m_fluid.at(adjacent))
				fluidData.group->m_fillQueue.removeBlock(index);
}
void Blocks::fluid_onBlockSetNotSolid(BlockIndex index)
{
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent.exists() && fluid_canEnterEver(adjacent))
			for(FluidData& fluidData : m_fluid.at(adjacent))
			{
				fluidData.group->m_fillQueue.addBlock(index);
				fluidData.group->m_stable = false;
			}
}
bool Blocks::fluid_contains(BlockIndex index, FluidTypeId fluidType) const
{
	auto fluid = m_fluid.at(index);
	return std::ranges::find(fluid, fluidType, &FluidData::type) != fluid.end();
}
CollisionVolume Blocks::fluid_getTotalVolume(BlockIndex index) const
{
	return m_totalFluidVolume.at(index);
}
std::vector<FluidData>& Blocks::fluid_getAll(BlockIndex index)
{
	return m_fluid.at(index);
}
std::vector<FluidData>& Blocks::fluid_getAllSortedByDensityAscending(BlockIndex index)
{
	std::ranges::sort(m_fluid.at(index), std::less<Density>(), [](const auto& data) { return FluidType::getDensity(data.type); });
	return m_fluid.at(index);
}
bool Blocks::fluid_any(BlockIndex index) const
{
	return !m_fluid.at(index).empty();
}
