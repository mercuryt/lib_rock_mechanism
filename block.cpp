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

#include "materialType.h"
#include "moveType.h"
#include "shape.h"
#include "hasShape.h"
#include "fluidType.h"

baseBlock::baseBlock() : m_solid(nullptr), m_routeCacheVersion(0), m_mist(nullptr), m_mistSource(nullptr),  m_mistInverseDistanceFromSource(0) {}
void baseBlock::setup(Area* a, uint32_t ax, uint32_t ay, uint32_t az)
{m_area=a;m_x=ax;m_y=ay;m_z=az;m_locationBucket = a->m_locationBuckets.getBucketFor(static_cast<Block*>(this));}
void baseBlock::recordAdjacent()
{
	static const int32_t offsetsList[6][3] = {{0,0,-1}, {0,-1,0}, {-1,0,0}, {0,1,0}, {1,0,0}, {0,0,1}};
	for(uint32_t i = 0; i < 6; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = m_adjacents[i] = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			m_adjacentsVector.push_back(block);
	}
}
std::vector<Block*> baseBlock::getAdjacentWithEdgeAdjacent() const
{
	std::vector<Block*> output;
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
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> baseBlock::getAdjacentWithEdgeAndCornerAdjacent() const
{
	std::vector<Block*> output;
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
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> baseBlock::getEdgeAdjacentOnly() const
{
	std::vector<Block*> output;
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
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> baseBlock::getEdgeAdjacentOnSameZLevelOnly() const
{
	std::vector<Block*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> baseBlock::getEdgeAdjacentOnlyOnNextZLevelDown() const
{
	std::vector<Block*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,-1}, {0,-1,-1},
		{1,0,-1}, {0,1,-1}, 
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> baseBlock::getEdgeAdjacentOnlyOnNextZLevelUp() const
{
	std::vector<Block*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,1}, {0,-1,1},
		{0,1,1}, {1,0,1}, 
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> baseBlock::getEdgeAndCornerAdjacentOnly() const
{
	std::vector<Block*> output;
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
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> baseBlock::getAdjacentOnSameZLevelOnly() const
{
	std::vector<Block*> output;
	output.reserve(4);
	static const int32_t offsetsList[4][3] = {
		{-1,0,0}, {1,0,0}, 
		{0,-1,0}, {0,1,0}
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
uint32_t baseBlock::distance(Block* block) const
{
	uint32_t dx = abs((int)m_x - (int)block->m_x);
	uint32_t dy = abs((int)m_y - (int)block->m_y);
	uint32_t dz = abs((int)m_z - (int)block->m_z);
	return pow((pow(dx, 2) + pow(dy, 2) + pow(dz, 2)), 0.5);
}
uint32_t baseBlock::taxiDistance(Block* block) const
{
	return abs((int)m_x - (int)block->m_x) + abs((int)m_y - (int)block->m_y) + abs((int)m_z - (int)block->m_z);
}
bool baseBlock::isAdjacentToAny(std::unordered_set<Block*>& blocks) const
{
	for(Block* adjacent : m_adjacentsVector)
		if(blocks.contains(adjacent))
			return true;
	return false;
}
void baseBlock::setNotSolid()
{
	m_solid = nullptr;
	for(Block* adjacent : m_adjacentsVector)
		if(adjacent->fluidCanEnterEver())
			for(auto& [fluidType, pair] : adjacent->m_fluids)
			{
				pair.second->m_fillQueue.addBlock(static_cast<Block*>(this));
				pair.second->m_stable = false;
			}
	clearMoveCostsCacheForSelfAndAdjacent();
}
void baseBlock::setSolid(const MaterialType* materialType)
{
	assert(materialType != nullptr);
	m_solid = materialType;
	// Displace fluids.
	m_totalFluidVolume = 0;
	for(auto& [fluidType, pair] : m_fluids)
	{
		pair.second->removeBlock(static_cast<Block*>(this));
		pair.second->addFluid(pair.first);
		// If there is no where to put the fluid.
		if(pair.second->m_drainQueue.m_set.empty() && pair.second->m_fillQueue.m_set.empty())
		{
			// If fluid piston is enabled then find a place above to add to potential.
			if constexpr (s_fluidPiston)
			{
				Block* above = m_adjacents[5];
				while(above != nullptr)
				{
					if(above->fluidCanEnterEver() && above->fluidCanEnterEver(fluidType) &&
							above->fluidCanEnterCurrently(fluidType)
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
	for(Block* adjacent : m_adjacentsVector)
		if(adjacent->fluidCanEnterEver())
			for(auto& [fluidType, pair] : adjacent->m_fluids)
				pair.second->m_fillQueue.removeBlock(static_cast<Block*>(this));
	// Possible expire path caches.
	// If more then one adjacent block can be entered then this block being cleared may open a new shortest path.
	if(std::ranges::count_if(m_adjacentsVector, [&](Block* block){ return block->anyoneCanEnterEver(); }) > 1)
		m_area->expireRouteCache();
	//TODO: Optionally clear diagonals as well.
	clearMoveCostsCacheForSelfAndAdjacent();
}
bool baseBlock::canEnterEver(Actor* actor) const
{
	return shapeAndMoveTypeCanEnterEver(actor->m_shape, actor->m_moveType);
}
bool baseBlock::isSolid() const
{
	return m_solid != nullptr;
}
const MaterialType* baseBlock::getSolidMaterial() const
{
	return m_solid;
}
void baseBlock::spawnMist(const FluidType* fluidType, uint32_t maxMistSpread)
{
	if(m_mist != nullptr and m_mist->density > fluidType->density)
		return;
	m_mist = fluidType;
	m_mistInverseDistanceFromSource = maxMistSpread != 0 ? maxMistSpread : fluidType->maxMistSpread;
	MistDisperseEvent* event = new MistDisperseEvent(s_step + fluidType->mistDuration, m_mist, static_cast<Block*>(this));
	m_area->scheduleEvent(event);
}
//TODO: This code puts the fluid into an adjacent group of the correct type if it can find one, it does not add the block or merge groups, leaving these tasks to fluidGroup readStep. Is this ok?
void baseBlock::addFluid(uint32_t volume, const FluidType* fluidType)
{
	// If a suitable fluid group exists already then just add to it's excessVolume.
	if(m_fluids.contains(fluidType))
	{
		assert(m_fluids.at(fluidType).second->m_fluidType == fluidType);
		m_fluids.at(fluidType).second->addFluid(volume);
		return;
	}
	m_fluids.emplace(fluidType, std::make_pair(volume, nullptr));
	m_totalFluidVolume += volume;
	// Find fluid group.
	FluidGroup* fluidGroup = nullptr;
	for(Block* adjacent : m_adjacentsVector)
		if(adjacent->fluidCanEnterEver() && adjacent->m_fluids.contains(fluidType))
		{
			assert(adjacent->m_fluids.at(fluidType).second->m_fluidType == fluidType);
			fluidGroup = adjacent->m_fluids.at(fluidType).second;
			fluidGroup->addBlock(static_cast<Block*>(this));
			continue;
		}
	// Create fluid group.
	if(fluidGroup == nullptr)
	{
		std::unordered_set<Block*> blocks({static_cast<Block*>(this)});
		fluidGroup = m_area->createFluidGroup(fluidType, blocks);
	}
	// Shift less dense fluids to excessVolume.
	if(m_totalFluidVolume > s_maxBlockVolume)
		resolveFluidOverfull();
}
void baseBlock::removeFluid(uint32_t volume, const FluidType* fluidType)
{
	m_fluids.at(fluidType).second->removeFluid(volume);
}
// Validate the actor can enter this block and also any other blocks required by it's Shape.
bool baseBlock::shapeAndMoveTypeCanEnterEver(const Shape* shape, const MoveType* moveType) const
{
	if(shape->positions.size() == 1)
	{
		const Block* block = static_cast<const Block*>(this);
		return block->anyoneCanEnterEver() && block->moveTypeCanEnter(moveType);
	}
	for(auto& [m_x, m_y, m_z, v] : shape->positions)
	{
		const Block* block = offset(m_x, m_y, m_z);
		if(block == nullptr || !block->anyoneCanEnterEver() || !block->moveTypeCanEnter(moveType))
			return false;
	}
	return true;
}
bool baseBlock::actorCanEnterCurrently(Actor* actor) const
{
	const Shape* shape = actor->m_shape;
	if(shape->positions.size() == 1)
	{
		const Block* block = static_cast<const Block*>(this);
		uint32_t v = shape->positions[0][3];
		return block->m_totalDynamicVolume + v <= s_maxBlockVolume;
	}
	for(auto& [x, y, z, v] : shape->positions)
	{
		const Block* block = offset(x, y, z);
		auto found = block->m_actors.find(actor);
		if(found != block->m_actors.end())
		{
			if(block->m_totalDynamicVolume + v - found->second > s_maxBlockVolume)
				return false;
		}
		else if(block->m_totalDynamicVolume + v > s_maxBlockVolume)
			return false;
	}
	return true;
}
Block* baseBlock::offset(int32_t ax, int32_t ay, int32_t az) const
{
	ax += m_x;
	ay += m_y;
	az += m_z;
	if(ax < 0 || (uint32_t)ax >= m_area->m_sizeX || ay < 0 || (uint32_t)ay >= m_area->m_sizeY || az < 0 || (uint32_t)az >= m_area->m_sizeZ)
		return nullptr;
	return &m_area->m_blocks[ax][ay][az];
}
bool baseBlock::fluidCanEnterCurrently(const FluidType* fluidType) const
{
	if(m_totalFluidVolume < s_maxBlockVolume)
		return true;
	for(auto& pair : m_fluids)
		if(pair.first->density < fluidType->density)
			return true;
	return false;
}
bool baseBlock::isAdjacentToFluidGroup(const FluidGroup* fluidGroup) const
{
	for(Block* block : m_adjacentsVector)
		if(block->m_fluids.contains(fluidGroup->m_fluidType) && block->m_fluids.at(fluidGroup->m_fluidType).second == fluidGroup)
			return true;
	return false;
}
uint32_t baseBlock::volumeOfFluidTypeCanEnter(const FluidType* fluidType) const
{
	assert(fluidType != nullptr);
	uint32_t output = s_maxBlockVolume;
	for(auto& pair : m_fluids)
		if(pair.first->density >= fluidType->density)
			output -= pair.second.first;
	return output;
}
uint32_t baseBlock::volumeOfFluidTypeContains(const FluidType* fluidType) const
{
	assert(fluidType != nullptr);
	auto found = m_fluids.find(fluidType);
	if(found == m_fluids.end())
		return 0;
	return found->second.first;
}
FluidGroup* baseBlock::getFluidGroup(const FluidType* fluidType) const
{
	assert(fluidType != nullptr);
	auto found = m_fluids.find(fluidType);
	if(found == m_fluids.end())
		return nullptr;
	assert(found->second.second->m_fluidType == fluidType);
	return found->second.second;
}
void baseBlock::resolveFluidOverfull()
{
	std::vector<const FluidType*> toErase;
	// Fluid types are sorted by density.
	for(auto& [fluidType, pair] : m_fluids)
	{
		assert(fluidType == pair.second->m_fluidType);
		// Displace lower density fluids.
		uint32_t displaced = std::min(pair.first, m_totalFluidVolume - s_maxBlockVolume);
		m_totalFluidVolume -= displaced;
		pair.first -= displaced;
		pair.second->addFluid(displaced);
		if(pair.first == 0)
			toErase.push_back(fluidType);
		if(pair.first < s_maxBlockVolume)
			pair.second->m_fillQueue.addBlock(static_cast<Block*>(this));
		if(m_totalFluidVolume == s_maxBlockVolume)
			break;
	}
	for(const FluidType* fluidType : toErase)
	{
		// If the last block of a fluidGroup is displaced disolve it in the lowest density liquid which is more dense then it.
		FluidGroup* fluidGroup = m_fluids.at(fluidType).second;
		assert(fluidGroup->m_fluidType = fluidType);
		fluidGroup->removeBlock(static_cast<Block*>(this));
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
void baseBlock::moveContentsTo(Block* block)
{
	if(isSolid())
	{
		const MaterialType* materialType = m_solid;
		setNotSolid();
		block->setSolid(materialType);
	}
	//TODO: other stuff falls?
}
// Add / remove  actor occupancy.
void baseBlock::enter(Actor* actor)
{
	assert(actor->m_location != static_cast<Block*>(this));
	bool hasPreviousLocation = actor->m_location != nullptr;
	actor->m_location = static_cast<Block*>(this);
	if(hasPreviousLocation)
	{
		actor->m_location->exit(actor);
		actor->m_blocks.clear();
		m_area->m_locationBuckets.update(actor, actor->m_location, static_cast<Block*>(this));
	}
	else
		m_area->m_locationBuckets.insert(actor);
	for(auto& [x, y, z, v] : actor->m_shape->positions)
	{
		Block* block = offset(x, y, z);
		assert(!block->m_actors.contains(actor));
		block->m_actors[actor] = v;
		block->m_totalDynamicVolume += v;
		actor->m_blocks.push_back(block);
	}
}
void baseBlock::exit(Actor* actor)
{
	for(Block* block : actor->m_blocks)
	{
		auto found = block->m_actors.find(actor);
		assert(found != block->m_actors.end());
		block->m_totalDynamicVolume -= found->second;
		block->m_actors.erase(found);
	}
}
std::vector<Block*> baseBlock::selectBetweenCorners(Block* otherBlock) const
{
	assert(otherBlock->m_x < m_area->m_sizeX);
	assert(otherBlock->m_y < m_area->m_sizeY);
	assert(otherBlock->m_z < m_area->m_sizeZ);
	std::vector<Block*> output;
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
std::string baseBlock::toS()
{
	std::string materialName = m_solid == nullptr ? "empty" : m_solid->name;
	std::string output;
	output.reserve(materialName.size() + 16 + (m_fluids.size() * 12));
	output = std::to_string(m_x) + ", " + std::to_string(m_y) + ", " + std::to_string(m_z) + ": " + materialName + ";";
	for(auto& [fluidType, pair] : m_fluids)
		output += fluidType->name + ":" + std::to_string(pair.first) + ";";
	return output;
}
/*
// Add / remove nongeneric non-actor occupancy for this block and also any other blocks required by it's Shape.
void baseBlock::place(HasShape* hasShape)
{
for(auto& [x, y, z, v] : hasShape->m_shape->positions)
{
Block* block = offset(x, y, z);
block->recordHasShapeAndVolume(hasShape, v);
}
hasShape->m_location = static_cast<Block*>(this);
}
void baseBlock::unplace(HasShape* hasShape)
{
for(auto& [x, y, z, v] : hasShape->m_shape->positions)
{
Block* block = offset(x, y, z);
block->removeHasShapeAndVolume(hasShape, v);
}
}
 *
 */
