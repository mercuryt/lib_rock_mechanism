#include "actors.h"
#include "../area/area.h"
#include "../blocks/blocks.h"
#include "../portables.hpp"

void Actors::location_set(const ActorIndex& index, const BlockIndex& location, const Facing4 facing)
{
	if(isStatic(index))
		location_setStatic(index, location, facing);
	else
		location_setDynamic(index, location, facing);
}
void Actors::location_setStatic(const ActorIndex& index, const BlockIndex& location, const Facing4 facing)
{
	assert(location.exists());
	assert(facing != Facing4::Null);
	assert(isStatic(index));
	Blocks& blocks = m_area.getBlocks();
	BlockIndex previousLocation = m_location[index];
	Facing4 previousFacing = m_facing[index];
	SmallMap<BlockIndex, std::unique_ptr<DishonorCallback>> reservableData;
	DeckRotationData deckRotationData;
	if(previousLocation.exists())
	{
		deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForActor(index));
		location_clearStatic(index);
	}
	m_location[index] = location;
	m_facing[index] = facing;
	assert(m_blocks[index].empty());
	for(const OffsetAndVolume& pair : Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing))
	{
		BlockIndex occupied = blocks.offset(location, pair.offset);
		blocks.actor_recordStatic(occupied, index, pair.volume);
		m_blocks[index].insert(occupied);
	}
	// Record in vision facade if has location and can currently see.
	vision_maybeUpdateLocation(index, location);
	// TODO: redundant with location_clear also calling getReference.
	m_area.m_octTree.record(m_area, getReference(index));
	deckRotationData.reinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
	onSetLocation(index, previousLocation, previousFacing);
}
void Actors::location_setDynamic(const ActorIndex& index, const BlockIndex& location, const Facing4 facing)
{
	assert(location.exists());
	assert(facing != Facing4::Null);
	assert(!isStatic(index));
	Blocks& blocks = m_area.getBlocks();
	BlockIndex previousLocation = m_location[index];
	Facing4 previousFacing = m_facing[index];
	SmallMap<BlockIndex, std::unique_ptr<DishonorCallback>> reservableData;
	DeckRotationData deckRotationData;
	if(previousLocation.exists())
	{
		deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForActor(index));
		location_clearDynamic(index);
	}
	m_location[index] = location;
	m_facing[index] = facing;
	assert(m_blocks[index].empty());
	for(const OffsetAndVolume& pair : Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing))
	{
		const BlockIndex& occupied = blocks.offset(location, pair.offset);
		blocks.actor_recordDynamic(occupied, index, pair.volume);
		m_blocks[index].insert(occupied);
	}
	// Record in vision facade if has location and can currently see.
	vision_maybeUpdateLocation(index, location);
	// TODO: redundant with location_clear also calling getReference.
	m_area.m_octTree.record(m_area, getReference(index));
	deckRotationData.reinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
	onSetLocation(index, previousLocation, previousFacing);
}
// Used when item already has a location, rolls back position on failure.
SetLocationAndFacingResult Actors::location_tryToMoveToStatic(const ActorIndex& index, const BlockIndex& location)
{
	const BlockIndex previousLocation = getLocation(index);
	const Facing4 previousFacing = getFacing(index);
	Blocks& blocks = m_area.getBlocks();
	const Facing4 facing = blocks.facingToSetWhenEnteringFrom(location, previousLocation);
	DeckRotationData deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForActor(index));
	location_clear(index);
	SetLocationAndFacingResult output = location_tryToSetDynamicInternal(index, location, facing);
	if(output == SetLocationAndFacingResult::Success)
	{
		SetLocationAndFacingResult deckSetLocationResult = deckRotationData.tryToReinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
		if(deckSetLocationResult == SetLocationAndFacingResult::Success)
			onSetLocation(index, previousLocation, previousFacing);
		else
			output = deckSetLocationResult;
	}
	// Not using else here: output.second may have changed due to deck occupants reinstance failing.
	if(output != SetLocationAndFacingResult::Success)
	{
		// Rollback.
		[[maybe_unused]] const SetLocationAndFacingResult status = location_tryToSetDynamicInternal(index, previousLocation, previousFacing);
		assert(status == SetLocationAndFacingResult::Success);
	}
	return output;
}
SetLocationAndFacingResult Actors::location_tryToMoveToDynamic(const ActorIndex& index, const BlockIndex& location)
{
	const BlockIndex previousLocation = getLocation(index);
	const Facing4 previousFacing = getFacing(index);
	Blocks& blocks = m_area.getBlocks();
	const Facing4 facing = blocks.facingToSetWhenEnteringFrom(location, previousLocation);
	DeckRotationData deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForActor(index));
	location_clear(index);
	SetLocationAndFacingResult output = location_tryToSetDynamicInternal(index, location, facing);
	if(output != SetLocationAndFacingResult::Success)
		// Rollback.
		location_setDynamic(index, previousLocation, previousFacing);
	else
	{
		deckRotationData.reinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
		onSetLocation(index, previousLocation, previousFacing);
	}
	return output;
}
// Used when item does not have a location.
SetLocationAndFacingResult Actors::location_tryToSet(const ActorIndex& index, const BlockIndex& location, const Facing4& facing)
{
	if(isStatic(index))
		return location_tryToSetStatic(index, location, facing);
	else
		return location_tryToSetDynamic(index, location, facing);
}
SetLocationAndFacingResult Actors::location_tryToSetStatic(const ActorIndex& index, const BlockIndex& location, const Facing4& facing)
{
	assert(m_location[index].empty());
	SetLocationAndFacingResult output = location_tryToSetStaticInternal(index, location, facing);
	if(output == SetLocationAndFacingResult::Success)
		onSetLocation(index, BlockIndex::null(), Facing4::Null);
	return output;
}
SetLocationAndFacingResult Actors::location_tryToSetDynamic(const ActorIndex& index, const BlockIndex& location, const Facing4& facing)
{
	assert(m_location[index].empty());
	SetLocationAndFacingResult output = location_tryToSetDynamicInternal(index, location, facing);
	if(output == SetLocationAndFacingResult::Success)
		onSetLocation(index, BlockIndex::null(), Facing4::Null);
	return output;
}
SetLocationAndFacingResult Actors::location_tryToSetStaticInternal(const ActorIndex& index, const BlockIndex& location, const Facing4& facing)
{
	assert(m_location[index].empty());
	assert(isStatic(index));
	Blocks& blocks = m_area.getBlocks();
	auto& occupiedBlocks = m_blocks[index];
	assert(occupiedBlocks.empty());
	Offset3D rollBackFrom;
	SetLocationAndFacingResult output = SetLocationAndFacingResult::Success;
	auto offsetsAndVolumes = Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing);
	for(const OffsetAndVolume& pair : offsetsAndVolumes)
	{
		const BlockIndex& occupied = blocks.offset(location, pair.offset);
		if(blocks.solid_is(occupied) || blocks.blockFeature_blocksEntrance(occupied))
		{
			rollBackFrom = pair.offset;
			output = SetLocationAndFacingResult::PermanantlyBlocked;
			break;
		}
		if(blocks.shape_getStaticVolume(occupied) + pair.volume > Config::maxBlockVolume)
		{
			output = SetLocationAndFacingResult::TemporarilyBlocked;
			rollBackFrom = pair.offset;
			break;
		}
		else
		{
			blocks.actor_recordStatic(occupied, index, pair.volume);
			occupiedBlocks.insert(occupied);
		}
	}
	if(output == SetLocationAndFacingResult::Success)
	{
		m_location[index] = location;
		m_facing[index] = facing;
	}
	else
	{
		// Set location failed, roll back.
		for(const OffsetAndVolume& offsetAndVolume : offsetsAndVolumes)
		{
			if(offsetAndVolume.offset == rollBackFrom)
				break;
			const BlockIndex& notOccupied = blocks.offset(location, offsetAndVolume.offset);
			blocks.actor_eraseStatic(notOccupied, index);
		}
		m_blocks[index].clear();
	}
	return output;
}
SetLocationAndFacingResult Actors::location_tryToSetDynamicInternal(const ActorIndex& index, const BlockIndex& location, const Facing4& facing)
{
	assert(m_location[index].empty());
	assert(!isStatic(index));
	Blocks& blocks = m_area.getBlocks();
	auto& occupiedBlocks = m_blocks[index];
	assert(occupiedBlocks.empty());
	Offset3D rollBackFrom;
	SetLocationAndFacingResult output = SetLocationAndFacingResult::Success;
	auto offsetsAndVolumes = Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing);
	for(const OffsetAndVolume& pair : offsetsAndVolumes)
	{
		const BlockIndex& occupied = blocks.offset(location, pair.offset);
		if(blocks.solid_is(occupied) || blocks.blockFeature_blocksEntrance(occupied))
		{
			rollBackFrom = pair.offset;
			output = SetLocationAndFacingResult::PermanantlyBlocked;
			break;
		}
		if(blocks.shape_getDynamicVolume(occupied) + pair.volume > Config::maxBlockVolume)
		{
			output = SetLocationAndFacingResult::TemporarilyBlocked;
			rollBackFrom = pair.offset;
			break;
		}
		else
		{
			blocks.actor_recordDynamic(occupied, index, pair.volume);
			occupiedBlocks.insert(occupied);
		}
	}
	if(output == SetLocationAndFacingResult::Success)
	{
		m_location[index] = location;
		m_facing[index] = facing;
	}
	else
	{
		// Set location failed, roll back.
		for(const OffsetAndVolume& offsetAndVolume : offsetsAndVolumes)
		{
			if(offsetAndVolume.offset == rollBackFrom)
				break;
			const BlockIndex& notOccupied = blocks.offset(location, offsetAndVolume.offset);
			assert(m_blocks[index].contains(notOccupied));
			blocks.actor_eraseDynamic(notOccupied, index);
		}
		m_blocks[index].clear();
	}
	return output;

}
void Actors::location_clear(const ActorIndex& index)
{
	if(isStatic(index))
		location_clearStatic(index);
	else
		location_clearDynamic(index);
}
void Actors::location_clearStatic(const ActorIndex& index)
{
	assert(m_location[index].exists());
	BlockIndex location = m_location[index];
	m_area.m_octTree.erase(m_area, getReference(index));
	auto& blocks = m_area.getBlocks();
	for(BlockIndex occupied : m_blocks[index])
		blocks.actor_eraseStatic(occupied, index);
	m_location[index].clear();
	m_facing[index] = Facing4::Null;
	m_blocks[index].clear();
	if(blocks.isExposedToSky(location))
		m_onSurface.unset(index);
	move_pathRequestMaybeCancel(index);
}
void Actors::location_clearDynamic(const ActorIndex& index)
{
	assert(m_location[index].exists());
	BlockIndex location = m_location[index];
	m_area.m_octTree.erase(m_area, getReference(index));
	auto& blocks = m_area.getBlocks();
	for(BlockIndex occupied : m_blocks[index])
		blocks.actor_eraseDynamic(occupied, index);
	m_location[index].clear();
	m_facing[index] = Facing4::Null;
	m_blocks[index].clear();
	if(blocks.isExposedToSky(location))
		m_onSurface.unset(index);
	move_pathRequestMaybeCancel(index);
}
