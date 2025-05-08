#include "blocks.h"
#include "../fluidType.h"
#include "../mistDisperseEvent.h"
#include "../fluidGroups/fluidGroup.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../deserializationMemo.h"
#include "../types.h"
std::vector<FluidData>::iterator Blocks::fluid_getDataIterator(const BlockIndex& index, const FluidTypeId& fluidType)
{
	auto& fluid = m_fluid[index];
	return std::ranges::find(fluid, fluidType, &FluidData::type);
}
const FluidData* Blocks::fluid_getData(const BlockIndex& index, const FluidTypeId& fluidType) const
{
	return const_cast<Blocks*>(this)->fluid_getData(index, fluidType);
}
FluidData* Blocks::fluid_getData(const BlockIndex& index, const FluidTypeId& fluidType)
{
	auto& fluid = m_fluid[index];
	auto iter = fluid_getDataIterator(index, fluidType);
	if(iter == fluid.end())
		return nullptr;
	return &*iter;
}
void Blocks::fluid_destroyData(const BlockIndex& index, const FluidTypeId& fluidType)
{
	auto& fluid = m_fluid[index];
	auto iter = std::ranges::find(fluid, fluidType, &FluidData::type);
	assert(iter != fluid.end());
	std::swap(*iter, fluid.back());
	fluid.pop_back();
}
void Blocks::fluid_setTotalVolume(const BlockIndex& index, const CollisionVolume& newTotal)
{
	CollisionVolume checkTotal = CollisionVolume::create(0);
	for(FluidData data : m_fluid[index])
		checkTotal += data.volume;
	assert(checkTotal == newTotal);
	m_totalFluidVolume[index] = newTotal;
}
void Blocks::fluid_spawnMist(const BlockIndex& index, const FluidTypeId& fluidType, const DistanceInBlocks maxMistSpread)
{
	FluidTypeId mist = m_mist[index];
	if(mist.exists() && (mist == fluidType || FluidType::getDensity(mist) > FluidType::getDensity(fluidType)))
		return;
	m_mist[index] = fluidType;
	m_mistInverseDistanceFromSource[index] = maxMistSpread != 0 ? maxMistSpread : FluidType::getMaxMistSpread(fluidType);
	MistDisperseEvent::emplace(m_area, FluidType::getMistDuration(fluidType), fluidType, index);
}
void Blocks::fluid_clearMist(const BlockIndex& index)
{
	m_mist[index].clear();
	m_mistInverseDistanceFromSource[index] = DistanceInBlocks::create(0);
}
DistanceInBlocks Blocks::fluid_getMistInverseDistanceToSource(const BlockIndex& index) const
{
	return m_mistInverseDistanceFromSource[index];
}
void Blocks::fluid_mistSetFluidTypeAndInverseDistance(const BlockIndex& index, const FluidTypeId& fluidType, const DistanceInBlocks& inverseDistance)
{
	m_mist[index] = fluidType;
	m_mistInverseDistanceFromSource[index] = inverseDistance;
}
FluidGroup* Blocks::fluid_getGroup(const BlockIndex& index, const FluidTypeId& fluidType) const
{
	auto found = fluid_getData(index, fluidType);
	if(!found)
		return nullptr;
	assert(found->type == fluidType);
	return found->group;
}
void Blocks::fluid_add(const Cuboid& cuboid, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	for(const BlockIndex& block : cuboid.getView(*this))
		fluid_add(block, volume, fluidType);
}
void Blocks::fluid_add(const BlockIndex& index, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	assert(!solid_is(index));
	bool wasEmpty = m_totalFluidVolume[index] == 0;
	// If a suitable fluid group exists already then just add to it's excessVolume.
	auto found = fluid_getData(index, fluidType);
	if(found)
	{
		assert(found->type == fluidType);
		found->group->addFluid(m_area, volume);
		return;
	}
	m_fluid[index].emplace_back(fluidType, nullptr, volume);
	// Find fluid group.
	FluidGroup* fluidGroup = nullptr;
	for(const BlockIndex& adjacent : getDirectlyAdjacent(index))
		if(fluid_canEnterEver(adjacent) && fluid_contains(adjacent, fluidType))
		{
			fluidGroup = fluid_getGroup(adjacent, fluidType);
			fluidGroup->addBlock(m_area, index, true);
			break;
		}
	// Create fluid group.
	if(fluidGroup == nullptr)
		fluidGroup = m_area.m_hasFluidGroups.createFluidGroup(fluidType, {index});
	m_area.m_hasFluidGroups.clearMergedFluidGroups();
	fluid_setTotalVolume(index, m_totalFluidVolume[index] + volume);
	// Shift less dense fluids to excessVolume.
	if(m_totalFluidVolume[index] > Config::maxBlockVolume)
		fluid_resolveOverfull(index);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
	assert(fluidGroup != nullptr);
	if(!fluidGroup->m_aboveGround && isExposedToSky(index))
	{
		fluidGroup->m_aboveGround = true;
		if(FluidType::getFreezesInto(fluidGroup->m_fluidType).exists())
			m_area.m_hasTemperature.addFreezeableFluidGroupAboveGround(*fluidGroup);
	}
	floating_maybeFloatUp(index);
	// Float.
	Items& items = m_area.getItems();
	for(const ItemIndex& item : item_getAll(index))
	{
		const BlockIndex& location = items.getLocation(item);
		if(!items.isFloating(item))
		{
			if(items.canFloatAt(item, location))
				items.setFloating(item, fluidType);
		}
		else
		{
			const BlockIndex& above = getBlockAbove(location);
			if(items.canFloatAt(item, above))
				items.location_set(item, above, items.getFacing(item));
		}
	}
	if(wasEmpty)
		fluid_maybeRecordFluidOnDeck(index);
}
void Blocks::fluid_setAllUnstableExcept(const BlockIndex& index, const FluidTypeId& fluidType)
{
	for(FluidData& fluidData : m_fluid[index])
		if(fluidData.type != fluidType)
			fluidData.group->m_stable = false;
}
void Blocks::fluid_drainInternal(const BlockIndex& index, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	auto iter = fluid_getDataIterator(index, fluidType);
	assert(iter != m_fluid[index].end());
	if(iter->volume == volume)
		// use Vector::erase rather then util::removeFromVectorByValueUnordered despite being slower to preserve sort order.
		m_fluid[index].erase(iter);
	else
		iter->volume -= volume;
	assert(m_totalFluidVolume[index] >= volume);
	m_totalFluidVolume[index] -= volume;
	//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
	floating_maybeSink(index);
	fluid_maybeEraseFluidOnDeck(index);
}
void Blocks::fluid_fillInternal(const BlockIndex& index, const CollisionVolume& volume, FluidGroup& fluidGroup)
{
	bool wasEmpty = m_totalFluidVolume[index] == 0;
	FluidData* found = fluid_getData(index, fluidGroup.m_fluidType);
	if(!found)
		m_fluid[index].emplace_back(fluidGroup.m_fluidType, &fluidGroup, volume);
	else
	{
		found->volume += volume;
		//TODO: should this be enforced?
		//assert(*found->group == fluidGroup);
	}
	fluid_setTotalVolume(index, m_totalFluidVolume[index] + volume);
	//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
	floating_maybeFloatUp(index);
	if(wasEmpty)
		fluid_maybeRecordFluidOnDeck(index);
}
void Blocks::fluid_unsetGroupInternal(const BlockIndex& index, const FluidTypeId& fluidType)
{
	FluidData* found = fluid_getData(index, fluidType);
	found->group = nullptr;
}
bool Blocks::fluid_undisolveInternal(const BlockIndex& index, FluidGroup& fluidGroup)
{
	FluidData* found = fluid_getData(index, fluidGroup.m_fluidType);
	if(found || fluid_canEnterCurrently(index, fluidGroup.m_fluidType))
	{
		CollisionVolume capacity = fluid_volumeOfTypeCanEnter(index, fluidGroup.m_fluidType);
		assert(fluidGroup.m_excessVolume > 0);
		CollisionVolume flow = std::min(capacity, CollisionVolume::create(fluidGroup.m_excessVolume));
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
			m_fluid[index].emplace_back(fluidGroup.m_fluidType, &fluidGroup, flow);
			fluidGroup.addBlock(m_area, index, false);
			fluidGroup.m_disolved = false;
		}
		fluid_setTotalVolume(index, m_totalFluidVolume[index] + flow);
		//TODO: this could be run mulitple times per step where two fluid groups of different types are mixing, move to FluidGroup writeStep.
		m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
		return true;
	}
	return false;
}
void Blocks::fluid_remove(const BlockIndex& index, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	assert(volume <= fluid_volumeOfTypeContains(index, fluidType));
	fluid_getGroup(index, fluidType)->removeFluid(m_area, volume);
}
void Blocks::fluid_removeSyncronus(const BlockIndex& index, const CollisionVolume& volume, const FluidTypeId& fluidType)
{
	assert(volume <= fluid_volumeOfTypeContains(index, fluidType));
	assert(m_totalFluidVolume[index] >= volume);
	if(fluid_volumeOfTypeContains(index, fluidType) > volume)
		fluid_getData(index, fluidType)->volume -= volume;
	else
	{
		FluidGroup& group = *fluid_getGroup(index, fluidType);
		fluid_destroyData(index, fluidType);
		if(group.getBlocks().size() == 1)
			m_area.m_hasFluidGroups.removeFluidGroup(group);
		else
			group.removeBlock(m_area, index);
	}
	fluid_setTotalVolume(index, m_totalFluidVolume[index] - volume);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
	floating_maybeSink(index);
	fluid_maybeEraseFluidOnDeck(index);
}
void Blocks::fluid_removeAllSyncronus(const BlockIndex& index)
{
	auto copy = m_fluid[index];
	for(FluidData& data : copy)
		fluid_removeSyncronus(index, data.volume, data.type);
	assert(m_totalFluidVolume[index] == 0);
}
bool Blocks::fluid_canEnterCurrently(const BlockIndex& index, const FluidTypeId& fluidType) const
{
	if(m_totalFluidVolume[index] < Config::maxBlockVolume)
		return true;
	for(const FluidData& fluidData : m_fluid[index])
		if(FluidType::getDensity(fluidData.type) < FluidType::getDensity(fluidType))
			return true;
	return false;
}
bool Blocks::fluid_canEnterEver(const BlockIndex& index) const
{
	return !solid_is(index);
}
bool Blocks::fluid_isAdjacentToGroup(const BlockIndex& index, const FluidGroup& fluidGroup) const
{
	for(const BlockIndex& adjacent : getDirectlyAdjacent(index))
		if(fluid_contains(adjacent, fluidGroup.m_fluidType) && fluid_getGroup(adjacent, fluidGroup.m_fluidType) == &fluidGroup)
			return true;
	return false;
}
CollisionVolume Blocks::fluid_volumeOfTypeCanEnter(const BlockIndex& index, const FluidTypeId& fluidType) const
{
	CollisionVolume output = Config::maxBlockVolume;
	for(const FluidData& fluidData : m_fluid[index])
		if(FluidType::getDensity(fluidData.type) >= FluidType::getDensity(fluidType))
			output -= fluidData.volume;
	return output;
}
CollisionVolume Blocks::fluid_volumeOfTypeContains(const BlockIndex& index, const FluidTypeId& fluidType) const
{
	const auto found = fluid_getData(index, fluidType);
	if(!found)
		return CollisionVolume::create(0);
	return found->volume;
}
FluidTypeId Blocks::fluid_getTypeWithMostVolume(const BlockIndex& index) const
{
	assert(!m_fluid[index].empty());
	CollisionVolume volume = CollisionVolume::create(0);
	FluidTypeId output;
	for(const FluidData& fluidData: m_fluid[index])
		if(volume < fluidData.volume)
			output = fluidData.type;
	assert(output.exists());
	return output;
}
FluidTypeId Blocks::fluid_getMist(const BlockIndex& index) const
{
	return m_mist[index];
}
void Blocks::fluid_resolveOverfull(const BlockIndex& index)
{
	std::vector<FluidTypeId> toErase;
	// Fluid types are sorted by density.
	for(FluidData& fluidData : fluid_getAllSortedByDensityAscending(index))
	{

		assert(fluidData.type == fluidData.group->m_fluidType);
		// Displace lower density fluids.
		CollisionVolume displaced = std::min(fluidData.volume, m_totalFluidVolume[index] - Config::maxBlockVolume);
		assert(displaced.exists());
		fluidData.volume -= displaced;
		fluid_setTotalVolume(index, m_totalFluidVolume[index] - displaced);
		fluidData.group->addFluid(m_area, displaced);
		if(fluidData.volume == 0)
			toErase.push_back(fluidData.type);
		if(fluidData.volume < Config::maxBlockVolume)
			fluidData.group->m_fillQueue.maybeAddBlock(index);
		if(m_totalFluidVolume[index] == Config::maxBlockVolume)
			break;
	}
	for(FluidTypeId fluidType : toErase)
	{
		// If the last block of a fluidGroup is displaced disolve it in the lowest density liquid which is more dense then it.
		FluidGroup* fluidGroup = fluid_getGroup(index, fluidType);
		assert(fluidGroup->m_fluidType == fluidType);
		fluidGroup->removeBlock(m_area, index);
		if(fluidGroup->m_drainQueue.m_set.empty())
		{
			for(FluidData& otherFluidData : m_fluid[index])
				if(FluidType::getDensity(otherFluidData.type) > FluidType::getDensity(fluidType))
				{
					//TODO: find.
					if(otherFluidData.group->m_disolvedInThisGroup.contains(fluidType))
					{
						otherFluidData.group->m_disolvedInThisGroup[fluidType]->m_excessVolume += fluidGroup->m_excessVolume;
						fluidGroup->m_destroy = true;
					}
					else
					{
						otherFluidData.group->m_disolvedInThisGroup.insert(fluidType, fluidGroup);
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
void Blocks::fluid_onBlockSetSolid(const BlockIndex& index)
{
	// Displace fluids.
	for(FluidData& fluidData : m_fluid[index])
	{
		fluidData.group->removeBlock(m_area, index);
		fluidData.group->addFluid(m_area, fluidData.volume);
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
						fluidData.group->m_fillQueue.maybeAddBlock(above);
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
		fluidData.group = nullptr;
	}
	m_fluid[index].clear();
	fluid_setTotalVolume(index, CollisionVolume::create(0));
	// Remove from fluid fill queues of adjacent groups, if contained.
	for(const BlockIndex& adjacent : getDirectlyAdjacent(index))
		if(fluid_canEnterEver(adjacent))
			for(FluidData& fluidData : m_fluid[adjacent])
				fluidData.group->m_fillQueue.maybeRemoveBlock(index);
}
void Blocks::fluid_onBlockSetNotSolid(const BlockIndex& index)
{
	for(const BlockIndex& adjacent : getDirectlyAdjacent(index))
		if(fluid_canEnterEver(adjacent))
			for(FluidData& fluidData : m_fluid[adjacent])
			{
				fluidData.group->m_fillQueue.maybeAddBlock(index);
				fluidData.group->m_stable = false;
			}
}
bool Blocks::fluid_contains(const BlockIndex& index, const FluidTypeId& fluidType) const
{
	auto fluid = m_fluid[index];
	return std::ranges::find(fluid, fluidType, &FluidData::type) != fluid.end();
}
CollisionVolume Blocks::fluid_getTotalVolume(const BlockIndex& index) const
{
	CollisionVolume total = CollisionVolume::create(0);
	for(FluidData data : m_fluid[index])
		total += data.volume;
	assert(total == m_totalFluidVolume[index]);
	return m_totalFluidVolume[index];
}
const std::vector<FluidData>& Blocks::fluid_getAll(const BlockIndex& index) const
{
	return m_fluid[index];
}
std::vector<FluidData>& Blocks::fluid_getAllSortedByDensityAscending(const BlockIndex& index)
{
	std::ranges::sort(m_fluid[index], std::less<Density>(), [](const auto& data) { return FluidType::getDensity(data.type); });
	return m_fluid[index];
}
bool Blocks::fluid_any(const BlockIndex& index) const
{
	return !m_fluid[index].empty();
}
void Blocks::fluid_maybeRecordFluidOnDeck(const BlockIndex& index)
{
	assert(m_totalFluidVolume[index] != 0);
	const DeckId& deckId = m_area.m_decks.getForBlock(index);
	if(deckId.exists())
	{
		const ActorOrItemIndex& isOnDeckOf = m_area.m_decks.getForId(deckId);
		assert(isOnDeckOf.isItem());
		const ItemIndex& item = isOnDeckOf.getItem();
		m_area.getItems().onDeck_recordBlockContainingFluid(item, index);
	}
}
void Blocks::fluid_maybeEraseFluidOnDeck(const BlockIndex& index)
{
	if(m_totalFluidVolume[index] == 0)
	{
		const DeckId& deckId = m_area.m_decks.getForBlock(index);
		if(deckId.exists())
		{
			const ActorOrItemIndex& isOnDeckOf = m_area.m_decks.getForId(deckId);
			assert(isOnDeckOf.isItem());
			const ItemIndex& item = isOnDeckOf.getItem();
			m_area.getItems().onDeck_eraseBlockContainingFluid(item, index);
		}
	}
}