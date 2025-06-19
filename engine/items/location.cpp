#include "items.h"
#include "../area/area.h"
#include "../definitions/moveType.h"
#include "../portables.hpp"

ItemIndex Items::location_set(const ItemIndex& index, const BlockIndex& block, Facing4 facing)
{
	if(isStatic(index))
		return location_setStatic(index, block, facing);
	else
		return location_setDynamic(index, block, facing);
}
ItemIndex Items::location_setStatic(const ItemIndex& index, const BlockIndex& block, Facing4 facing)
{
	Blocks& blocks = m_area.getBlocks();
	#ifndef NDEBUG
		assert(index.exists());
		assert(isStatic(index));
		assert(block.exists());
		assert(m_location[index] != block);
	#endif
	Facing4 previousFacing = m_facing[index];
	if(isGeneric(index))
	{
		// Check for existing generic item to combine with.
		ItemIndex found = blocks.item_getGeneric(block, getItemType(index), getMaterialType(index));
		if(found.exists() && getLocation(found) == block && isStatic(found) && (!Shape::getIsMultiTile(getShape(index)) || m_facing[found] == m_facing[index]))
			// Return the index of the found item, which may be different then it was before 'index' was destroyed by merge.
			return merge(found, index);
	}
	BlockIndex previousLocation = m_location[index];
	DeckRotationData deckRotationData;
	if(previousLocation.exists())
	{
		deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForItem(index));
		location_clear(index);
	}
	m_location[index] = block;
	m_facing[index] = facing;
	auto& occupiedBlocks = m_blocks[index];
	if(m_constructedShape[index] != nullptr)
		m_constructedShape[index]->setLocationAndFacingStatic(m_area, previousFacing, block, facing, occupiedBlocks);
	else
		for(const auto& pair : Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing))
		{
			BlockIndex occupied = blocks.offset(block, pair.offset);
			blocks.item_recordStatic(occupied, index, pair.volume);
			occupiedBlocks.insert(occupied);
		}
	deckRotationData.reinstanceAtRotatedPosition(m_area, previousLocation, block, previousFacing, facing);
	onSetLocation(index, previousLocation, previousFacing);
	return index;
}
ItemIndex Items::location_setDynamic(const ItemIndex& index, const BlockIndex& block, Facing4 facing)
{
	assert(index.exists());
	assert(!isStatic(index));
	assert(block.exists());
	assert(m_location[index] != block);
	Blocks& blocks = m_area.getBlocks();
	BlockIndex previousLocation = m_location[index];
	Facing4 previousFacing = m_facing[index];
	DeckRotationData deckRotationData;
	if(previousLocation.exists())
	{
		deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForItem(index));
		location_clear(index);
	}
	m_location[index] = block;
	m_facing[index] = facing;
	auto& occupiedBlocks = m_blocks[index];
	if(m_constructedShape[index] != nullptr)
		m_constructedShape[index]->setLocationAndFacingDynamic(m_area, previousFacing, block, facing, occupiedBlocks);
	else
		for(const auto& pair : Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing))
		{
			BlockIndex occupied = blocks.offset(block, pair.offset);
			blocks.item_recordDynamic(occupied, index, pair.volume);
			occupiedBlocks.insert(occupied);
		}
	deckRotationData.reinstanceAtRotatedPosition(m_area, previousLocation, block, previousFacing, facing);
	onSetLocation(index, previousLocation, previousFacing);
	return index;
}
SetLocationAndFacingResult Items::location_tryToSetNongenericStatic(const ItemIndex& index, const BlockIndex& block, const Facing4 facing)
{
	assert(m_location[index].empty());
	assert(!ItemType::getIsGeneric(m_itemType[index]));
	assert(isStatic(index));
	Blocks& blocks = m_area.getBlocks();
	auto& occupiedBlocks = m_blocks[index];
	assert(occupiedBlocks.empty());
	Offset3D rollBackFrom;
	SetLocationAndFacingResult output = SetLocationAndFacingResult::Success;
	auto offsetsAndVolumes = Shape::makeOccupiedPositionsWithFacing(m_shape[index], facing);
	for(const OffsetAndVolume& pair : offsetsAndVolumes)
	{
		const BlockIndex& occupied = blocks.offset(block, pair.offset);
		if(blocks.solid_is(occupied) || blocks.blockFeature_blocksEntrance(occupied))
		{
			rollBackFrom = pair.offset;
			output =  SetLocationAndFacingResult::PermanantlyBlocked;
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
			blocks.item_recordStatic(occupied, index, pair.volume);
			occupiedBlocks.insert(occupied);
		}
	}
	if(output != SetLocationAndFacingResult::Success)
	{
		// Set location failed, roll back.
		for(const OffsetAndVolume& offsetAndVolume : offsetsAndVolumes)
		{
			if(offsetAndVolume.offset == rollBackFrom)
				break;
			const BlockIndex& notOccupied = blocks.offset(block, offsetAndVolume.offset);
			blocks.item_eraseStatic(notOccupied, index);
		}
		m_blocks[index].clear();
	}
	else
	{
		m_location[index] = block;
		m_facing[index] = facing;
	}
	return output;
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToSetGenericStatic(const ItemIndex& index, const BlockIndex& block, const Facing4 facing)
{
	assert(m_location[index].empty());
	assert(ItemType::getIsGeneric(m_itemType[index]));
	assert(isStatic(index));
	const ItemTypeId& itemType = m_itemType[index];
	Blocks& blocks = m_area.getBlocks();
	auto& occupiedBlocks = m_blocks[index];
	assert(occupiedBlocks.empty());
	Offset3D rollBackFrom;
	SetLocationAndFacingResult output = SetLocationAndFacingResult::Success;
	const ShapeId& shape = m_shape[index];
	ItemIndex toCombine = blocks.item_getGeneric(block, itemType, m_materialType[index]);
	if(toCombine.exists() && canCombine(toCombine, index))
	{
		toCombine = merge(toCombine, index);
		return {toCombine, output};
	}
	auto offsetsAndVolumes = Shape::makeOccupiedPositionsWithFacing(shape, facing);
	for(const OffsetAndVolume& pair : offsetsAndVolumes)
	{
		const BlockIndex& occupied = blocks.offset(block, pair.offset);
		if(blocks.solid_is(occupied) || blocks.blockFeature_blocksEntrance(occupied))
		{
			rollBackFrom = pair.offset;
			output = SetLocationAndFacingResult::PermanantlyBlocked;
			break;
		}
		if(blocks.shape_getStaticVolume(occupied) != 0)
		{
			output = SetLocationAndFacingResult::TemporarilyBlocked;
			rollBackFrom = pair.offset;
			break;
		}
		else
		{
			blocks.item_recordStatic(occupied, index, pair.volume);
			occupiedBlocks.insert(occupied);
		}
	}
	if(output != SetLocationAndFacingResult::Success)
	{
		// Set location failed, roll back.
		for(const OffsetAndVolume& offsetAndVolume : offsetsAndVolumes)
		{
			if(offsetAndVolume.offset == rollBackFrom)
				break;
			const BlockIndex& notOccupied = blocks.offset(block, offsetAndVolume.offset);
			blocks.item_eraseStatic(notOccupied, index);
		}
		m_blocks[index].clear();
	}
	else
	{
		m_location[index] = block;
		m_facing[index] = facing;
	}
	return {index, output};
}
SetLocationAndFacingResult Items::location_tryToSetDynamicInternal(const ItemIndex& index, const BlockIndex& location, const Facing4& facing)
{
	assert(m_location[index].empty());
	assert(!isStatic(index));
	Blocks& blocks = m_area.getBlocks();
	if(m_constructedShape[index] != nullptr)
	{
		// constructed shapes always have a facing assigned. It indicates the layout of the constructed shape data.
		SetLocationAndFacingResult result = m_constructedShape[index]->tryToSetLocationAndFacingDynamic(m_area, m_facing[index], location, facing, m_blocks[index]);
		if(result == SetLocationAndFacingResult::Success)
		{
			m_location[index] = location;
			m_facing[index] = facing;
		}
		return result;
	}
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
			blocks.item_recordDynamic(occupied, index, pair.volume);
			occupiedBlocks.insert(occupied);
		}
	}
	if(output != SetLocationAndFacingResult::Success)
	{
		// Set location failed, roll back.
		for(const OffsetAndVolume& offsetAndVolume : offsetsAndVolumes)
		{
			if(offsetAndVolume.offset == rollBackFrom)
				break;
			const BlockIndex& notOccupied = blocks.offset(location, offsetAndVolume.offset);
			blocks.item_eraseDynamic(notOccupied, index);
		}
		m_blocks[index].clear();
	}
	else
	{
		m_location[index] = location;
		m_facing[index] = facing;
	}
	return output;
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToSetStaticInternal(const ItemIndex& index, const BlockIndex& location, const Facing4& facing)
{
	std::pair<ItemIndex, SetLocationAndFacingResult> output;
	if(isGeneric(index))
		// Static generic.
		output = location_tryToSetGenericStatic(index, location, facing);
	else
		// Static nongeneric.
		output = {index, location_tryToSetNongenericStatic(index, location, facing)};
	if(output.second == SetLocationAndFacingResult::Success)
	{
		m_location[index] = location;
		m_facing[index] = facing;
	}
	return output;
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToSet(const ItemIndex& index, const BlockIndex& location, const Facing4& facing)
{
	if(isStatic(index))
		return location_tryToSetStatic(index, location, facing);
	else
		return {index, location_tryToSetDynamic(index, location, facing)};
}
SetLocationAndFacingResult Items::location_tryToSetDynamic(const ItemIndex& index, const BlockIndex& location, const Facing4& facing)
{
	assert(!hasLocation(index));
	assert(isStatic(index));
	SetLocationAndFacingResult output = location_tryToSetDynamicInternal(index, location, facing);
	if(output == SetLocationAndFacingResult::Success)
		onSetLocation(index, BlockIndex::null(), Facing4::Null);
	return output;
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToSetStatic(const ItemIndex& index, const BlockIndex& location, const Facing4& facing)
{
	assert(!hasLocation(index));
	assert(isStatic(index));
	std::pair<ItemIndex, SetLocationAndFacingResult> output = location_tryToSetStaticInternal(index, location, facing);
	if(output.second == SetLocationAndFacingResult::Success)
		onSetLocation(index, BlockIndex::null(), Facing4::Null);
	return output;
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToMoveToStatic(const ItemIndex& index, const BlockIndex& location)
{
	assert(hasLocation(index));
	assert(isStatic(index));
	const BlockIndex previousLocation = getLocation(index);
	const Facing4 previousFacing = getFacing(index);
	Blocks& blocks = m_area.getBlocks();
	const Facing4 facing = blocks.facingToSetWhenEnteringFrom(location, previousLocation);
	DeckRotationData deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForItem(index));
	location_clear(index);
	std::pair<ItemIndex, SetLocationAndFacingResult> output;
	if(isGeneric(index))
		// Static generic.
		output = location_tryToSetGenericStatic(index, location, facing);
	else
	{
		// Static nongeneric.
		output.first = index;
		output.second = location_tryToSetNongenericStatic(index, location, facing);
	}
	if(output.second == SetLocationAndFacingResult::Success)
	{
		SetLocationAndFacingResult deckSetLocationResult = deckRotationData.tryToReinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
		if(deckSetLocationResult == SetLocationAndFacingResult::Success)
			onSetLocation(index, previousLocation, previousFacing);
		else
			output.second = deckSetLocationResult;
	}
	if(output.second != SetLocationAndFacingResult::Success)
	{
		// Rollback.
		[[maybe_unused]] const SetLocationAndFacingResult status = location_tryToSetNongenericStatic(index, previousLocation, previousFacing);
		assert(status == SetLocationAndFacingResult::Success);
	}
	return output;
}
std::pair<ItemIndex, SetLocationAndFacingResult> Items::location_tryToMoveToDynamic(const ItemIndex& index, const BlockIndex& location)
{
	assert(!isStatic(index));
	const BlockIndex previousLocation = getLocation(index);
	const Facing4 previousFacing = getFacing(index);
	Blocks& blocks = m_area.getBlocks();
	const Facing4 facing = blocks.facingToSetWhenEnteringFrom(location, previousLocation);
	DeckRotationData deckRotationData = DeckRotationData::recordAndClearDependentPositions(m_area, ActorOrItemIndex::createForItem(index));
	location_clear(index);
	std::pair<ItemIndex, SetLocationAndFacingResult> output = {index, location_tryToSetDynamicInternal(index, location, facing)};
	if(output.second == SetLocationAndFacingResult::Success)
	{
		SetLocationAndFacingResult deckSetLocationResult = deckRotationData.tryToReinstanceAtRotatedPosition(m_area, previousLocation, location, previousFacing, facing);
		if(deckSetLocationResult == SetLocationAndFacingResult::Success)
			onSetLocation(index, previousLocation, previousFacing);
		else
			output.second = deckSetLocationResult;
	}
	// Not using else here: output.second may have changed due to deck occupants reinstance failing.
	if(output.second != SetLocationAndFacingResult::Success)
	{
		// Rollback.
		[[maybe_unused]] const SetLocationAndFacingResult status = location_tryToSetDynamicInternal(index, previousLocation, previousFacing);
		assert(status == SetLocationAndFacingResult::Success);
	}
	return output;
}
void Items::location_clear(const ItemIndex& index)
{
	if(isStatic(index))
		location_clearStatic(index);
	else
		location_clearDynamic(index);
}
void Items::location_clearStatic(const ItemIndex& index)
{
	assert(isStatic(index));
	assert(m_location[index].exists());
	BlockIndex location = m_location[index];
	Blocks& blocks = m_area.getBlocks();
	std::unique_ptr<ConstructedShape>& shapePtr = m_constructedShape[index];
	if(shapePtr != nullptr)
		shapePtr->recordAndClearStatic(m_area, location);
	else
		for(BlockIndex occupied : m_blocks[index])
			blocks.item_eraseStatic(occupied, index);
	m_location[index].clear();
	m_blocks[index].clear();
	if(blocks.isExposedToSky(location))
		m_onSurface.maybeUnset(index);
}
void Items::location_clearDynamic(const ItemIndex& index)
{
	assert(!isStatic(index));
	assert(m_location[index].exists());
	BlockIndex location = m_location[index];
	Blocks& blocks = m_area.getBlocks();
	std::unique_ptr<ConstructedShape>& shapePtr = m_constructedShape[index];
	if(shapePtr != nullptr)
		shapePtr->recordAndClearDynamic(m_area, location);
	else
		for(BlockIndex occupied : m_blocks[index])
			blocks.item_eraseDynamic(occupied, index);
	m_location[index].clear();
	m_blocks[index].clear();
	if(blocks.isExposedToSky(location))
		m_onSurface.maybeUnset(index);
}
bool Items::location_canEnterEverWithFacing(const ItemIndex& index, const BlockIndex& block, const Facing4& facing) const
{
	return m_area.getBlocks().shape_shapeAndMoveTypeCanEnterEverWithFacing(block, m_compoundShape[index], m_moveType[index], facing);
}
bool Items::location_canEnterCurrentlyWithFacing(const ItemIndex& index, const BlockIndex& block, const Facing4& facing) const
{
	return m_area.getBlocks().shape_canEnterCurrentlyWithFacing(block, m_compoundShape[index], facing, m_blocks[index]);
}
bool Items::location_canEnterEverFrom(const ItemIndex& index, const BlockIndex& block, const BlockIndex& previous) const
{
	return m_area.getBlocks().shape_shapeAndMoveTypeCanEnterEverFrom(block, m_compoundShape[index], m_moveType[index], previous);
}
bool Items::location_canEnterCurrentlyFrom(const ItemIndex& index, const BlockIndex& block, const BlockIndex& previous) const
{
	return m_area.getBlocks().shape_canEnterCurrentlyFrom(block, m_compoundShape[index], previous, m_blocks[index]);
}