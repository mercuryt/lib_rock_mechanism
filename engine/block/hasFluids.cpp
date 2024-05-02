#include "hasFluids.h"
#include "../fluidType.h"
#include "../mistDisperseEvent.h"
#include "../fluidGroup.h"
#include "../block.h"
#include "../area.h"
#include "../deserializationMemo.h"

bool SortByDensity::operator()(const FluidType* a, const FluidType* b) const { return a->density < b->density; }
void BlockHasFluids::load(const Json& data, [[maybe_unused]] const DeserializationMemo& deserializationMemo)
{
		for(const Json& pair : data)
			addFluid(pair[1].get<Volume>(), *pair[0].get<const FluidType*>());
}
Json BlockHasFluids::toJson() const
{
	Json data = Json::array();
	for(auto& [fluidType, pair] : m_fluids)
		data.push_back({fluidType, pair.first});
	return data;
}
void BlockHasFluids::spawnMist(const FluidType& fluidType, DistanceInBlocks maxMistSpread)
{
	if(m_mist != nullptr && (m_mist == &fluidType || m_mist->density > fluidType.density))
		return;
	m_mist = &fluidType;
	m_mistInverseDistanceFromSource = maxMistSpread != 0 ? maxMistSpread : fluidType.maxMistSpread;
	MistDisperseEvent::emplace(fluidType.mistDuration, fluidType, m_block);
}
FluidGroup* BlockHasFluids::getFluidGroup(const FluidType& fluidType) const
{
	auto found = m_fluids.find(&fluidType);
	if(found == m_fluids.end())
		return nullptr;
	assert(found->second.second->m_fluidType == fluidType);
	return found->second.second;
}
void BlockHasFluids::addFluid(uint32_t volume, const FluidType& fluidType)
{
	assert(!m_block.isSolid());
	// If a suitable fluid group exists already then just add to it's excessVolume.
	if(m_fluids.contains(&fluidType))
	{
		assert(m_fluids.at(&fluidType).second->m_fluidType == fluidType);
		m_fluids.at(&fluidType).second->addFluid(volume);
		return;
	}
	m_fluids.emplace(&fluidType, std::make_pair(volume, nullptr));
	m_totalFluidVolume += volume;
	// Find fluid group.
	FluidGroup* fluidGroup = nullptr;
	for(Block* adjacent : m_block.m_adjacents)
		if(adjacent && adjacent->m_hasFluids.fluidCanEnterEver() && adjacent->m_hasFluids.m_fluids.contains(&fluidType))
		{
			assert(adjacent->m_hasFluids.m_fluids.at(&fluidType).second->m_fluidType == fluidType);
			fluidGroup = adjacent->m_hasFluids.m_fluids.at(&fluidType).second;
			fluidGroup->addBlock(m_block, true);
			break;
		}
	// Create fluid group.
	if(fluidGroup == nullptr)
	{
		std::unordered_set<Block*> blocks({&m_block});
		fluidGroup = m_block.m_area->createFluidGroup(fluidType, blocks);
	}
	else
		m_block.m_area->clearMergedFluidGroups();
	// Shift less dense fluids to excessVolume.
	if(m_totalFluidVolume > Config::maxBlockVolume)
		resolveFluidOverfull();
}
void BlockHasFluids::removeFluid(uint32_t volume, const FluidType& fluidType)
{
	assert(volume <= volumeOfFluidTypeContains(fluidType));
	m_fluids.at(&fluidType).second->removeFluid(volume);
}
void BlockHasFluids::removeFluidSyncronus(uint32_t volume, const FluidType& fluidType)
{
	assert(volume <= volumeOfFluidTypeContains(fluidType));
	m_totalFluidVolume -= volume;
	if(volumeOfFluidTypeContains(fluidType) > volume)
		m_fluids.at(&fluidType).first -= volume;
	else
	{
		FluidGroup& group = *m_fluids.at(&fluidType).second;
		m_fluids.erase(&fluidType);
		if(group.getBlocks().size() == 1)
			m_block.m_area->removeFluidGroup(group);
		else
			group.removeBlock(m_block);
	}
}
bool BlockHasFluids::fluidCanEnterCurrently(const FluidType& fluidType) const
{
	if(m_totalFluidVolume < Config::maxBlockVolume)
		return true;
	for(auto& pair : m_fluids)
		if(pair.first->density < fluidType.density)
			return true;
	return false;
}
bool BlockHasFluids::fluidCanEnterEver() const
{
	return !m_block.isSolid();
}
bool BlockHasFluids::isAdjacentToFluidGroup(const FluidGroup* fluidGroup) const
{
	for(Block* block : m_block.m_adjacents)
		if(block && block->m_hasFluids.m_fluids.contains(&fluidGroup->m_fluidType) && block->m_hasFluids.m_fluids.at(&fluidGroup->m_fluidType).second == fluidGroup)
			return true;
	return false;
}
uint32_t BlockHasFluids::volumeOfFluidTypeCanEnter(const FluidType& fluidType) const
{
	uint32_t output = Config::maxBlockVolume;
	for(auto& pair : m_fluids)
		if(pair.first->density >= fluidType.density)
			output -= pair.second.first;
	return output;
}
uint32_t BlockHasFluids::volumeOfFluidTypeContains(const FluidType& fluidType) const
{
	auto found = m_fluids.find(&fluidType);
	if(found == m_fluids.end())
		return 0;
	return found->second.first;
}
const FluidType& BlockHasFluids::getFluidTypeWithMostVolume() const
{
	assert(!m_fluids.empty());
	uint32_t volume = 0;
	FluidType* output = nullptr;
	for(auto [fluidType, pair] : m_fluids)
		if(volume < pair.first)
			output = const_cast<FluidType*>(fluidType);
	assert(output != nullptr);
	return *output;
}
void BlockHasFluids::resolveFluidOverfull()
{
	std::vector<const FluidType*> toErase;
	// Fluid types are sorted by density.
	for(auto& [fluidType, pair] : m_fluids)
	{
		assert(fluidType == &pair.second->m_fluidType);
		// Displace lower density fluids.
		uint32_t displaced = std::min(pair.first, m_totalFluidVolume - Config::maxBlockVolume);
		m_totalFluidVolume -= displaced;
		pair.first -= displaced;
		pair.second->addFluid(displaced);
		if(pair.first == 0)
			toErase.push_back(fluidType);
		if(pair.first < Config::maxBlockVolume)
			pair.second->m_fillQueue.addBlock(&m_block);
		if(m_totalFluidVolume == Config::maxBlockVolume)
			break;
	}
	for(const FluidType* fluidType : toErase)
	{
		// If the last block of a fluidGroup is displaced disolve it in the lowest density liquid which is more dense then it.
		FluidGroup* fluidGroup = m_fluids.at(fluidType).second;
		assert(fluidGroup->m_fluidType == *fluidType);
		fluidGroup->removeBlock(m_block);
		if(fluidGroup->m_drainQueue.m_set.empty())
		{
			for(auto& [otherFluidType, pair] : m_fluids)
				if(otherFluidType->density > fluidType->density)
				{
					//TODO: find.
					if(pair.second->m_disolvedInThisGroup.contains(fluidType))
					{
						pair.second->m_disolvedInThisGroup.at(fluidType)->m_excessVolume += fluidGroup->m_excessVolume;
						fluidGroup->m_destroy = true;
					}
					else
					{
						pair.second->m_disolvedInThisGroup[fluidType] = fluidGroup;
						fluidGroup->m_disolved = true;
					}
					break;
				}
			assert(fluidGroup->m_disolved || fluidGroup->m_destroy);
		}
		m_fluids.erase(fluidType);
	}
}
void BlockHasFluids::onBlockSetSolid()
{
	// Displace fluids.
	m_totalFluidVolume = 0;
	for(auto& [fluidType, pair] : m_fluids)
	{
		pair.second->removeBlock(m_block);
		pair.second->addFluid(pair.first);
		// If there is no where to put the fluid.
		if(pair.second->m_drainQueue.m_set.empty() && pair.second->m_fillQueue.m_set.empty())
		{
			// If fluid piston is enabled then find a place above to add to potential.
			if constexpr (Config::fluidPiston)
			{
				Block* above = m_block.getBlockAbove();
				while(above != nullptr)
				{
					if(above->m_hasFluids.fluidCanEnterEver() && above->m_hasFluids.fluidCanEnterCurrently(*fluidType))
					{
						pair.second->m_fillQueue.addBlock(above);
						break;
					}
					above = above->m_adjacents[5];
				}
				// The only way that this 'piston' code should be triggered is if something falls, which means the top layer cannot be full.
				assert(above != nullptr);
			}
			else
			{
				// Otherwise destroy the group.
				m_block.m_area->m_fluidGroups.remove(*pair.second);
				m_block.m_area->m_unstableFluidGroups.erase(pair.second);
			}
		}
	}
	m_fluids.clear();
	// Remove from fluid fill queues.
	for(Block* adjacent : m_block.m_adjacents)
		if(adjacent && adjacent->m_hasFluids.fluidCanEnterEver())
			for(auto& [fluidType, pair] : adjacent->m_hasFluids.m_fluids)
				pair.second->m_fillQueue.removeBlock(&m_block);
}
void BlockHasFluids::onBlockSetNotSolid()
{
	for(Block* adjacent : m_block.m_adjacents)
		if(adjacent && adjacent->m_hasFluids.fluidCanEnterEver())
			for(auto& [fluidType, pair] : adjacent->m_hasFluids.m_fluids)
			{
				pair.second->m_fillQueue.addBlock(&m_block);
				pair.second->m_stable = false;
			}
}
