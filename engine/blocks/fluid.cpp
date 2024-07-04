#include "blocks.h"
#include "../fluidType.h"
#include "../mistDisperseEvent.h"
#include "../fluidGroup.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "../types.h"
/*
void Blocks::fluid_load(const Json& data, [[maybe_unused]] const DeserializationMemo& deserializationMemo)
{
		for(const Json& pair : data)
			addFluid(pair[1].get<Volume>(), *pair[0].get<const FluidType*>());
}
Json Blocks::fluid_toJson() const
{
	Json data = Json::array();
	for(auto& [fluidType, pair] : m_fluids)
		data.push_back({fluidType, pair.first});
	return data;
}
*/
std::vector<FluidData>::iterator Blocks::fluid_getDataIterator(BlockIndex index, const FluidType& fluidType)
{
	auto& fluid = m_fluid.at(index);
	return std::ranges::find(fluid, &fluidType, &FluidData::type);
}
const FluidData* Blocks::fluid_getData(BlockIndex index, const FluidType& fluidType) const
{
	return const_cast<Blocks*>(this)->fluid_getData(index, fluidType);
}
FluidData* Blocks::fluid_getData(BlockIndex index, const FluidType& fluidType)
{
	auto& fluid = m_fluid.at(index);
	auto iter = fluid_getDataIterator(index, fluidType);
	if(iter == fluid.end())
		return nullptr;
	return &*iter;
}
void Blocks::fluid_destroyData(BlockIndex index, const FluidType& fluidType)
{
	auto& fluid = m_fluid.at(index);
	auto iter = std::ranges::find(fluid, &fluidType, &FluidData::type);
	assert(iter != fluid.end());
	std::swap(*iter, fluid.back());
	fluid.pop_back();
}
void Blocks::fluid_spawnMist(BlockIndex index, const FluidType& fluidType, DistanceInBlocks maxMistSpread)
{
	const FluidType* mist = m_mist.at(index);
	if(mist != nullptr && (mist == &fluidType || mist->density > fluidType.density))
		return;
	m_mist[index] = &fluidType;
	m_mistInverseDistanceFromSource[index] = maxMistSpread != 0 ? maxMistSpread : fluidType.maxMistSpread;
	MistDisperseEvent::emplace(m_area, fluidType.mistDuration, fluidType, index);
}
void Blocks::fluid_clearMist(BlockIndex index)
{
	m_mist[index] = nullptr;
	m_mistInverseDistanceFromSource[index] = 0;
}
DistanceInBlocks Blocks::fluid_getMistInverseDistanceToSource(BlockIndex index) const
{
	return m_mistInverseDistanceFromSource.at(index);
}
void Blocks::fluid_mistSetFluidTypeAndInverseDistance(BlockIndex index, const FluidType& fluidType, DistanceInBlocks inverseDistance)
{
	m_mist[index] = &fluidType;
	m_mistInverseDistanceFromSource[index] = inverseDistance;
}
FluidGroup* Blocks::fluid_getGroup(BlockIndex index, const FluidType& fluidType) const
{
	auto found = fluid_getData(index, fluidType);
	if(!found)
		return nullptr;
	assert(*found->type == fluidType);
	return found->group;
}
void Blocks::fluid_add(BlockIndex index, CollisionVolume volume, const FluidType& fluidType)
{
	assert(!solid_is(index));
	// If a suitable fluid group exists already then just add to it's excessVolume.
	auto found = fluid_getData(index, fluidType);
	if(found)
	{
		assert(found->type == &fluidType);
		found->group->addFluid(volume);
		return;
	}
	m_fluid.at(index).emplace_back(&fluidType, nullptr, volume);
	m_totalFluidVolume.at(index) += volume;
	// Find fluid group.
	FluidGroup* fluidGroup = nullptr;
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent != BLOCK_INDEX_MAX && fluid_canEnterEver(adjacent) && fluid_contains(adjacent, fluidType))
		{
			fluidGroup = fluid_getGroup(adjacent, fluidType);
			fluidGroup->addBlock(index, true);
			break;
		}
	// Create fluid group.
	if(fluidGroup == nullptr)
	{
		std::unordered_set<BlockIndex> blocks({index});
		fluidGroup = m_area.m_hasFluidGroups.createFluidGroup(fluidType, blocks);
	}
	else
		m_area.m_hasFluidGroups.clearMergedFluidGroups();
	// Shift less dense fluids to excessVolume.
	if(m_totalFluidVolume.at(index) > Config::maxBlockVolume)
		fluid_resolveOverfull(index);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
}
void Blocks::fluid_setAllUnstableExcept(BlockIndex index, const FluidType& fluidType)
{
	for(FluidData& fluidData : m_fluid.at(index))
		if(*fluidData.type != fluidType)
			fluidData.group->m_stable = false;
}
void Blocks::fluid_drainInternal(BlockIndex index, CollisionVolume volume, const FluidType& fluidType)
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
		m_fluid.at(index).emplace_back(&fluidGroup.m_fluidType, &fluidGroup, volume);
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
		uint32_t capacity = fluid_volumeOfTypeCanEnter(index, fluidGroup.m_fluidType);
		uint32_t flow = std::min(capacity, (uint32_t)fluidGroup.m_excessVolume);
		m_totalFluidVolume[index] += flow;
		fluidGroup.m_excessVolume -= flow;
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
			m_fluid.at(index).emplace_back(&fluidGroup.m_fluidType, &fluidGroup, flow);
			fluidGroup.addBlock(index, false);
			fluidGroup.m_disolved = false;
		}
		//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
		m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
		return true;
	}
	return false;
}
void Blocks::fluid_remove(BlockIndex index, CollisionVolume volume, const FluidType& fluidType)
{
	assert(volume <= fluid_volumeOfTypeContains(index, fluidType));
	fluid_getGroup(index, fluidType)->removeFluid(volume);
}
void Blocks::fluid_removeSyncronus(BlockIndex index, CollisionVolume volume, const FluidType& fluidType)
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
bool Blocks::fluid_canEnterCurrently(BlockIndex index, const FluidType& fluidType) const
{
	if(m_totalFluidVolume.at(index) < Config::maxBlockVolume)
		return true;
	for(const FluidData& fluidData : m_fluid.at(index))
		if(fluidData.type->density < fluidType.density)
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
		if(adjacent != BLOCK_INDEX_MAX && fluid_contains(adjacent, fluidGroup->m_fluidType) && fluid_getGroup(adjacent, fluidGroup->m_fluidType) == fluidGroup)
			return true;
	return false;
}
CollisionVolume Blocks::fluid_volumeOfTypeCanEnter(BlockIndex index, const FluidType& fluidType) const
{
	CollisionVolume output = Config::maxBlockVolume;
	for(const FluidData& fluidData : m_fluid.at(index))
		if(fluidData.type->density >= fluidType.density)
			output -= fluidData.volume;
	return output;
}
CollisionVolume Blocks::fluid_volumeOfTypeContains(BlockIndex index, const FluidType& fluidType) const
{
	const auto found = fluid_getData(index, fluidType);
	if(!found)
		return 0;
	return found->volume;
}
const FluidType& Blocks::fluid_getTypeWithMostVolume(BlockIndex index) const
{
	assert(!m_fluid.at(index).empty());
	CollisionVolume volume = 0;
	FluidType* output = nullptr;
	for(const FluidData& fluidData: m_fluid.at(index))
		if(volume < fluidData.volume)
			output = const_cast<FluidType*>(fluidData.type);
	assert(output != nullptr);
	return *output;
}
const FluidType* Blocks::fluid_getMist(BlockIndex index) const
{
	return m_mist.at(index);
}
void Blocks::fluid_resolveOverfull(BlockIndex index)
{
	std::vector<const FluidType*> toErase;
	// Fluid types are sorted by density.
	for(FluidData& fluidData : fluid_getAllSortedByDensityAscending(index))
	{

		assert(*fluidData.type == fluidData.group->m_fluidType);
		// Displace lower density fluids.
		CollisionVolume displaced = std::min(fluidData.volume, m_totalFluidVolume.at(index) - Config::maxBlockVolume);
		assert(displaced);
		m_totalFluidVolume.at(index) -= displaced;
		fluidData.volume -= displaced;
		fluidData.group->addFluid(displaced);
		if(!fluidData.volume)
			toErase.push_back(fluidData.type);
		if(fluidData.volume < Config::maxBlockVolume)
			fluidData.group->m_fillQueue.addBlock(index);
		if(m_totalFluidVolume.at(index) == Config::maxBlockVolume)
			break;
	}
	for(const FluidType* fluidType : toErase)
	{
		// If the last block of a fluidGroup is displaced disolve it in the lowest density liquid which is more dense then it.
		FluidGroup* fluidGroup = fluid_getGroup(index, *fluidType);
		assert(fluidGroup->m_fluidType == *fluidType);
		fluidGroup->removeBlock(index);
		if(fluidGroup->m_drainQueue.m_set.empty())
		{
			for(FluidData& otherFluidData : m_fluid.at(index))
				if(otherFluidData.type->density > fluidType->density)
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
		fluid_destroyData(index, *fluidType);
	}
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
}
void Blocks::fluid_onBlockSetSolid(BlockIndex index)
{
	// Displace fluids.
	m_totalFluidVolume.at(index) = 0;
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
				while(above != BLOCK_INDEX_MAX)
				{
					if(fluid_canEnterEver(above) && fluid_canEnterCurrently(above, *fluidData.type))
					{
						fluidData.group->m_fillQueue.addBlock(above);
						break;
					}
					above = getBlockAbove(above);
				}
				// The only way that this 'piston' code should be triggered is if something falls, which means the top layer cannot be full.
				assert(above != BLOCK_INDEX_MAX);
			}
			else
				// Otherwise destroy the group.
				m_area.m_hasFluidGroups.removeFluidGroup(*fluidData.group);
		}
	}
	m_fluid.at(index).clear();
	// Remove from fluid fill queues.
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent != BLOCK_INDEX_MAX && fluid_canEnterEver(adjacent))
			for(FluidData& fluidData : m_fluid.at(adjacent))
				fluidData.group->m_fillQueue.removeBlock(index);
}
void Blocks::fluid_onBlockSetNotSolid(BlockIndex index)
{
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent != BLOCK_INDEX_MAX && fluid_canEnterEver(adjacent))
			for(FluidData& fluidData : m_fluid[adjacent])
			{
				fluidData.group->m_fillQueue.addBlock(index);
				fluidData.group->m_stable = false;
			}
}
bool Blocks::fluid_contains(BlockIndex index, const FluidType& fluidType) const
{
	auto fluid = m_fluid.at(index);
	return std::ranges::find(fluid, &fluidType, &FluidData::type) != fluid.end();
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
	std::ranges::sort(m_fluid.at(index), std::less<Density>(), [](const auto& data) { return data.type->density; });
	return m_fluid.at(index);
}
bool Blocks::fluid_any(BlockIndex index) const
{
	return !m_fluid.at(index).empty();
}
