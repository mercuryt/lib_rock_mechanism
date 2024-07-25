#include "blocks.h"
#include "../shape.h"
#include "../itemType.h"
#include "../moveType.h"
#include "types.h"
bool Blocks::shape_anythingCanEnterEver(BlockIndex index) const
{
	// TODO: cache this in a bitset.
	if(solid_is(index))
		return false;
	return !blockFeature_blocksEntrance(index);
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
	if(!shape_anythingCanEnterEver(index) || !shape_moveTypeCanEnter(index, moveType))
		return false;
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		BlockIndex otherIndex = offset(index, x, y, z);
		if(otherIndex.empty() || !shape_anythingCanEnterEver(otherIndex) || !shape_moveTypeCanEnter(otherIndex, moveType))
			return false;
	}
	return true;
}
bool Blocks::shape_canEnterCurrentlyWithFacing(BlockIndex index, const Shape& shape, Facing facing, const BlockIndices& occupied) const
{
	assert(shape_anythingCanEnterEver(index));
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		BlockIndex otherIndex = offset(index,x, y, z);
		assert(otherIndex.exists());
		assert(shape_anythingCanEnterEver(otherIndex));
		if(occupied.contains(otherIndex))
			continue;
		if( m_dynamicVolume.at(otherIndex) + v > Config::maxBlockVolume)
			return false;
	}
	return true;
}
bool Blocks::shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(BlockIndex index, const Shape& shape, const MoveType& moveType, const Facing facing, const BlockIndices& occupied) const
{
	assert(shape_anythingCanEnterEver(index));
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		BlockIndex otherIndex = offset(index,x, y, z);
		if(std::ranges::find(occupied, otherIndex) != occupied.end())
			continue;
		if(otherIndex.empty() || !shape_anythingCanEnterEver(otherIndex) ||
			m_dynamicVolume.at(otherIndex) + v > Config::maxBlockVolume || 
			!shape_moveTypeCanEnter(otherIndex, moveType)
		)
			return false;
	}
	return true;
}
bool Blocks::shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(BlockIndex index, const Shape& shape, const MoveType& moveType) const
{
	if(shape.isRadiallySymetrical)
		return shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, moveType, 0);
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, moveType, facing))
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
			if(above.exists() && shape_anythingCanEnterEver(above) && shape_moveTypeCanBreath(above, moveType))
				return true;
		}
	}
	// Not swimming and fluid level is too high.
	if(m_totalFluidVolume.at(index) > Config::maxBlockVolume / 2)
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
MoveCost Blocks::shape_moveCostFrom(BlockIndex index, const MoveType& moveType, BlockIndex from) const
{
	assert(shape_anythingCanEnterEver(index));
	assert(shape_anythingCanEnterEver(from));
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
bool Blocks::shape_canEnterCurrentlyFrom(BlockIndex index, const Shape& shape, BlockIndex block, const BlockIndices& occupied) const
{
	const Facing facing = facingToSetWhenEnteringFrom(index, block);
	return shape_canEnterCurrentlyWithFacing(index, shape, facing, occupied);
}
bool Blocks::shape_canEnterCurrentlyWithAnyFacing(BlockIndex index, const Shape& shape, const BlockIndices& occupied) const
{
	if(shape.isRadiallySymetrical)
		return shape_canEnterCurrentlyWithFacing(index, shape, 0, occupied);
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_canEnterCurrentlyWithFacing(index, shape, facing, occupied))
			return true;
	return false;
}
std::pair<bool, Facing> Blocks::shape_canEnterCurrentlyWithAnyFacingReturnFacing(BlockIndex index, const Shape& shape, const BlockIndices& occupied) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_canEnterCurrentlyWithFacing(index, shape, facing, occupied))
			return {true, facing};
	return {false, 0};
}
bool Blocks::shape_staticCanEnterCurrentlyWithFacing(BlockIndex index, const Shape& shape, const Facing& facing, const BlockIndices& occupied) const
{
	return shape_staticShapeCanEnterWithFacing(index, shape, facing, occupied);
}
bool Blocks::shape_staticCanEnterCurrentlyWithAnyFacing(BlockIndex index, const Shape& shape, const BlockIndices& occupied) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_staticCanEnterCurrentlyWithFacing(index, shape, facing, occupied))
			return true;
	return false;
}
std::pair<bool, Facing> Blocks::shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(BlockIndex index, const Shape& shape, const BlockIndices& occupied) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_staticCanEnterCurrentlyWithFacing(index, shape, facing, occupied))
			return {true, facing};
	return {false, 0};
}
bool Blocks::shape_staticShapeCanEnterWithFacing(BlockIndex index, const Shape& shape, Facing facing, const BlockIndices& occupied) const
{
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		BlockIndex otherIndex = offset(index, x, y, z);
		assert(otherIndex.exists());
		if(std::ranges::find(occupied, otherIndex) != occupied.end())
			continue;
		if(m_staticVolume.at(otherIndex) + v > Config::maxBlockVolume)
			return false;
	}
	return true;
}
bool Blocks::shape_staticShapeCanEnterWithAnyFacing(BlockIndex index, const Shape& shape, const BlockIndices& occupied) const
{
	if(shape.isRadiallySymetrical)
		return shape_staticShapeCanEnterWithFacing(index, shape, 0, occupied);
	for(Facing facing = 0; facing < 4; ++facing)
		if(shape_staticShapeCanEnterWithFacing(index, shape, facing, occupied))
			return true;
	return false;
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
		otherIndex.exists() && (solid_is(otherIndex) || 
		blockFeature_canStandAbove(otherIndex))) || 
		blockFeature_canStandIn(index);
}
void Blocks::shape_addStaticVolume(BlockIndex index, CollisionVolume volume)
{
	m_staticVolume.at(index) += volume;
}
void Blocks::shape_removeStaticVolume(BlockIndex index, CollisionVolume volume)
{
	assert(m_staticVolume.at(index) >= volume);
	m_staticVolume.at(index) -= volume;
}
