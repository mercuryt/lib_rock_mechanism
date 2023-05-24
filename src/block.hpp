/*
 * A block. Contains either a single type of material in 'solid' form or arbitrary objects with volume, generic solids and liquids.
 * Total volume is 100.
 */
#pragma once

#include <unordered_map>
#include <unordered_set>
#include <array>
#include <math.h>
#include <algorithm>
#include <cassert>

#include "moveType.h"
#include "shape.h"
#include "hasShape.h"
#include "block.h"

template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::BaseBlock() : m_solid(nullptr), m_routeCacheVersion(0), m_totalDynamicVolume(0), m_totalStaticVolume(0), m_totalFluidVolume(0), m_mist(nullptr), m_mistSource(nullptr),  m_mistInverseDistanceFromSource(0), m_visionCuboid(nullptr), m_deltaTemperature(0), m_exposedToSky(true) {}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::setup(Area* a, uint32_t ax, uint32_t ay, uint32_t az)
{m_area=a;m_x=ax;m_y=ay;m_z=az;m_locationBucket = a->m_locationBuckets.getBucketFor(const_derived());}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::recordAdjacent()
{
	static const int32_t offsetsList[6][3] = {{0,0,-1}, {0,-1,0}, {-1,0,0}, {0,1,0}, {1,0,0}, {0,0,1}};
	for(uint32_t i = 0; i < 6; i++)
	{
		auto& offsets = offsetsList[i];
		DerivedBlock* block = m_adjacents[i] = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			m_adjacentsVector.push_back(block);
	}
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
std::vector<DerivedBlock*> BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getAdjacentWithEdgeAdjacent() const
{
	std::vector<DerivedBlock*> output;
	output.reserve(18);
	static const int32_t offsetsList[18][3] = {
		{-1,0,-1}, 
		{0,1,-1}, {0,0,-1}, {0,-1,-1},
		{1,0,-1}, 

		{-1,-1,0}, {-1,0,0}, {0,-1,0},
		{1,1,0}, {0,1,0},
		{1,-1,0}, {1,0,0}, {0,-1,0},

		{-1,0,1}, 
		{0,1,1}, {0,0,1}, {0,-1,1},
		{1,0,1}, 
	};
	for(uint32_t i = 0; i < 26; i++)
	{
		auto& offsets = offsetsList[i];
		DerivedBlock* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
std::vector<DerivedBlock*> BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getAdjacentWithEdgeAndCornerAdjacent() const
{
	std::vector<DerivedBlock*> output;
	output.reserve(26);
	static const int32_t offsetsList[26][3] = {
		{-1,1,-1}, {-1,0,-1}, {-1,-1,-1},
		{0,1,-1}, {0,0,-1}, {0,-1,-1},
		{1,1,-1}, {1,0,-1}, {1,-1,-1},

		{-1,-1,0}, {-1,0,0}, {0,-1,0},
		{1,1,0}, {0,1,0},
		{1,-1,0}, {1,0,0}, {-1,1,0},

		{-1,1,1}, {-1,0,1}, {-1,-1,1},
		{0,1,1}, {0,0,1}, {0,-1,1},
		{1,1,1}, {1,0,1}, {1,-1,1}
	};
	for(uint32_t i = 0; i < 26; i++)
	{
		auto& offsets = offsetsList[i];
		DerivedBlock* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
std::vector<DerivedBlock*> BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getEdgeAdjacentOnly() const
{
	std::vector<DerivedBlock*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,-1}, {0,-1,-1},
		{1,0,-1}, {0,1,-1}, 

		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},

		{-1,0,1}, {0,-1,1},
		{0,1,1}, {1,0,1}, 
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		DerivedBlock* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
std::vector<DerivedBlock*> BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getEdgeAdjacentOnSameZLevelOnly() const
{
	std::vector<DerivedBlock*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		DerivedBlock* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
std::vector<DerivedBlock*> BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getEdgeAdjacentOnlyOnNextZLevelDown() const
{
	std::vector<DerivedBlock*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,-1}, {0,-1,-1},
		{1,0,-1}, {0,1,-1}, 
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		DerivedBlock* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
std::vector<DerivedBlock*> BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getEdgeAdjacentOnlyOnNextZLevelUp() const
{
	std::vector<DerivedBlock*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,1}, {0,-1,1},
		{0,1,1}, {1,0,1}, 
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		DerivedBlock* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
std::vector<DerivedBlock*> BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getEdgeAndCornerAdjacentOnly() const
{
	std::vector<DerivedBlock*> output;
	output.reserve(20);
	static const int32_t offsetsList[20][3] = {
		{-1,-1,-1}, {-1,0,-1}, {0,-1,-1},
		{1,1,-1}, {0,1,-1},
		{1,-1,-1}, {1,0,-1}, {0,1,-1}, 

		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},

		{-1,-1,1}, {-1,0,1}, {0,-1,1},
		{1,1,1}, {0,1,1},
		{1,-1,1}, {1,0,1}, {0,-1,1}
	};
	for(uint32_t i = 0; i < 20; i++)
	{
		auto& offsets = offsetsList[i];
		DerivedBlock* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
std::vector<DerivedBlock*> BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getAdjacentOnSameZLevelOnly() const
{
	std::vector<DerivedBlock*> output;
	output.reserve(4);
	static const int32_t offsetsList[4][3] = {
		{-1,0,0}, {1,0,0}, 
		{0,-1,0}, {0,1,0}
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		DerivedBlock* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
uint32_t BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::distance(DerivedBlock& block) const
{
	uint32_t dx = abs((int)m_x - (int)block.m_x);
	uint32_t dy = abs((int)m_y - (int)block.m_y);
	uint32_t dz = abs((int)m_z - (int)block.m_z);
	return pow((pow(dx, 2) + pow(dy, 2) + pow(dz, 2)), 0.5);
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
uint32_t BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::taxiDistance(DerivedBlock& block) const
{
	return abs((int)m_x - (int)block.m_x) + abs((int)m_y - (int)block.m_y) + abs((int)m_z - (int)block.m_z);
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
bool BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::isAdjacentToAny(std::unordered_set<DerivedBlock*>& blocks) const
{
	for(DerivedBlock* adjacent : m_adjacentsVector)
		if(blocks.contains(adjacent))
			return true;
	return false;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::setNotSolid()
{
	if(m_solid == nullptr)
		return;
	m_solid = nullptr;
	for(DerivedBlock* adjacent : m_adjacentsVector)
		if(adjacent->fluidCanEnterEver())
			for(auto& [fluidType, pair] : adjacent->m_fluids)
			{
				pair.second->m_fillQueue.addBlock(&derived());
				pair.second->m_stable = false;
			}
	DerivedBlock& block = derived();
	block.clearMoveCostsCacheForSelfAndAdjacent();
	if(m_area->m_visionCuboidsActive)
		VisionCuboid<DerivedBlock, Area>::BlockIsNeverOpaque(block);
	if(m_adjacents[5] == nullptr || m_adjacents[5]->m_exposedToSky)
	{
		block.setExposedToSky(true);
		block.setBelowExposedToSky();
	}
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::setBelowExposedToSky()
{
	// Set blocks below as exposed to sky.
	DerivedBlock* block = m_adjacents[0];
	while(block != nullptr && block->canSeeThroughFrom(*block->m_adjacents[5]) && !block->m_exposedToSky)
	{
		block->setExposedToSky(true);
		block = block->m_adjacents[0];
	}
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::setSolid(const MaterialType& materialType)
{
	if(&materialType == m_solid)
		return;
	m_solid = &materialType;
	// Displace fluids.
	m_totalFluidVolume = 0;
	for(auto& [fluidType, pair] : m_fluids)
	{
		pair.second->removeBlock(derived());
		pair.second->addFluid(pair.first);
		// If there is no where to put the fluid.
		if(pair.second->m_drainQueue.m_set.empty() && pair.second->m_fillQueue.m_set.empty())
		{
			// If fluid piston is enabled then find a place above to add to potential.
			if constexpr (Config::fluidPiston)
			{
				DerivedBlock* above = m_adjacents[5];
				while(above != nullptr)
				{
					if(above->fluidCanEnterEver() && above->fluidCanEnterEver(*fluidType) &&
							above->fluidCanEnterCurrently(*fluidType)
					  )
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
				m_area->m_fluidGroups.remove(*pair.second);
				m_area->m_unstableFluidGroups.erase(pair.second);
			}
		}
	}
	m_fluids.clear();
	// Remove from fluid fill queues.
	for(DerivedBlock* adjacent : m_adjacentsVector)
		if(adjacent->fluidCanEnterEver())
			for(auto& [fluidType, pair] : adjacent->m_fluids)
				pair.second->m_fillQueue.removeBlock(&derived());
	// Possible expire path caches.
	// If more then one adjacent block can be entered then this block being cleared may open a new shortest path.
	if(std::ranges::count_if(m_adjacentsVector, [&](DerivedBlock* block){ return block->anyoneCanEnterEver(); }) > 1)
		m_area->expireRouteCache();
	DerivedBlock& block = derived();
	block.clearMoveCostsCacheForSelfAndAdjacent();
	if(m_area->m_visionCuboidsActive && !materialType.transparent)
		VisionCuboid<DerivedBlock, Area>::BlockIsSometimesOpaque(block);
	// Set blocks below as not exposed to sky.
	block.setExposedToSky(false);
	block.setBelowNotExposedToSky();
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::setBelowNotExposedToSky()
{
	DerivedBlock* block = m_adjacents[0];
	while(block != nullptr && block->m_exposedToSky)
	{
		block->setExposedToSky(false);
		block = block->m_adjacents[0];
	}
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
bool BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::canEnterEver(Actor& actor) const
{
	return shapeAndMoveTypeCanEnterEver(*actor.m_shape, *actor.m_moveType);
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
bool BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::isSolid() const
{
	return m_solid != nullptr;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
const MaterialType& BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getSolidMaterial() const
{
	return *m_solid;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::spawnMist(const FluidType& fluidType, uint32_t maxMistSpread)
{
	if(m_mist != nullptr and m_mist->density > fluidType.density)
		return;
	m_mist = &fluidType;
	m_mistInverseDistanceFromSource = maxMistSpread != 0 ? maxMistSpread : fluidType.maxMistSpread;
	MistDisperseEvent<DerivedBlock, FluidType>::emplace(m_area->m_eventSchedule, fluidType.mistDuration, fluidType, derived());
}
//TODO: This code puts the fluid into an adjacent group of the correct type if it can find one, it does not add the block or merge groups, leaving these tasks to fluidGroup readStep. Is this ok?
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::addFluid(uint32_t volume, const FluidType& fluidType)
{
	// If a suitable fluid group exists already then just add to it's excessVolume.
	if(m_fluids.contains(&fluidType))
	{
		assert(m_fluids.at(&fluidType).second->m_fluidType == &fluidType);
		m_fluids.at(&fluidType).second->addFluid(volume);
		return;
	}
	m_fluids.emplace(&fluidType, std::make_pair(volume, nullptr));
	m_totalFluidVolume += volume;
	// Find fluid group.
	FluidGroup<DerivedBlock, Area, FluidType>* fluidGroup = nullptr;
	for(DerivedBlock* adjacent : m_adjacentsVector)
		if(adjacent->fluidCanEnterEver() && adjacent->m_fluids.contains(&fluidType))
		{
			assert(adjacent->m_fluids.at(&fluidType).second->m_fluidType == &fluidType);
			fluidGroup = adjacent->m_fluids.at(&fluidType).second;
			fluidGroup->addBlock(derived());
			continue;
		}
	// Create fluid group.
	if(fluidGroup == nullptr)
	{
		std::unordered_set<DerivedBlock*> blocks({&derived()});
		fluidGroup = m_area->createFluidGroup(fluidType, blocks);
	}
	// Shift less dense fluids to excessVolume.
	if(m_totalFluidVolume > Config::maxBlockVolume)
		resolveFluidOverfull();
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::removeFluid(uint32_t volume, const FluidType& fluidType)
{
	m_fluids.at(&fluidType).second->removeFluid(volume);
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
// Validate the actor can enter this block and also any other blocks required by it's Shape.
bool BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::shapeAndMoveTypeCanEnterEver(const Shape& shape, const MoveType& moveType) const
{
	if(shape.positions.size() == 1)
	{
		const DerivedBlock& block = const_derived();
		return block.anyoneCanEnterEver() && block.moveTypeCanEnter(moveType);
	}
	for(auto& [m_x, m_y, m_z, v] : shape.positions)
	{
		const DerivedBlock* block = offset(m_x, m_y, m_z);
		if(block == nullptr || !block->anyoneCanEnterEver() || !block->moveTypeCanEnter(moveType))
			return false;
	}
	return true;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
bool BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::actorCanEnterCurrently(Actor& actor) const
{
	const Shape& shape = *actor.m_shape;
	if(shape.positions.size() == 1)
	{
		const DerivedBlock& block = const_derived();
		uint32_t v = shape.positions[0][3];
		if(&block == actor.m_location)
			v = 0;
		return block.m_totalDynamicVolume + v <= Config::maxBlockVolume;
	}
	for(auto& [x, y, z, v] : shape.positions)
	{
		const DerivedBlock* block = offset(x, y, z);
		auto found = block->m_actors.find(&actor);
		if(found != block->m_actors.end())
		{
			if(block->m_totalDynamicVolume + v - found->second > Config::maxBlockVolume)
				return false;
		}
		else if(block->m_totalDynamicVolume + v > Config::maxBlockVolume)
			return false;
	}
	return true;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
DerivedBlock* BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::offset(int32_t ax, int32_t ay, int32_t az) const
{
	ax += m_x;
	ay += m_y;
	az += m_z;
	if(ax < 0 || (uint32_t)ax >= m_area->m_sizeX || ay < 0 || (uint32_t)ay >= m_area->m_sizeY || az < 0 || (uint32_t)az >= m_area->m_sizeZ)
		return nullptr;
	return &m_area->m_blocks[ax][ay][az];
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
bool BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::fluidCanEnterCurrently(const FluidType& fluidType) const
{
	if(m_totalFluidVolume < Config::maxBlockVolume)
		return true;
	for(auto& pair : m_fluids)
		if(pair.first->density < fluidType.density)
			return true;
	return false;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
bool BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::isAdjacentToFluidGroup(const FluidGroup<DerivedBlock, Area, FluidType>* fluidGroup) const
{
	for(DerivedBlock* block : m_adjacentsVector)
		if(block->m_fluids.contains(fluidGroup->m_fluidType) && block->m_fluids.at(fluidGroup->m_fluidType).second == fluidGroup)
			return true;
	return false;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
uint32_t BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::volumeOfFluidTypeCanEnter(const FluidType& fluidType) const
{
	uint32_t output = Config::maxBlockVolume;
	for(auto& pair : m_fluids)
		if(pair.first->density >= fluidType.density)
			output -= pair.second.first;
	return output;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
uint32_t BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::volumeOfFluidTypeContains(const FluidType& fluidType) const
{
	auto found = m_fluids.find(&fluidType);
	if(found == m_fluids.end())
		return 0;
	return found->second.first;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
const FluidType& BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getFluidTypeWithMostVolume() const
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
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
FluidGroup<DerivedBlock, Area, FluidType>* BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::getFluidGroup(const FluidType& fluidType) const
{
	auto found = m_fluids.find(&fluidType);
	if(found == m_fluids.end())
		return nullptr;
	assert(found->second.second->m_fluidType == fluidType);
	return found->second.second;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::resolveFluidOverfull()
{
	std::vector<const FluidType*> toErase;
	// Fluid types are sorted by density.
	for(auto& [fluidType, pair] : m_fluids)
	{
		assert(fluidType == pair.second->m_fluidType);
		// Displace lower density fluids.
		uint32_t displaced = std::min(pair.first, m_totalFluidVolume - Config::maxBlockVolume);
		m_totalFluidVolume -= displaced;
		pair.first -= displaced;
		pair.second->addFluid(displaced);
		if(pair.first == 0)
			toErase.push_back(fluidType);
		if(pair.first < Config::maxBlockVolume)
			pair.second->m_fillQueue.addBlock(&derived());
		if(m_totalFluidVolume == Config::maxBlockVolume)
			break;
	}
	for(const FluidType* fluidType : toErase)
	{
		// If the last block of a fluidGroup is displaced disolve it in the lowest density liquid which is more dense then it.
		FluidGroup<DerivedBlock, Area, FluidType>* fluidGroup = m_fluids.at(fluidType).second;
		assert(fluidGroup->m_fluidType == *fluidType);
		fluidGroup->removeBlock(derived());
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
// Add / remove  actor occupancy.
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::enter(Actor& actor)
{
	assert(actor.m_location != this);
	actor.m_taskDelayCount = 0;
	bool hasPreviousLocation = actor.m_location != nullptr;
	actor.m_location = &derived();
	if(hasPreviousLocation)
	{
		actor.m_location->exit(actor);
		actor.m_blocks.clear();
		m_area->m_locationBuckets.update(actor, *actor.m_location, derived());
	}
	else
		m_area->m_locationBuckets.insert(actor);
	for(auto& [x, y, z, v] : actor.m_shape->positions)
	{
		DerivedBlock* block = offset(x, y, z);
		assert(!block->m_actors.contains(&actor));
		block->m_actors[&actor] = v;
		block->m_totalDynamicVolume += v;
		actor.m_blocks.push_back(block);
	}
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::exit(Actor& actor)
{
	for(DerivedBlock* block : actor.m_blocks)
	{
		auto found = block->m_actors.find(&actor);
		assert(found != block->m_actors.end());
		block->m_totalDynamicVolume -= found->second;
		block->m_actors.erase(found);
	}
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::applyTemperatureDelta(int32_t delta)
{
	assert((int)m_deltaTemperature + (int)delta > INT32_MIN);
	assert((int)m_deltaTemperature + (int)delta < INT32_MAX);
	if(!m_area->m_blocksWithChangedTemperature.contains(&derived()))
		m_area->m_blocksWithChangedTemperature[&derived()] = m_deltaTemperature;
	// Use insert so only record once if changed multiple times in a step.
	//m_area->m_blocksWithChangedTemperature.insert({this, m_deltaTemperature});
	m_deltaTemperature += delta;
}
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
bool BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::operator==(const DerivedBlock& block) const { return &block == this; };
//TODO: Replace with cuboid.
template<class DerivedBlock, class Actor, class Area, class FluidType, class MoveType, class MaterialType>
std::vector<DerivedBlock*> BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::selectBetweenCorners(DerivedBlock* otherBlock) const
{
	assert(otherBlock->m_x < m_area->m_sizeX);
	assert(otherBlock->m_y < m_area->m_sizeY);
	assert(otherBlock->m_z < m_area->m_sizeZ);
	std::vector<DerivedBlock*> output;
	uint32_t minX = std::min(m_x, otherBlock->m_x);
	uint32_t maxX = std::max(m_x, otherBlock->m_x);
	uint32_t minY = std::min(m_y, otherBlock->m_y);
	uint32_t maxY = std::max(m_y, otherBlock->m_y);
	uint32_t minZ = std::min(m_z, otherBlock->m_z);
	uint32_t maxZ = std::max(m_z, otherBlock->m_z);
	for(uint32_t x = minX; x <= maxX; ++x)
		for(uint32_t y = minY; y <= maxY; ++y)
			for(uint32_t z = minZ; z <= maxZ; ++z)
			{
				output.push_back(&m_area->m_blocks[x][y][z]);
				assert(output.back() != nullptr);
			}
	return output;
}
/*
// Add / remove nongeneric non-actor occupancy for this block and also any other blocks required by it's Shape.
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::place(HasShape* hasShape)
{
for(auto& [x, y, z, v] : hasShape->m_shape.positions)
{
DerivedBlock* block = offset(x, y, z);
block->recordHasShapeAndVolume(hasShape, v);
}
hasShape->m_location = this;
}
void BaseBlock<DerivedBlock, Actor, Area, FluidType, MoveType, MaterialType>::unplace(HasShape* hasShape)
{
for(auto& [x, y, z, v] : hasShape->m_shape.positions)
{
DerivedBlock* block = offset(x, y, z);
block->removeHasShapeAndVolume(hasShape, v);
}
}
 *
 */
