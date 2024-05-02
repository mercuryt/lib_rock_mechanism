#include "hasShapes.h"
#include "../block.h"
#include "../shape.h"
#include "../item.h"
#include "../moveType.h"
void BlockHasShapes::record(HasShape& hasShape, CollisionVolume volume)
{
	assert(!m_shapes.contains(&hasShape));
	m_shapes[&hasShape] = volume;
	// Do not assert that maxBlockVolume has not been exceeded, volumes in excess of max are allowed in some circumstances.
	if(hasShape.m_static)
	{
		m_staticVolume += volume;
		// Static volume does not ever block actors from entering but it does effect move cost.
		clearCache();
	}
	else
		m_dynamicVolume += volume;
}
void BlockHasShapes::remove(HasShape& hasShape)
{
	assert(m_shapes.contains(&hasShape));
	auto volume = m_shapes.at(&hasShape);
	m_shapes.erase(&hasShape);
	if(hasShape.m_static)
	{
		m_staticVolume -= volume;
		// Static volume does not ever block actors from entering but it does effect move cost.
		clearCache();
	}
	else
		m_dynamicVolume -= volume;
}
void BlockHasShapes::enter(HasShape& hasShape)
{
	assert(hasShape.m_location != &m_block);
	assert(hasShape.m_blocks.empty());
	for(auto& [x, y, z, v] : hasShape.m_shape->positionsWithFacing(hasShape.m_facing))
	{
		Block* block = m_block.offset(x, y, z);
		assert(block);
		block->m_hasShapes.record(hasShape, v);
		hasShape.m_blocks.insert(block);
	}
	hasShape.m_location = &m_block;
}
void BlockHasShapes::exit(HasShape& hasShape)
{
	assert(hasShape.m_location == &m_block);
	for(Block* block : hasShape.m_blocks)
		block->m_hasShapes.remove(hasShape);
	hasShape.m_blocks.clear();
	hasShape.m_location = nullptr;
}
//TODO: Move this to item.
void BlockHasShapes::addQuantity(HasShape& hasShape, uint32_t quantity)
{
	assert(hasShape.isItem());
	Item& item = static_cast<Item&>(hasShape);
	assert(item.m_itemType.generic);
	exit(hasShape);
	item.addQuantity(quantity);
	enter(hasShape);
}
void BlockHasShapes::removeQuantity(HasShape& hasShape, uint32_t quantity)
{
	assert(hasShape.isItem());
	assert(hasShape.getQuantity() > quantity);
	Item& item = static_cast<Item&>(hasShape);
	assert(item.m_itemType.generic);
	exit(hasShape);
	item.removeQuantity(quantity);
	enter(hasShape);
}
void BlockHasShapes::tryToCacheMoveCosts(const Shape& shape, const MoveType& moveType, std::vector<std::pair<Block*, MoveCost>>& moveCosts)
{
	if(!m_moveCostsCache.contains(&shape) || !m_moveCostsCache.at(&shape).contains(&moveType))
		m_moveCostsCache[&shape][&moveType] = std::move(moveCosts);
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
bool BlockHasShapes::canEnterEverFrom(const HasShape& hasShape, const Block& block) const
{
	return shapeAndMoveTypeCanEnterEverFrom(*hasShape.m_shape, hasShape.getMoveType(), block);
}
bool BlockHasShapes::canEnterEverWithFacing(const HasShape& hasShape, const Facing facing) const
{
	return shapeAndMoveTypeCanEnterEverWithFacing(*hasShape.m_shape, hasShape.getMoveType(), facing);
}
bool BlockHasShapes::canEnterEverWithAnyFacing(const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(canEnterEverWithFacing(hasShape, facing))
			return true;
	return false;
}
bool BlockHasShapes::shapeAndMoveTypeCanEnterEverFrom(const Shape& shape, const MoveType& moveType, const Block& from) const
{
	assert(from.m_hasShapes.anythingCanEnterEver());
	if(!anythingCanEnterEver())
		return false;
	if(!moveTypeCanEnterFrom(moveType, from))
		return false;
	const Facing facing = m_block.facingToSetWhenEnteringFrom(from);
	return shapeAndMoveTypeCanEnterEverWithFacing(shape, moveType, facing);
}
bool BlockHasShapes::shapeAndMoveTypeCanEnterEverWithFacing(const Shape& shape, const MoveType& moveType, const Facing facing) const
{
	if(!anythingCanEnterEver())
		return false;
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		Block* block = m_block.offset(x, y, z);
		if(block == nullptr || block->isSolid() || !block->m_hasShapes.moveTypeCanEnter(moveType))
			return false;
	}
	return true;
}
bool BlockHasShapes::shapeAndMoveTypeCanEnterCurrentlyWithFacing(const Shape& shape, const MoveType& moveType, const Facing facing) const
{
	assert(anythingCanEnterEver());
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		Block* block = m_block.offset(x, y, z);
		assert(block);
		assert(!block->isSolid());
		if( block->m_hasShapes.m_dynamicVolume + v > Config::maxBlockVolume || 
			!block->m_hasShapes.moveTypeCanEnter(moveType)
		)
			return false;
	}
	return true;
}
bool BlockHasShapes::shapeAndMoveTypeCanEnterEverWithAnyFacing(const Shape& shape, const MoveType& moveType) const
{
	if(shape.isRadiallySymetrical)
		return shapeAndMoveTypeCanEnterCurrentlyWithFacing(shape, moveType, 0);
	for(Facing facing = 0; facing < 4; ++facing)
		if(shapeAndMoveTypeCanEnterCurrentlyWithFacing(shape, moveType, facing))
			return true;
	return false;
}
bool BlockHasShapes::moveTypeCanEnterFrom(const MoveType& moveType, const Block& from) const
{
	for(auto& [fluidType, volume] : moveType.swim)
	{
		// Can travel within and enter liquid from any angle.
		if(m_block.m_hasFluids.volumeOfFluidTypeContains(*fluidType) >= volume)
			return true;
		// Can leave liquid at any angle.
		if(from.m_hasFluids.volumeOfFluidTypeContains(*fluidType) >= volume)
			return true;
	}
	// Can always enter on same z level.
	if(m_block.m_z == from.m_z)
		return true;
	// Cannot go up if:
	if(m_block.m_z > from.m_z && !m_block.m_hasBlockFeatures.canEnterFromBelow())
		return false;
	// Cannot go down if:
	if(m_block.m_z < from.m_z && !m_block.m_hasBlockFeatures.canEnterFromAbove(from))
		return false;
	// Can enter from any angle if flying or climbing.
	if(moveType.fly || moveType.climb > 0)
		return true;
	// Can go up if from contains a ramp or stairs.
	if(m_block.m_z > from.m_z && (from.m_hasBlockFeatures.contains(BlockFeatureType::ramp) || from.m_hasBlockFeatures.contains(BlockFeatureType::stairs))) 
		return true;
	// Can go down if this contains a ramp or stairs.
	if(m_block.m_z < from.m_z && (m_block.m_hasBlockFeatures.contains(BlockFeatureType::ramp) || 
					m_block.m_hasBlockFeatures.contains(BlockFeatureType::stairs)
					))
		return true;
	return false;
}
bool BlockHasShapes::moveTypeCanEnter(const MoveType& moveType) const
{
	// Swiming.
	for(auto& [fluidType, pair] : m_block.m_hasFluids.getAll())
	{
		auto found = moveType.swim.find(fluidType);
		if(found != moveType.swim.end() && found->second <= pair.first)
		{
			// Can swim in fluid, check breathing requirements
			if(moveType.breathless)
				return true;
			if(moveTypeCanBreath(moveType))
				return true;
			Block* above = m_block.getBlockAbove();
			if(above && !above->isSolid() && above->m_hasShapes.moveTypeCanBreath(moveType))
				return true;
		}
	}
	// Not swimming and fluid level is too high.
	if(m_block.m_hasFluids.getTotalVolume() > Config::maxBlockVolume / 2)
		return false;
	// Fly can always enter if fluid level isn't preventing it.
	if(moveType.fly)
		return true;
	// Walk can enter only if can stand in or if also has climb2 and there is a isSupport() block adjacent.
	if(moveType.walk)
	{
		if(canStandIn())
			return true;
		else

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
	return false;
}
bool BlockHasShapes::moveTypeCanBreath(const MoveType& moveType) const
{
	if(m_block.m_hasFluids.getTotalVolume() < Config::maxBlockVolume && !moveType.onlyBreathsFluids)
		return true;
	for(auto& pair : m_block.m_hasFluids.getAll())
		//TODO: Minimum volume should probably be scaled by body size somehow.
		if(moveType.breathableFluids.contains(pair.first) && pair.second.first >= Config::minimumVolumeOfFluidToBreath)
			return true;
	return false;
}
bool BlockHasShapes::hasCachedMoveCosts(const Shape& shape, const MoveType& moveType) const
{
	return m_moveCostsCache.contains(&shape) && m_moveCostsCache.at(&shape).contains(&moveType);
}
const std::vector<std::pair<Block*, MoveCost>>& BlockHasShapes::getCachedMoveCosts(const Shape& shape, const MoveType& moveType) const
{
	assert(hasCachedMoveCosts(shape, moveType));
	return m_moveCostsCache.at(&shape).at(&moveType);
}
const std::vector<std::pair<Block*, MoveCost>> BlockHasShapes::makeMoveCosts(const Shape& shape, const MoveType& moveType) const
{
	std::vector<std::pair<Block*, MoveCost>> output;
	for(Block* block : m_block.getAdjacentWithEdgeAndCornerAdjacent())
		if(block->m_hasShapes.shapeAndMoveTypeCanEnterEverFrom(shape, moveType, m_block))
			output.emplace_back(block, block->m_hasShapes.moveCostFrom(moveType, m_block));
	return output;
}
MoveCost BlockHasShapes::moveCostFrom(const MoveType& moveType, const Block& from) const
{
	assert(!m_block.isSolid());
	assert(!from.isSolid());
	if(moveType.fly)
		return Config::baseMoveCost;
	for(auto& [fluidType, volume] : moveType.swim)
		if(m_block.m_hasFluids.volumeOfFluidTypeContains(*fluidType) >= volume)
			return Config::baseMoveCost;
	// Double cost to go up if not fly, swim, or ramp (if climb).
	if(m_block.m_z > from.m_z && !from.m_hasBlockFeatures.contains(BlockFeatureType::ramp))
		return Config::goUpMoveCost;
	return Config::baseMoveCost;
}
bool BlockHasShapes::canEnterCurrentlyFrom(const HasShape& hasShape, const Block& block) const
{
	const Facing facing = m_block.facingToSetWhenEnteringFrom(block);
	return canEnterCurrentlyWithFacing(hasShape, facing);
}
bool BlockHasShapes::canEnterCurrentlyWithFacing(const HasShape& hasShape, const Facing& facing) const
{
	for(auto& [x, y, z, v] : hasShape.m_shape->positionsWithFacing(facing))
	{
		Block* block = m_block.offset(x, y, z);
		assert(block != nullptr);
		if(block->m_hasShapes.m_dynamicVolume + v - block->m_hasShapes.getVolume(hasShape) > Config::maxBlockVolume)
			return false;
	}
	return true;
}
bool BlockHasShapes::canEnterCurrentlyWithAnyFacing(const HasShape& hasShape) const
{
	if(hasShape.m_shape->isRadiallySymetrical)
		return canEnterCurrentlyWithFacing(hasShape, 0);
	for(Facing facing = 0; facing < 4; ++facing)
		if(canEnterCurrentlyWithFacing(hasShape, facing))
			return true;
	return false;
}
std::pair<bool, Facing> BlockHasShapes::canEnterCurrentlyWithAnyFacingReturnFacing(const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(canEnterCurrentlyWithFacing(hasShape, facing))
			return {true, facing};
	return {false, 0};
}
bool BlockHasShapes::staticCanEnterCurrentlyWithFacing(const HasShape& hasShape, const Facing& facing) const
{
	return staticShapeCanEnterWithFacing(*hasShape.m_shape, facing);
}
bool BlockHasShapes::staticCanEnterCurrentlyWithAnyFacing(const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(staticCanEnterCurrentlyWithFacing(hasShape, facing))
			return true;
	return false;
}
std::pair<bool, Facing> BlockHasShapes::staticCanEnterCurrentlyWithAnyFacingReturnFacing(const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(staticCanEnterCurrentlyWithFacing(hasShape, facing))
			return {true, facing};
	return {false, 0};
}
bool BlockHasShapes::staticShapeCanEnterWithFacing(const Shape& shape, Facing facing) const
{
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		Block* block = m_block.offset(x, y, z);
		assert(block != nullptr);
		if(block->m_hasShapes.m_staticVolume + v > Config::maxBlockVolume)
			return false;
	}
	return true;
}
bool BlockHasShapes::staticShapeCanEnterWithAnyFacing(const Shape& shape) const
{
	if(shape.isRadiallySymetrical)
		return staticShapeCanEnterWithFacing(shape, 0);
	for(Facing facing = 0; facing < 4; ++facing)
		if(staticShapeCanEnterWithFacing(shape, facing))
			return true;
	return false;
}
CollisionVolume BlockHasShapes::getVolume(const HasShape& hasShape) const
{	
	auto found = m_shapes.find(const_cast<HasShape*>(&hasShape));
	if(found == m_shapes.end())
		return 0u;
	return found->second;
}
CollisionVolume BlockHasShapes::getDynamicVolume() const 
{
	return m_dynamicVolume;
}
uint32_t BlockHasShapes::getQuantityOfItemWhichCouldFit(const ItemType& itemType) const
{
	if(m_dynamicVolume > Config::maxBlockVolume)
		return 0;
	CollisionVolume freeVolume = Config::maxBlockVolume - m_staticVolume;
	return freeVolume / itemType.shape.getCollisionVolumeAtLocationBlock();
}
bool BlockHasShapes::canStandIn() const
{
	return (m_block.m_adjacents[0] != nullptr && (m_block.m_adjacents[0]->isSolid() || 
				m_block.m_adjacents[0]->m_hasBlockFeatures.canStandAbove())) || 
		m_block.m_hasBlockFeatures.canStandIn();
}
