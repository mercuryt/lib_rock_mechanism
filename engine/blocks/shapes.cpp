#include "blocks.h"
#include "../shape.h"
#include "../itemType.h"
#include "../moveType.h"
#include "../area/area.h"
#include "types.h"
bool Blocks::shape_anythingCanEnterEver(const BlockIndex& index) const
{
	// TODO: cache this in a bitset.
	if(solid_is(index))
		return false;
	return !blockFeature_blocksEntrance(index);
}
bool Blocks::shape_canFit(const BlockIndex& index, const ShapeId& shape, const Facing4& facing) const
{
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		BlockIndex otherIndex = offset(index, pair.offset);
		if(
			otherIndex.empty() ||
			!shape_anythingCanEnterEver(otherIndex) ||
			m_dynamicVolume[otherIndex] + pair.volume > Config::maxBlockVolume
		)
			return false;
	}
	return true;
}
bool Blocks::shape_shapeAndMoveTypeCanEnterEverFrom(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const BlockIndex& from) const
{
	assert(shape_anythingCanEnterEver(from));
	if(!shape_anythingCanEnterEver(index))
		return false;
	if(!shape_moveTypeCanEnterFrom(index, moveType, from))
		return false;
	const Facing4 facing = facingToSetWhenEnteringFrom(index, from);
	return shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, moveType, facing);
}
bool Blocks::shape_shapeAndMoveTypeCanEnterEverWithFacing(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing) const
{
	if(!shape_anythingCanEnterEver(index) || !shape_moveTypeCanEnter(index, moveType))
		return false;
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		BlockIndex otherIndex = offset(index, pair.offset);
		if(otherIndex.empty() || !shape_anythingCanEnterEver(otherIndex))
			return false;
	}
	return true;
}
bool Blocks::shape_canEnterCurrentlyWithFacing(const BlockIndex& index, const ShapeId& shape, const Facing4& facing, const OccupiedBlocksForHasShape& occupied) const
{
	assert(shape_anythingCanEnterEver(index));
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		BlockIndex otherIndex = offset(index, pair.offset);
		assert(otherIndex.exists());
		assert(shape_anythingCanEnterEver(otherIndex));
		if(occupied.contains(otherIndex))
			continue;
		if( m_dynamicVolume[otherIndex] + pair.volume > Config::maxBlockVolume)
			return false;
	}
	return true;
}
bool Blocks::shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const Facing4& facing, const OccupiedBlocksForHasShape& occupied) const
{
	assert(shape_anythingCanEnterEver(index));
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		BlockIndex otherIndex = offset(index, pair.offset);
		if(occupied.contains(otherIndex))
			continue;
		if(otherIndex.empty() || !shape_anythingCanEnterEver(otherIndex) ||
			m_dynamicVolume[otherIndex] + pair.volume > Config::maxBlockVolume ||
			!shape_moveTypeCanEnter(otherIndex, moveType)
		)
			return false;
	}
	return true;
}
bool Blocks::shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithAnyFacing(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType, const OccupiedBlocksForHasShape& occupied) const
{
	if(Shape::getIsRadiallySymetrical(shape))
		return shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(index, shape, moveType, Facing4::North, occupied);
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(index, shape, moveType, facing, occupied))
			return true;
	return false;
}
bool Blocks::shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(const BlockIndex& index, const ShapeId& shape, const MoveTypeId& moveType) const
{
	if(Shape::getIsRadiallySymetrical(shape))
		return shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, moveType, Facing4::North);
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, moveType, facing))
			return true;
	return false;
}
bool Blocks::shape_moveTypeCanEnterFrom(const BlockIndex& index, const MoveTypeId& moveType, const BlockIndex& from) const
{
	assert(shape_anythingCanEnterEver(index));
	assert(shape_moveTypeCanEnter(index, moveType));
	const DistanceInBlocks fromZ = getZ(from);
	const DistanceInBlocks toZ = getZ(index);
	// Floating requires fluid on same Z level.
	// TODO: Does not combine with other move types.
	if(MoveType::getFloating(moveType))
		return fluid_any(index) && toZ == fromZ;
	for(auto& [fluidType, volume] : MoveType::getSwim(moveType))
	{
		// Can travel within and enter liquid from any angle.
		if(fluid_volumeOfTypeContains(index, fluidType) >= volume)
			return true;
		// Can leave liquid at any angle.
		if(fluid_volumeOfTypeContains(from, fluidType) >= volume)
			return true;
	}
	// Can always enter on same z level.
	if(toZ == fromZ)
		return true;
	// Cannot go up if:
	if(toZ > fromZ && !blockFeature_canEnterFromBelow(index))
		return false;
	// Cannot go down if:
	if(toZ < fromZ && !blockFeature_canEnterFromAbove(index, from))
		return false;
	// Can enter from any angle if flying or climbing.
	if(MoveType::getFly(moveType) || MoveType::getClimb(moveType) > 0)
		return true;
	// Can go up if from contains a ramp or stairs.
	if(toZ > fromZ && (blockFeature_contains(from, BlockFeatureType::ramp) || blockFeature_contains(from, BlockFeatureType::stairs)))
		return true;
	// Can go down if this contains a ramp or stairs.
	if(toZ < fromZ && (blockFeature_contains(index, BlockFeatureType::ramp) ||
					blockFeature_contains(index, BlockFeatureType::stairs)
	))
		return true;
	return false;
}
bool Blocks::shape_moveTypeCanEnter(const BlockIndex& index, const MoveTypeId& moveType) const
{
	assert(shape_anythingCanEnterEver(index));
	// Swiming.
	for(const FluidData& fluidData : m_fluid[index])
	{
		auto found = MoveType::getSwim(moveType).find(fluidData.type);
		if(found != MoveType::getSwim(moveType).end() && found.second() <= fluidData.volume)
		{
			// Can swim in fluid, check breathing requirements
			if(MoveType::getBreathless(moveType))
				return true;
			if(shape_moveTypeCanBreath(index, moveType))
				return true;
			BlockIndex above = getBlockAbove(index);
			if(above.exists() && shape_anythingCanEnterEver(above) && shape_moveTypeCanBreath(above, moveType))
				return true;
		}
	}
	// Not swimming and fluid level is too high.
	if(m_totalFluidVolume[index] > Config::maxBlockVolume / 2)
		return false;
	// Fly can always enter if fluid level isn't preventing it.
	if(MoveType::getFly(moveType))
		return true;
	// Walk can enter only if can stand in or if also has climb2 and there is a isSupport() block adjacent.
	if(MoveType::getWalk(moveType))
	{
		if(shape_canStandIn(index))
			return true;
		else

			if(MoveType::getClimb(moveType) < 2)
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
bool Blocks::shape_moveTypeCanBreath(const BlockIndex& index, const MoveTypeId& moveType) const
{
	if(m_totalFluidVolume[index] < Config::maxBlockVolume && !MoveType::getOnlyBreathsFluids(moveType))
		return true;
	for(const FluidData& fluidData : m_fluid[index])
		//TODO: Minimum volume should probably be scaled by body size somehow.
		if(MoveType::getBreathableFluids(moveType).contains(fluidData.type) && fluidData.volume >= Config::minimumVolumeOfFluidToBreath)
			return true;
	return false;
}
MoveCost Blocks::shape_moveCostFrom(const BlockIndex& index, const MoveTypeId& moveType, const BlockIndex& from) const
{
	assert(shape_anythingCanEnterEver(index));
	assert(shape_anythingCanEnterEver(from));
	if(MoveType::getFly(moveType))
		return Config::baseMoveCost;
	for(auto& [fluidType, volume] : MoveType::getSwim(moveType))
		if(fluid_volumeOfTypeContains(index, fluidType) >= volume)
			return Config::baseMoveCost;
	// Double cost to go up if not fly, swim, or ramp (if climb).
	if(getZ(index) > getZ(from) && !blockFeature_contains(from, BlockFeatureType::ramp))
		return Config::goUpMoveCost;
	return Config::baseMoveCost;
}
bool Blocks::shape_canEnterCurrentlyFrom(const BlockIndex& index, const ShapeId& shape, const BlockIndex& block, const OccupiedBlocksForHasShape& occupied) const
{
	const Facing4 facing = facingToSetWhenEnteringFrom(index, block);
	return shape_canEnterCurrentlyWithFacing(index, shape, facing, occupied);
}
bool Blocks::shape_canEnterCurrentlyWithAnyFacing(const BlockIndex& index, const ShapeId& shape, const OccupiedBlocksForHasShape& occupied) const
{
	if(Shape::getIsRadiallySymetrical(shape))
		return shape_canEnterCurrentlyWithFacing(index, shape, Facing4::North, occupied);
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_canEnterCurrentlyWithFacing(index, shape, facing, occupied))
			return true;
	return false;
}
Facing4 Blocks::shape_canEnterCurrentlyWithAnyFacingReturnFacing(const BlockIndex& index, const ShapeId& shape, const OccupiedBlocksForHasShape& occupied) const
{
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_canEnterCurrentlyWithFacing(index, shape, facing, occupied))
			return facing;
	return Facing4::Null;
}
bool Blocks::shape_staticCanEnterCurrentlyWithFacing(const BlockIndex& index, const ShapeId& shape, const Facing4& facing, const OccupiedBlocksForHasShape& occupied) const
{
	return shape_staticShapeCanEnterWithFacing(index, shape, facing, occupied);
}
bool Blocks::shape_staticCanEnterCurrentlyWithAnyFacing(const BlockIndex& index, const ShapeId& shape, const OccupiedBlocksForHasShape& occupied) const
{
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_staticCanEnterCurrentlyWithFacing(index, shape, facing, occupied))
			return true;
	return false;
}
std::pair<bool, Facing4> Blocks::shape_staticCanEnterCurrentlyWithAnyFacingReturnFacing(const BlockIndex& index, const ShapeId& shape, const OccupiedBlocksForHasShape& occupied) const
{
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_staticCanEnterCurrentlyWithFacing(index, shape, facing, occupied))
			return {true, facing};
	return {false, Facing4::North};
}
bool Blocks::shape_staticShapeCanEnterWithFacing(const BlockIndex& index, const ShapeId& shape, const Facing4& facing, const OccupiedBlocksForHasShape& occupied) const
{
	for(const auto& pair : Shape::positionsWithFacing(shape, facing))
	{
		BlockIndex otherIndex = offset(index, pair.offset);
		assert(otherIndex.exists());
		if(occupied.contains(otherIndex))
			continue;
		if(m_staticVolume[otherIndex] + pair.volume > Config::maxBlockVolume)
			return false;
	}
	return true;
}
bool Blocks::shape_staticShapeCanEnterWithAnyFacing(const BlockIndex& index, const ShapeId& shape, const OccupiedBlocksForHasShape& occupied) const
{
	if(Shape::getIsRadiallySymetrical(shape))
		return shape_staticShapeCanEnterWithFacing(index, shape, Facing4::North, occupied);
	for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4((uint)facing + 1))
		if(shape_staticShapeCanEnterWithFacing(index, shape, facing, occupied))
			return true;
	return false;
}
CollisionVolume Blocks::shape_getDynamicVolume(const BlockIndex& index) const
{
	return m_dynamicVolume[index];
}
CollisionVolume Blocks::shape_getStaticVolume(const BlockIndex& index) const
{
	return m_staticVolume[index];
}
Quantity Blocks::shape_getQuantityOfItemWhichCouldFit(const BlockIndex& index, const ItemTypeId& itemType) const
{
	if(m_staticVolume[index] >= Config::maxBlockVolume)
		return Quantity::create(0);
	CollisionVolume freeVolume = Config::maxBlockVolume - m_staticVolume[index];
	return Quantity::create((freeVolume / Shape::getCollisionVolumeAtLocationBlock(ItemType::getShape(itemType))).get());
}
SmallSet<BlockIndex> Blocks::shape_getBelowBlocksWithFacing(const BlockIndex& index, const ShapeId& shape, const Facing4& facing) const
{
	// TODO: SIMD.
	SmallSet<BlockIndex> output;
	assert(getZ(index) != 0);
	auto occupiedAt = Shape::getBlocksOccupiedAt(shape, *this, index, facing);
	for(const BlockIndex& block : occupiedAt)
	{
		const BlockIndex& below = getBlockBelow(block);
		if(!occupiedAt.contains(below))
			output.insert(below);
	}
	return output;
}
bool Blocks::shape_canStandIn(const BlockIndex& index) const
{
	BlockIndex otherIndex = getBlockBelow(index);
	return (
		otherIndex.exists() && (solid_is(otherIndex) ||
		blockFeature_canStandAbove(otherIndex))) ||
		blockFeature_canStandIn(index) ||
		m_area.m_decks.blockIsPartOfDeck(index);
}
void Blocks::shape_addStaticVolume(const BlockIndex& index, const CollisionVolume& volume)
{
	m_staticVolume[index] += volume;
}
void Blocks::shape_removeStaticVolume(const BlockIndex& index, const CollisionVolume& volume)
{
	assert(m_staticVolume[index] >= volume);
	m_staticVolume[index] -= volume;
}
void Blocks::shape_addDynamicVolume(const BlockIndex& index, const CollisionVolume& volume)
{
	m_dynamicVolume[index] += volume;
}
void Blocks::shape_removeDynamicVolume(const BlockIndex& index, const CollisionVolume& volume)
{
	assert(m_dynamicVolume[index] >= volume);
	m_dynamicVolume[index] -= volume;
}