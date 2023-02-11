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

baseBlock::baseBlock() : m_visionCacheVersion(0), m_routeCacheVersion(0) {}

void baseBlock::setup(Area* a, uint32_t ax, uint32_t ay, uint32_t az) {m_area=a;m_x=ax;m_y=ay;m_z=az;}

void baseBlock::recordAdjacent()
{
	static int32_t offsetsList[6][3] = {{0,0,1}, {0,1,0,}, {1,0,0}, {0,0,-1,}, {0,-1,0}, {-1,0,0}};
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

// Validate the nongeneric object can enter this block and also any other blocks required by it's Shape.
// TODO: Caches other blocks by Shape*?
// TODO: optimize for 1x1 case?
bool baseBlock::shapeCanEnterEver(Shape* shape) const
{
	for(auto& [m_x, m_y, m_z, v] : shape->m_positions)
	{
		baseBlock* block = offset(m_x, m_y, m_z);
		if(!block->canEnterEver()|| !block->hasFreeStaticVolume(v))
			return false;
	}
	return true;
}
bool baseBlock::shapeCanEnterCurrently(Shape* shape) const
{
	for(auto& [x, y, z, v] : shape->m_positions)
	{
		baseBlock* block = offset(x, y, z);
		if(!block->hasFreeDynamicVolume(v))
			return false;
	}
	return true;
}
Block* baseBlock::offset(uint32_t ax, uint32_t ay, uint32_t az) const
{
	if(ax < 0 || ax > m_area->m_sizeX || ay < 0 || ay > m_area->m_sizeY || az < 0 || az > m_area->m_sizeZ)
		return nullptr;
	return &m_area->m_blocks[m_x + ax][m_y + ay][m_z + az];
}
std::vector<std::pair<Block*, uint32_t>> baseBlock::getMoveCosts(Shape* shape, MoveType* moveType)
{
	std::vector<std::pair<Block*, uint32_t>> output;
	for(Block* block : m_adjacents)
		if(block->shapeCanEnterEver(shape) && block->moveTypeCanEnter(moveType))
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
/*
// Add / remove  actor occupancy.
void baseBlock::enter(Actor* actor)
{
	for(auto& [x, y, z, v] : actor->m_shape->m_positions)
	{
		Block* block = offset(x, y, z);
		block->recordActorAndVolume(actor, v);
	}
	actor->m_location = static_cast<Block*>(this);
}
void baseBlock::exit(Actor* actor)
{
	for(auto& [x, y, z, v] : actor->m_shape->m_positions)
	{
		Block* block = offset(x, y, z);
		block->removeActorAndVolume(actor, v);
	}
}
// Add / remove nongeneric non-actor occupancy for this block and also any other blocks required by it's Shape.
void baseBlock::place(HasShape* hasShape)
{
	for(auto& [x, y, z, v] : hasShape->m_shape->m_positions)
	{
		Block* block = offset(x, y, z);
		block->recordHasShapeAndVolume(hasShape, v);
	}
	hasShape->m_location = static_cast<Block*>(this);
}
void baseBlock::unplace(HasShape* hasShape)
{
	for(auto& [x, y, z, v] : hasShape->m_shape->m_positions)
	{
		Block* block = offset(x, y, z);
		block->removeHasShapeAndVolume(hasShape, v);
	}
}
*/
