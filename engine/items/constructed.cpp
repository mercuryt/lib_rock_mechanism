#include "constructed.h"
#include "items.h"
#include "../area/area.h"
#include "../blocks/blocks.h"
#include "../blockFeature.h"
#include "../types.h"
ConstructedShape::ConstructedShape(const Json& data) { nlohmann::from_json(data, *this); }
void ConstructedShape::addBlock(Area& area, const BlockIndex& origin, const Facing4& facing, const BlockIndex& newBlock)
{
	Blocks& blocks = area.getBlocks();
	const Offset3D offset = blocks.relativeOffsetTo(origin, newBlock);
	const BlockIndex& rotatedBlock = blocks.offsetRotated(origin, offset, facing, Facing4::North);
	Offset3D rotatedOffset = blocks.relativeOffsetTo(origin, rotatedBlock);
	MaterialTypeId solid = blocks.solid_get(newBlock);
	bool canEnter = true;
	if(solid.exists())
	{
		m_solidBlocks.insert(rotatedOffset, solid);
		m_value += MaterialType::getValuePerUnitFullDisplacement(solid);
		m_mass += blocks.solid_getMass(newBlock);
		canEnter = false;
		blocks.solid_setNot(newBlock);
	}
	else
	{
		m_features.insert(rotatedOffset, blocks.blockFeature_getAllMove(newBlock));
		blocks.blockFeature_removeAll(newBlock);
		const auto& features = m_features.back().second;
		// If the block is not solid then it must contain at least one feature.
		assert(!features.empty());
		for(const BlockFeature& blockFeature : features)
		{
			const auto& featureType = BlockFeatureType::byId(blockFeature.blockFeatureTypeId);
			if(featureType.blocksEntrance)
				canEnter = false;
			m_value += featureType.value * MaterialType::getValuePerUnitFullDisplacement(blockFeature.materialType);
			m_motiveForce += featureType.motiveForce;
			// TODO: add mass for block features.
		}
	}
	if(!canEnter)
		m_fullDisplacement += FullDisplacement::createFromCollisionVolume(Config::maxBlockVolume);
}
void ConstructedShape::constructShape()
{
	// Collect all blocks making up the shape, including enterable features.
	SmallSet<OffsetAndVolume> blocks;
	for(const auto& pair : m_solidBlocks)
		blocks.maybeInsert({pair.first, Config::maxBlockVolume});
	for(const auto& pair : m_features)
		blocks.maybeInsert({pair.first, Config::maxBlockVolume});
	m_shape = Shape::createCustom(blocks);
	for(const Offset3D& offset : m_decks)
		blocks.maybeEmplace(offset, Config::maxBlockVolume);
	m_shapeIncludingDecks = Shape::createCustom(blocks);
}
void ConstructedShape::constructDecks()
{
	for(const auto& pair : m_solidBlocks)
	{
		Offset3D above = pair.first;
		++above.z();
		m_decks.addOrMerge(above);
		m_decks.maybeRemove(pair.first);
	}
	for(const auto& pair : m_features)
	{
		if(pair.second.blocksEntrance())
			m_decks.maybeRemove(pair.first);
		if(pair.second.canStandAbove())
		{
			Offset3D above = pair.first;
			++above.z();
			if(!m_solidBlocks.contains(above))
				m_decks.addOrMerge(above);
		}
	}
}
void ConstructedShape::recordAndClearDynamic(Area& area, const BlockIndex& origin)
{
	Blocks& blocks = area.getBlocks();
	for(const auto& [offset, materialType] : m_solidBlocks)
	{
		const BlockIndex& block = blocks.offset(origin, offset);
		assert(blocks.solid_get(block) == materialType);
		blocks.solid_setNotDynamic(block);
	}
	for(auto& [offset, features] : m_features)
	{
		const BlockIndex& block = blocks.offset(origin, offset);
		// Update the stored feature data with moved feature data from block.
		features = blocks.blockFeature_getAllMove(block);
		blocks.blockFeature_removeAll(block);
		blocks.unsetDynamic(block);
	}
}
void ConstructedShape::recordAndClearStatic(Area& area, const BlockIndex& origin)
{
	Blocks& blocks = area.getBlocks();
	for(const auto& [offset, materialType] : m_solidBlocks)
	{
		const BlockIndex& block = blocks.offset(origin, offset);
		assert(blocks.solid_get(block) == materialType);
		blocks.solid_setNot(block);
	}
	for(auto& [offset, features] : m_features)
	{
		const BlockIndex& block = blocks.offset(origin, offset);
		// Update the stored feature data with moved feature data from block.
		features = blocks.blockFeature_getAllMove(block);
		blocks.blockFeature_removeAll(block);
	}
}
SetLocationAndFacingResult ConstructedShape::tryToSetLocationAndFacingDynamic(Area& area, const Facing4& currentFacing, const BlockIndex& newLocation, const Facing4& newFacing, OccupiedBlocksForHasShape& occupied)
{
	Blocks& blocks = area.getBlocks();
	SetLocationAndFacingResult output = SetLocationAndFacingResult::Success;
	if(newFacing != currentFacing)
	{
		for(auto& [offset, materialType] : m_solidBlocks)
			offset.rotate2D(currentFacing, newFacing);
		for(auto& [offset, features] : m_features)
			offset.rotate2D(currentFacing, newFacing);
	}
	Offset3D rollback;
	for(const auto& [offset, materialType] : m_solidBlocks)
	{
		const BlockIndex block = blocks.offset(newLocation, offset);
		if(!blocks.shape_anythingCanEnterEver(block))
		{
			rollback = offset;
			output = SetLocationAndFacingResult::PermanantlyBlocked;
		}
		else if(!blocks.item_empty(block) || !blocks.actor_empty(block))
		{
			rollback = offset;
			output = SetLocationAndFacingResult::TemporarilyBlocked;
		}
		if(output != SetLocationAndFacingResult::Success)
		{
			// Contains is more expensive then permanant or temporary block test so check it last.
			if(occupied.contains(block))
			{
				output = SetLocationAndFacingResult::Success;
				rollback.clear();
			}
			else
				break;
		}
		blocks.solid_setDynamic(block, materialType, true);
		occupied.insert(block);
	}
	if(!rollback.empty())
	{
		// Remove all solid blocks placed so far.
		assert(output != SetLocationAndFacingResult::Success);
		for(const auto& [offset, materialType] : m_solidBlocks)
		{
			if(offset == rollback)
				break;
			const BlockIndex block = blocks.offset(newLocation, offset);
			blocks.solid_setNotDynamic(block);
		}
		occupied.clear();
		return output;
	}
	// Solid blocks placed successfully, try to place block features.
	for(auto& [offset, features] : m_features)
	{
		const BlockIndex block = blocks.offset(newLocation, offset);
		if(!blocks.shape_anythingCanEnterEver(block))
		{
			rollback = offset;
			output = SetLocationAndFacingResult::PermanantlyBlocked;
		}
		else if(!blocks.item_empty(block) || !blocks.actor_empty(block))
		{
			rollback = offset;
			output = SetLocationAndFacingResult::TemporarilyBlocked;
		}
		if(output != SetLocationAndFacingResult::Success)
		{
			// Contains is more expensive then permanant or temporary block test so check it last.
			if(occupied.contains(block))
			{
				output = SetLocationAndFacingResult::Success;
				rollback.clear();
			}
			else
				break;
		}
		blocks.blockFeature_setAllMoveDynamic(block, std::move(features));
		occupied.insert(block);
	}
	if(!rollback.empty())
	{
		// Remove all block features placed so far. Move them back into this object.
		assert(output != SetLocationAndFacingResult::Success);
		for(auto& [offset, features] : m_features)
		{
			if(offset == rollback)
				break;
			const BlockIndex block = blocks.offset(newLocation, offset);
			features = blocks.blockFeature_getAllMove(block);
			blocks.blockFeature_removeAll(block);
		}
		// Remove all solid blocks.
		assert(output != SetLocationAndFacingResult::Success);
		for(const auto& [offset, materialType] : m_solidBlocks)
		{
			const BlockIndex block = blocks.offset(newLocation, offset);
			blocks.solid_setNotDynamic(block);
		}
		occupied.clear();
		return output;
	}
	assert(output == SetLocationAndFacingResult::Success);
	return output;
}
void ConstructedShape::setLocationAndFacingDynamic(Area& area, const Facing4& currentFacing, const BlockIndex& newLocation, const Facing4& newFacing, OccupiedBlocksForHasShape& occupied)
{
	Blocks& blocks = area.getBlocks();
	if(newFacing != currentFacing)
	{
		for(auto& [offset, materialType] : m_solidBlocks)
			offset.rotate2D(currentFacing, newFacing);
		for(auto& [offset, features] : m_features)
			offset.rotate2D(currentFacing, newFacing);
	}
	for(const auto& [offset, materialType] : m_solidBlocks)
	{
		const BlockIndex block = blocks.offset(newLocation, offset);
		assert(blocks.shape_anythingCanEnterEver(block));
		assert(blocks.item_empty(block) && blocks.actor_empty(block));
		blocks.solid_setDynamic(block, materialType, true);
		occupied.insert(block);
	}
	for(auto& [offset, features] : m_features)
	{
		const BlockIndex block = blocks.offset(newLocation, offset);
		blocks.blockFeature_setAllMoveDynamic(block, std::move(features));
		occupied.insert(block);
	}
}
void ConstructedShape::setLocationAndFacingStatic(Area& area, const Facing4& currentFacing, const BlockIndex& newLocation, const Facing4& newFacing, OccupiedBlocksForHasShape& occupied)
{
	Blocks& blocks = area.getBlocks();
	if(newFacing != currentFacing)
	{
		for(auto& [offset, materialType] : m_solidBlocks)
			offset.rotate2D(currentFacing, newFacing);
		for(auto& [offset, features] : m_features)
			offset.rotate2D(currentFacing, newFacing);
	}
	for(const auto& [offset, materialType] : m_solidBlocks)
	{
		const BlockIndex block = blocks.offset(newLocation, offset);
		assert(blocks.shape_anythingCanEnterEver(block));
		assert(blocks.item_empty(block) && blocks.actor_empty(block));
		blocks.solid_set(block, materialType, true);
		occupied.insert(block);
	}
	for(auto& [offset, features] : m_features)
	{
		const BlockIndex block = blocks.offset(newLocation, offset);
		// Not bothering with move here because this method shouldn't be very hot.
		blocks.blockFeature_setAll(block, features);
		occupied.insert(block);
	}
}
SmallSet<MaterialTypeId> ConstructedShape::getMaterialTypesAt(const Area& area, const BlockIndex& location, const Facing4& facing, const BlockIndex& block) const
{
	const Blocks& blocks = area.getBlocks();
	SmallSet<MaterialTypeId> output;
	Offset3D offset = blocks.relativeOffsetTo(location, block);
	// All m_solid and m_features are both stored with north facing, subtract the current facing from it's self to convert the offset to north.
	offset.rotate2DInvert(facing);
	auto found = m_solidBlocks.find(offset);
	if(found != m_solidBlocks.end())
		output.insert(found->second);
	else
	{
		auto found2 = m_features.find(offset);
		assert(found2 != m_features.end());
		for(const BlockFeature& feature : found2->second)
			output.insert(feature.materialType);
	}
	return output;
}
std::pair<ConstructedShape, BlockIndex> ConstructedShape::makeForKeelBlock(Area& area, const BlockIndex& block, const Facing4& facing)
{
	ConstructedShape output;
	Blocks& blocks = area.getBlocks();
	auto keelCondition = [&](const BlockIndex& block) { return blocks.blockFeature_contains(block, BlockFeatureTypeId::Keel); };
	auto keelBlocks = blocks.collectAdjacentsWithCondition(block, keelCondition);
	Offset3D sum = Offset3D::create(0,0,0);
	for(const BlockIndex& block : keelBlocks)
		sum += blocks.getCoordinates(block).toOffset();
	Point3D average = Point3D::create(sum / keelBlocks.size());
	DistanceInBlocks lowestZ = average.z();
	for(const BlockIndex& block : keelBlocks)
		lowestZ = std::min(lowestZ, blocks.getZ(block));
	average.setZ(lowestZ);
	BlockIndex center = blocks.getIndex(average);
	output.addBlock(area, center, facing, center);
	// Only keel blocks are allowed on same level as center.
	auto occupiedCondition = [&](const BlockIndex& block) {
		return ( blocks.solid_is(block) || !blocks.blockFeature_empty(block)) && blocks.getZ(block) > lowestZ;
	};
	auto connectedBlocks = blocks.collectAdjacentsWithCondition(center, occupiedCondition);
	for(const BlockIndex& block : connectedBlocks)
		if(block != center)
			output.addBlock(area, center, facing, block);
	for(const BlockIndex& block : keelBlocks)
		if(block != center)
			output.addBlock(area, center, facing, block);
	output.constructShape();
	output.constructDecks();
	return {output, center};
}
Json ConstructedShape::toJson() const
{
	return *this;
}