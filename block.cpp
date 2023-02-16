/*
 * A block. Contains either a single type of material in 'solid' form or arbitrary objects with volume, generic solids and liquids.
 * Total volume is 100.
 */
#pragma once

#include <unordered_map>
#include <unordered_set>
#include <array>
#include <math.h>

#include "materialType.h"
#include "moveType.h"
#include "shape.h"
#include "hasShape.h"
#include "fluidType.h"

baseBlock::baseBlock() : m_routeCacheVersion(0), m_visionCacheVersion(0), m_solid(nullptr) {}

void baseBlock::setup(Area* a, uint32_t ax, uint32_t ay, uint32_t az) {m_area=a;m_x=ax;m_y=ay;m_z=az;}

void baseBlock::recordAdjacent()
{
	static int32_t offsetsList[6][3] = {{0,0,-1}, {0,-1,0}, {-1,0,0}, {0,1,0}, {1,0,0}, {0,0,1}};
	for(uint32_t i = 0; i < 6; i++)
	{
		auto& offsets = offsetsList[i];
		m_adjacents[i] = offset(offsets[0],offsets[1],offsets[2]);
	}
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
//TODO: This code puts the fluid into an adjacent group of the correct type if it can find one, it does not add the block or merge groups, leaving these tasks to fluidGroup readStep. Is this ok?
void baseBlock::addFluid(uint32_t volume, FluidType* fluidType)
{
	FluidGroup* fluidGroup = nullptr;
	if(m_fluids.contains(fluidType))
		fluidGroup = m_fluids[fluidType].second;
	else
		for(Block* adjacent : m_adjacents)
			if(adjacent != nullptr && adjacent->fluidCanEnterEver() && adjacent->m_fluids.contains(fluidType))
			{
				fluidGroup = adjacent->m_fluids[fluidType].second;
				break;
			}
	if(fluidGroup == nullptr)
	{
		uint32_t capacity = volumeOfFluidTypeCanEnter(fluidType);
		uint32_t flow = std::min(capacity, volume);
		volume -= flow; // Any extra is added to the group's m_excessVolume
		m_fluids[fluidType].first = flow;
		m_totalFluidVolume += flow;
		if(m_totalFluidVolume > MAX_BLOCK_VOLUME)
			resolveFluidOverfull();
		std::unordered_set<Block*> blocks({static_cast<Block*>(this)});
		fluidGroup = m_area->createFluidGroup(fluidType, blocks);
	}
	fluidGroup->addFluid(volume);
}
void baseBlock::removeFluid(uint32_t volume, FluidType* fluidType)
{
	m_fluids.at(fluidType).second->removeFluid(volume);
}
// Validate the nongeneric object can enter this block and also any other blocks required by it's Shape.
// TODO: Caches other blocks by Shape*?
// TODO: optimize for 1x1 case?
bool baseBlock::shapeCanEnterEver(Shape* shape) const
{
	for(auto& [m_x, m_y, m_z, v] : shape->positions)
	{
		Block* block = offset(m_x, m_y, m_z);
		if(!block->canEnterEver())
			return false;
	}
	return true;
}
bool baseBlock::shapeCanEnterCurrently(Shape* shape) const
{
	for(auto& [x, y, z, v] : shape->positions)
	{
		Block* block = offset(x, y, z);
		if(!block->m_totalDynamicVolume + v > MAX_BLOCK_VOLUME)
			return false;
	}
	return true;
}
Block* baseBlock::offset(uint32_t ax, uint32_t ay, uint32_t az) const
{
	ax += m_x;
	ay += m_y;
	az += m_z;
	if(ax < 0 || ax >= m_area->m_sizeX || ay < 0 || ay >= m_area->m_sizeY || az < 0 || az >= m_area->m_sizeZ)
		return nullptr;
	return &m_area->m_blocks[ax][ay][az];
}
std::vector<std::pair<Block*, uint32_t>> baseBlock::getMoveCosts(Shape* shape, MoveType* moveType)
{
	std::vector<std::pair<Block*, uint32_t>> output;
	for(Block* block : m_adjacents)
		if(block != nullptr && block->shapeCanEnterEver(shape) && block->moveTypeCanEnter(moveType))
			output.emplace_back(block, block->moveCost(moveType, static_cast<Block*>(this)));
	return output;
}
bool baseBlock::fluidCanEnterCurrently(FluidType* fluidType) const
{
	if(m_totalFluidVolume < MAX_BLOCK_VOLUME)
		return true;
	for(auto& pair : m_fluids)
		if(pair.first->density < fluidType->density)
			return true;
	return false;
}
uint32_t baseBlock::volumeOfFluidTypeCanEnter(FluidType* fluidType) const
{
	uint32_t output = MAX_BLOCK_VOLUME;
	for(auto& pair : m_fluids)
		if(pair.first->density >= fluidType->density)
			output -= pair.second.first;
	return output;
}
FluidGroup* baseBlock::getFluidGroup(FluidType* fluidType) const
{
	if(!m_fluids.contains(fluidType))
		return nullptr;
	return m_fluids.at(fluidType).second;
}
void baseBlock::resolveFluidOverfull()
{
	std::vector<FluidType*> toErase;
	for(auto& [fluidType, pair] : m_fluids)
	{
		// Displace lower density fluids.
		uint32_t displaced = std::min(m_fluids[fluidType].first, m_totalFluidVolume - MAX_BLOCK_VOLUME);
		m_totalFluidVolume -= displaced;
		m_fluids[fluidType].first -= displaced;
		m_fluids[fluidType].second->addFluid(displaced);
		if(m_fluids[fluidType].first == 0)
		{
			m_fluids[fluidType].second->removeBlock(static_cast<Block*>(this));
			toErase.push_back(fluidType);
		}
		if(m_totalFluidVolume == MAX_BLOCK_VOLUME)
			break;
	}
	for(FluidType* fluidType : toErase)
		m_fluids.erase(fluidType);
}
void baseBlock::moveContentsTo(Block* block)
{
	block->m_solid = m_solid;
	m_solid = nullptr;
//TODO: other stuff falls?
}
// Add / remove  actor occupancy.
void baseBlock::enter(Actor* actor)
{
	if(actor->m_location != nullptr)
		actor->m_location->exit(actor);
	for(auto& [x, y, z, v] : actor->m_shape->positions)
	{
		Block* block = offset(x, y, z);
		block->m_actors[actor] = v;
		block->m_totalDynamicVolume += v;
	}
	actor->m_location = static_cast<Block*>(this);
	}
void baseBlock::exit(Actor* actor)
{
	for(auto& [x, y, z, v] : actor->m_shape->positions)
	{
		Block* block = offset(x, y, z);
		block->m_actors.erase(actor);
		block->m_totalDynamicVolume -= v;
	}
}
std::string baseBlock::toS()
{
	std::string materialName = m_solid == nullptr ? "empty" : m_solid->name;
	return std::to_string(m_x) + ", " + std::to_string(m_y) + ", " + std::to_string(m_z) + ": " + materialName;
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
