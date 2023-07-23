#include "hasShape.h"
#include "block.h"
#include <cassert>
void HasShape::setStatic(bool isTrue)
{
	assert(m_static != isTrue);
	if(isTrue)
		for(auto& [x, y, z, v] : m_shape->positionsWithFacing(m_facing))
		{
			Block* block = m_location->offset(x, y, z);
			block->m_hasShapes.m_staticVolume += v;
		}
	else
		for(auto& [x, y, z, v] : m_shape->positionsWithFacing(m_facing))
		{
			Block* block = m_location->offset(x, y, z);
			block->m_hasShapes.m_staticVolume -= v;
		}
	m_static = isTrue;
}
bool HasShape::isAdjacentTo(HasShape& other) const
{
	assert(this != &other);
	for(Block* block : m_blocks)
	{
		if(other.m_blocks.contains(block))
			return true;
		for(Block* adjacent : block->m_adjacentsVector)
			if(other.m_blocks.contains(adjacent))
				return true;
	}
	return false;
}
bool HasShape::isAdjacentTo(Block& location) const
{
	for(Block* block : m_blocks)
	{
		if(block == &location)
			return true;
		for(Block* adjacent : block->m_adjacentsVector)
			if(adjacent == &location)
				return true;
	}
	return false;
}
std::unordered_set<Block*> HasShape::getOccupiedAndAdjacent()
{
	std::unordered_set<Block*> output(m_blocks);
	for(Block* block : m_blocks)
		output.insert(block->m_adjacentsVector.begin(), block->m_adjacentsVector.end());
	return output;
}
std::unordered_set<Block*> HasShape::getAdjacentBlocks()
{
	std::unordered_set<Block*> output(m_blocks);
	for(Block* block : m_blocks)
		output.insert(block->m_adjacentsVector.begin(), block->m_adjacentsVector.end());
	output.erase(m_blocks.begin(), m_blocks.end());
	return output;
}
std::unordered_set<HasShape*> HasShape::getAdjacentHasShapes()
{
	std::unordered_set<HasShape*> output;
	for(Block* block : m_blocks)
	{
		{
			auto& shapes = block->m_hasShapes.getShapes();
			for(auto& pair : shapes)
				output.insert(pair.first);
		}
		for(Block* adjacent : block->m_adjacentsVector)
		{
			auto& shapes = adjacent->m_hasShapes.getShapes();
			for(auto& pair : shapes)
				output.insert(pair.first);
		}
	}
	output.erase(this);
	return output;
}
void BlockHasShapes::record(HasShape& hasShape, uint32_t volume)
{
	assert(!m_shapes.contains(&hasShape));
	m_shapes[&hasShape] = volume;
	m_totalVolume += volume;
	// Do not assert that maxBlockVolume has not been exceeded, volumes in excess of max are allowed in some circumstances.
	if(hasShape.m_static)
	{
		m_staticVolume += volume;
		clearCache();
	}
}
void BlockHasShapes::remove(HasShape& hasShape)
{
	assert(m_shapes.contains(&hasShape));
	auto v = m_shapes.at(&hasShape);
	m_totalVolume -= v;
	m_shapes.erase(&hasShape);
	if(hasShape.m_static)
	{
		m_staticVolume -= v;
		clearCache();
	}
}
void BlockHasShapes::enter(HasShape& hasShape)
{
	assert(hasShape.m_location != &m_block);
	assert(hasShape.m_blocks.empty());
	for(auto& [x, y, z, v] : hasShape.m_shape->positionsWithFacing(hasShape.m_facing))
	{
		Block* block = m_block.offset(x, y, z);
		block->m_hasShapes.record(hasShape, v);
		hasShape.m_blocks.insert(block);
	}
	hasShape.m_location = &m_block;
}
void BlockHasShapes::exit(HasShape& hasShape)
{
	assert(hasShape.m_location == &m_block);
	for(auto& [x, y, z, v] : hasShape.m_shape->positionsWithFacing(hasShape.m_facing))
	{
		Block* block = m_block.offset(x, y, z);
		block->m_hasShapes.remove(hasShape);
	}
	hasShape.m_blocks.clear();
	hasShape.m_location = nullptr;
}
void BlockHasShapes::addQuantity(HasShape& hasShape, uint32_t quantity)
{
	assert(hasShape.isItem());
	Item& item = static_cast<Item&>(hasShape);
	assert(item.m_itemType.generic);
	exit(hasShape);
	item.m_quantity += quantity;
	enter(hasShape);
}
void BlockHasShapes::removeQuantity(HasShape& hasShape, uint32_t quantity)
{
	assert(hasShape.isItem());
	Item& item = static_cast<Item&>(hasShape);
	assert(item.m_itemType.generic);
	exit(hasShape);
	item.m_quantity -= quantity;
	enter(hasShape);
}
void BlockHasShapes::clearCache()
{
	m_moveCostsCache.clear();
	for(Block* block : m_block.getAdjacentWithEdgeAndCornerAdjacent())
		block->m_hasShapes.m_moveCostsCache.clear();
}
bool BlockHasShapes::anythingCanEnterEver() const
{
	if(m_block.isSolid())
		return false;
	// TODO: cache this.
	return !m_block.m_hasBlockFeatures.blocksEntrance();
}
bool BlockHasShapes::canEnterEverFrom(HasShape& hasShape, Block& block) const
{
	return shapeAndMoveTypeCanEnterEverFrom(*hasShape.m_shape, hasShape.getMoveType(), block);
}
bool BlockHasShapes::canEnterEverWithFacing(HasShape& hasShape, const uint8_t facing) const
{
	return shapeAndMoveTypeCanEnterEverWithFacing(*hasShape.m_shape, hasShape.getMoveType(), facing);
}
bool BlockHasShapes::shapeAndMoveTypeCanEnterEverFrom(const Shape& shape, const MoveType& moveType, Block& from) const
{
	const uint8_t facing = m_block.facingToSetWhenEnteringFrom(from);
	return shapeAndMoveTypeCanEnterEverWithFacing(shape, moveType, facing);
}
bool BlockHasShapes::shapeAndMoveTypeCanEnterEverWithFacing(const Shape& shape, const MoveType& moveType, const uint8_t facing) const
{
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		Block* block = m_block.offset(x, y, z);
		if(block == nullptr || block->isSolid() || 
				block->m_hasShapes.m_staticVolume + v > Config::maxBlockVolume || 
				!block->m_hasShapes.moveTypeCanEnter(moveType))
			return false;
	}
	return true;
}
bool BlockHasShapes::moveTypeCanEnter(const MoveType& moveType) const
{
	// Swiming.
	for(auto& [fluidType, pair] : m_block.m_fluids)
	{
		auto found = moveType.swim.find(fluidType);
		if(found != moveType.swim.end() && found->second <= pair.first)
			return true;
	}
	// Not swimming and fluid level is too high.
	if(m_block.m_totalFluidVolume > Config::maxBlockVolume / 2)
		return false;
	// Not flying and either not walking or ground is not supported.
	if(!moveType.fly && (!moveType.walk || !canStandIn()))
	{
		if(moveType.climb < 2)
			return false;
		else
		{
			// Only climb2 moveTypes can enter.
			for(Block* block : m_block.getAdjacentOnSameZLevelOnly())
				//TODO: check for climable features?
				if(block->isSupport())
					return true;
			return false;
		}
	}
	return true;
}
const std::vector<std::pair<Block*, uint32_t>>& BlockHasShapes::getMoveCosts(const Shape& shape, const MoveType& moveType)
{
	if(m_moveCostsCache.contains(&shape) && m_moveCostsCache.at(&shape).contains(&moveType))
		return m_moveCostsCache.at(&shape).at(&moveType);
	auto& output = m_moveCostsCache[&shape][&moveType];
	for(Block* block : m_block.m_adjacentsVector)
		if(block->m_hasShapes.shapeAndMoveTypeCanEnterEverFrom(shape, moveType, m_block))
			output.emplace_back(block, block->m_hasShapes.moveCostFrom(moveType, m_block));
	return output;
}
// Get a move cost for moving from a block onto this one for a given move type.
uint32_t BlockHasShapes::moveCostFrom(const MoveType& moveType, Block& from) const
{
	if(moveType.fly)
		return 10;
	for(auto& [fluidType, volume] : moveType.swim)
		if(m_block.volumeOfFluidTypeContains(*fluidType) >= volume)
			return 10;
	// Double cost to go up if not fly, swim, or ramp (if climb).
	if(m_block.m_z > from.m_z && !from.m_hasBlockFeatures.contains(BlockFeatureType::ramp))
		return 20;
	return 10;
}
bool BlockHasShapes::canEnterCurrentlyFrom(const HasShape& hasShape, const Block& block) const
{
	const uint8_t facing = m_block.facingToSetWhenEnteringFrom(block);
	return canEnterCurrentlyWithFacing(hasShape, facing);
}
bool BlockHasShapes::canEnterCurrentlyWithFacing(const HasShape& hasShape, const uint8_t& facing) const
{
	for(auto& [x, y, z, v] : hasShape.m_shape->positionsWithFacing(facing))
	{
		Block* block = m_block.offset(x, y, z);
		if(block->m_hasShapes.m_totalVolume + v - getVolume(hasShape) > Config::maxBlockVolume)
			return false;
	}
	return true;
}
uint32_t BlockHasShapes::getVolume(const HasShape& hasShape) const
{	
	auto found = m_shapes.find(const_cast<HasShape*>(&hasShape));
	if(found == m_shapes.end())
		return 0u;
	return found->second;
}
bool BlockHasShapes::canStandIn() const
{
	return (m_block.m_adjacents[0] != nullptr && (m_block.m_adjacents[0]->isSolid() || 
		m_block.m_adjacents[0]->m_hasBlockFeatures.canStandAbove())) || 
		m_block.m_hasBlockFeatures.canStandIn();
}
