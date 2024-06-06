#include "blocks.h"
#include "../shape.h"
#include "../item.h"
#include "../moveType.h"
#include "types.h"
void Blocks::shape_record(BlockIndex index, HasShape& hasShape, CollisionVolume volume)
{
	assert(!m_shapes.at(index).contains(&hasShape));
	m_shapes.at(index)[&hasShape] = volume;
	// Do not assert that maxBlockVolume has not been exceeded, volumes in excess of max are allowed in some circumstances.
	if(hasShape.isStatic())
		m_staticVolume.at(index) += volume;
	else
		m_dynamicVolume.at(index) += volume;
}
void Blocks::shape_remove(BlockIndex index, HasShape& hasShape)
{
	assert(m_shapes.at(index).contains(&hasShape));
	auto& shapes = m_shapes.at(index);
	auto volume = shapes.at(&hasShape);
	shapes.erase(&hasShape);
	if(hasShape.isStatic())
		m_staticVolume.at(index) -= volume;
	else
		m_dynamicVolume.at(index) -= volume;
}
void Blocks::shape_enter(BlockIndex index, HasShape& hasShape)
{
	assert(hasShape.m_location != index);
	assert(hasShape.m_blocks.empty());
	for(auto& [x, y, z, v] : hasShape.m_shape->positionsWithFacing(hasShape.m_facing))
	{
		BlockIndex block = offset(index, x, y, z);
		assert(block);
		shape_record(block, hasShape, v);
		hasShape.m_blocks.insert(block);
	}
	hasShape.m_location = index;
}
void Blocks::shape_exit(BlockIndex index, HasShape& hasShape)
{
	assert(hasShape.m_location == index);
	for(BlockIndex otherIndex : hasShape.m_blocks)
		shape_remove(otherIndex, hasShape);
	hasShape.m_blocks.clear();
	hasShape.m_location = BLOCK_INDEX_MAX;
}
//TODO: Move this to item.
void Blocks::shape_addQuantity(BlockIndex index, HasShape& hasShape, uint32_t quantity)
{
	assert(hasShape.isItem());
	Item& item = static_cast<Item&>(hasShape);
	assert(item.m_itemType.generic);
	shape_exit(index, hasShape);
	item.addQuantity(quantity);
	shape_enter(index, hasShape);
}
void Blocks::shape_removeQuantity(BlockIndex index, HasShape& hasShape, uint32_t quantity)
{
	assert(hasShape.isItem());
	assert(hasShape.getQuantity() > quantity);
	Item& item = static_cast<Item&>(hasShape);
	assert(item.m_itemType.generic);
	shape_exit(index, hasShape);
	item.removeQuantity(quantity);
	shape_enter(index, hasShape);
}
bool Blocks::shape_anythingCanEnterEver(BlockIndex index) const
{
	if(solid_is(index))
		return false;
	// TODO: cache this.
	return !blockFeature_blocksEntrance(index);
}
bool Blocks::shape_canEnterEverFrom(BlockIndex index, const HasShape& hasShape, BlockIndex from) const
{
	return shape_shapeAndMoveTypeCanEnterEverFrom(index, *hasShape.m_shape, hasShape.getMoveType(), from);
}
bool Blocks::shape_canEnterEverWithFacing(BlockIndex index, const HasShape& hasShape, const Facing facing) const
{
	return shape_shapeAndMoveTypeCanEnterEverWithFacing(index, *hasShape.m_shape, hasShape.getMoveType(), facing);
}
bool Blocks::shape_canEnterEverWithAnyFacing(BlockIndex index, const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_canEnterEverWithFacing(index, hasShape, facing))
			return true;
	return false;
}
bool Blocks::shape_shapeAndMoveTypeCanEnterEverFrom(BlockIndex index, const Shape& shape, const MoveType& moveType, BlockIndex from) const
{
	assert(shape_anythingCanEnterEver(from));
	if(!shape_anythingCanEnterEver(index))
		return false;
	if(!shape_moveTypeCanEnterFrom(index, moveType, from))
		return false;
	const Facing facing = facingToSetWhenEnteringFrom(index, from);
	return shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, moveType, facing);
}
bool Blocks::shape_shapeAndMoveTypeCanEnterEverWithFacing(BlockIndex index, const Shape& shape, const MoveType& moveType, const Facing facing) const
{
	if(!shape_anythingCanEnterEver(index))
		return false;
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		BlockIndex otherIndex = offset(index, x, y, z);
		if(otherIndex == BLOCK_INDEX_MAX || solid_is(otherIndex) || !shape_moveTypeCanEnter(otherIndex, moveType))
			return false;
	}
	return true;
}
bool Blocks::shape_shapeAndMoveTypeCanEnterCurrentlyWithFacing(BlockIndex index, const Shape& shape, const MoveType& moveType, const Facing facing) const
{
	assert(shape_anythingCanEnterEver(index));
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		BlockIndex otherIndex = offset(index,x, y, z);
		assert(otherIndex != BLOCK_INDEX_MAX);
		assert(!solid_is(otherIndex));
		if( m_dynamicVolume.at(otherIndex) + v > Config::maxBlockVolume || 
			!shape_moveTypeCanEnter(otherIndex, moveType)
		)
			return false;
	}
	return true;
}
bool Blocks::shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(BlockIndex index, const Shape& shape, const MoveType& moveType) const
{
	if(shape.isRadiallySymetrical)
		return shape_shapeAndMoveTypeCanEnterCurrentlyWithFacing(index, shape, moveType, 0);
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_shapeAndMoveTypeCanEnterCurrentlyWithFacing(index, shape, moveType, facing))
			return true;
	return false;
}
bool Blocks::shape_moveTypeCanEnterFrom(BlockIndex index, const MoveType& moveType, BlockIndex from) const
{
	assert(shape_anythingCanEnterEver(index));
	assert(shape_moveTypeCanEnter(index, moveType));
	for(auto& [fluidType, volume] : moveType.swim)
	{
		// Can travel within and enter liquid from any angle.
		if(fluid_volumeOfTypeContains(index, *fluidType) >= volume)
			return true;
		// Can leave liquid at any angle.
		if(fluid_volumeOfTypeContains(from, *fluidType) >= volume)
			return true;
	}
	// Can always enter on same z level.
	if(getZ(index) == getZ(from))
		return true;
	// Cannot go up if:
	if(getZ(index) > getZ(from) && !blockFeature_canEnterFromBelow(index))
		return false;
	// Cannot go down if:
	if(getZ(index) < getZ(from) && !blockFeature_canEnterFromAbove(index, from))
		return false;
	// Can enter from any angle if flying or climbing.
	if(moveType.fly || moveType.climb > 0)
		return true;
	// Can go up if from contains a ramp or stairs.
	if(getZ(index) > getZ(from) && (blockFeature_contains(from, BlockFeatureType::ramp) || blockFeature_contains(from, BlockFeatureType::stairs))) 
		return true;
	// Can go down if this contains a ramp or stairs.
	if(getZ(index) < getZ(from) && (blockFeature_contains(index, BlockFeatureType::ramp) || 
					blockFeature_contains(index, BlockFeatureType::stairs)
					))
		return true;
	return false;
}
bool Blocks::shape_moveTypeCanEnter(BlockIndex index, const MoveType& moveType) const
{
	assert(shape_anythingCanEnterEver(index));
	// Swiming.
	for(const FluidData& fluidData : m_fluid.at(index))
	{
		auto found = moveType.swim.find(fluidData.type);
		if(found != moveType.swim.end() && found->second <= fluidData.volume)
		{
			// Can swim in fluid, check breathing requirements
			if(moveType.breathless)
				return true;
			if(shape_moveTypeCanBreath(index, moveType))
				return true;
			BlockIndex above = getBlockAbove(index);
			if(above != BLOCK_INDEX_MAX && !solid_is(above) && shape_moveTypeCanBreath(above, moveType))
				return true;
		}
	}
	// Not swimming and fluid level is too high.
	if(m_totalFluidVolume[index] > Config::maxBlockVolume / 2)
		return false;
	// Fly can always enter if fluid level isn't preventing it.
	if(moveType.fly)
		return true;
	// Walk can enter only if can stand in or if also has climb2 and there is a isSupport() block adjacent.
	if(moveType.walk)
	{
		if(shape_canStandIn(index))
			return true;
		else

			if(moveType.climb < 2)
				return false;
			else
			{
				// Only climb2 moveTypes can enter.
				for(BlockIndex otherIndex : getAdjacentOnSameZLevelOnly(index))
					//TODO: check for climable features?
					if(isSupport(otherIndex))
						return true;
				return false;
			}
	}
	return false;
}
bool Blocks::shape_moveTypeCanBreath(BlockIndex index, const MoveType& moveType) const
{
	if(m_totalFluidVolume.at(index) < Config::maxBlockVolume && !moveType.onlyBreathsFluids)
		return true;
	for(const FluidData& fluidData : m_fluid.at(index))
		//TODO: Minimum volume should probably be scaled by body size somehow.
		if(moveType.breathableFluids.contains(fluidData.type) && fluidData.volume >= Config::minimumVolumeOfFluidToBreath)
			return true;
	return false;
}
const std::vector<std::pair<BlockIndex, MoveCost>> Blocks::shape_makeMoveCosts(BlockIndex index, const Shape& shape, const MoveType& moveType) const
{
	std::vector<std::pair<BlockIndex, MoveCost>> output;
	for(BlockIndex otherIndex : getAdjacentWithEdgeAndCornerAdjacent(index))
		if(shape_shapeAndMoveTypeCanEnterEverFrom(otherIndex, shape, moveType, index))
			output.emplace_back(otherIndex, shape_moveCostFrom(otherIndex, moveType, index));
	return output;
}
MoveCost Blocks::shape_moveCostFrom(BlockIndex index, const MoveType& moveType, BlockIndex from) const
{
	assert(!solid_is(index));
	assert(!solid_is(from));
	if(moveType.fly)
		return Config::baseMoveCost;
	for(auto& [fluidType, volume] : moveType.swim)
		if(fluid_volumeOfTypeContains(index, *fluidType) >= volume)
			return Config::baseMoveCost;
	// Double cost to go up if not fly, swim, or ramp (if climb).
	if(getZ(index) > getZ(from) && !blockFeature_contains(from, BlockFeatureType::ramp))
		return Config::goUpMoveCost;
	return Config::baseMoveCost;
}
bool Blocks::shape_canEnterCurrentlyFrom(BlockIndex index, const HasShape& hasShape, BlockIndex block) const
{
	const Facing facing = facingToSetWhenEnteringFrom(index, block);
	return shape_canEnterCurrentlyWithFacing(index, hasShape, facing);
}
bool Blocks::shape_canEnterCurrentlyWithFacing(BlockIndex index, const HasShape& hasShape, const Facing& facing) const
{
	for(auto& [x, y, z, v] : hasShape.m_shape->positionsWithFacing(facing))
	{
		BlockIndex otherIndex = offset(index, x, y, z);
		assert(otherIndex != BLOCK_INDEX_MAX);
		if(m_dynamicVolume.at(otherIndex) + v - shape_getVolume(otherIndex, hasShape) > Config::maxBlockVolume)
			return false;
	}
	return true;
}
bool Blocks::shape_canEnterCurrentlyWithAnyFacing(BlockIndex index, const HasShape& hasShape) const
{
	if(hasShape.m_shape->isRadiallySymetrical)
		return shape_canEnterCurrentlyWithFacing(index, hasShape, 0);
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_canEnterCurrentlyWithFacing(index, hasShape, facing))
			return true;
	return false;
}
std::pair<bool, Facing> Blocks::shape_canEnterCurrentlyWithAnyFacingReturnFacing(BlockIndex index, const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_canEnterCurrentlyWithFacing(index, hasShape, facing))
			return {true, facing};
	return {false, 0};
}
bool Blocks::shape_staticCanEnterCurrentlyWithFacing(BlockIndex index, const HasShape& hasShape, const Facing& facing) const
{
	return shape_staticShapeCanEnterWithFacing(index, *hasShape.m_shape, facing);
}
bool Blocks::shape_staticCanEnterCurrentlyWithAnyFacing(BlockIndex index, const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_staticCanEnterCurrentlyWithFacing(index, hasShape, facing))
			return true;
	return false;
}
std::pair<bool, Facing> Blocks::shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(BlockIndex index, const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_staticCanEnterCurrentlyWithFacing(index, hasShape, facing))
			return {true, facing};
	return {false, 0};
}
bool Blocks::shape_staticShapeCanEnterWithFacing(BlockIndex index, const Shape& shape, Facing facing) const
{
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		BlockIndex otherIndex = offset(index, x, y, z);
		assert(otherIndex != BLOCK_INDEX_MAX);
		if(m_staticVolume.at(otherIndex) + v > Config::maxBlockVolume)
			return false;
	}
	return true;
}
bool Blocks::shape_staticShapeCanEnterWithAnyFacing(BlockIndex index, const Shape& shape) const
{
	if(shape.isRadiallySymetrical)
		return shape_staticShapeCanEnterWithFacing(index, shape, 0);
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_staticShapeCanEnterWithFacing(index, shape, facing))
			return true;
	return false;
}
CollisionVolume Blocks::shape_getVolume(BlockIndex index, const HasShape& hasShape) const
{	
	auto& shapes = m_shapes.at(index);
	auto found = shapes.find(const_cast<HasShape*>(&hasShape));
	if(found == shapes.end())
		return 0u;
	return found->second;
}
CollisionVolume Blocks::shape_getDynamicVolume(BlockIndex index) const 
{
	return m_dynamicVolume.at(index);
}
CollisionVolume Blocks::shape_getStaticVolume(BlockIndex index) const 
{
	return m_staticVolume.at(index);
}
uint32_t Blocks::shape_getQuantityOfItemWhichCouldFit(BlockIndex index, const ItemType& itemType) const
{
	if(m_dynamicVolume.at(index) > Config::maxBlockVolume)
		return 0;
	CollisionVolume freeVolume = Config::maxBlockVolume - m_staticVolume.at(index);
	return freeVolume / itemType.shape.getCollisionVolumeAtLocationBlock();
}
bool Blocks::shape_canStandIn(BlockIndex index) const
{
	BlockIndex otherIndex = getBlockBelow(index);
	return (
		otherIndex != BLOCK_INDEX_MAX && (solid_is(otherIndex) || 
		blockFeature_canStandAbove(otherIndex))) || 
		blockFeature_canStandIn(index);
}
bool Blocks::shape_contains(BlockIndex index, HasShape& hasShape) const
{
	return m_shapes.at(index).contains(&hasShape);
}
std::unordered_map<HasShape*, CollisionVolume>& Blocks::shape_getShapes(BlockIndex index)
{
	return m_shapes.at(index);
}
void Blocks::shape_addStaticVolume(BlockIndex index, CollisionVolume volume)
{
	m_staticVolume.at(index) += volume;
}
void Blocks::shape_removeStaticVolume(BlockIndex index, CollisionVolume volume)
{
	assert(m_staticVolume.at(index) > volume);
	m_staticVolume.at(index) += volume;
}
