#include "constructed.h"
#include "../area/area.h"
#include "../blocks/blocks.h"
#include "../blockFeature.h"
#include "../types.h"
void ConstructedShape::addBlock(Area& area, const BlockIndex& origin, const Facing4& facing, const BlockIndex& newBlock)
{
	Blocks& blocks = area.getBlocks();
	const Offset3D offset = blocks.relativeOffsetTo(origin, newBlock);
	const BlockIndex& rotatedBlock = blocks.offsetRotated(origin, offset, facing, Facing4::North);
	Offset3D rotatedOffset = blocks.relativeOffsetTo(origin, rotatedBlock);
	MaterialTypeId solid = blocks.solid_get(newBlock);
	if(solid.exists())
	{
		m_solidBlocks.insert(rotatedOffset, solid);
		m_fullDisplacement += FullDisplacement::createFromCollisionVolume(Config::maxBlockVolume);
		m_value += MaterialType::getValuePerUnitFullDisplacement(solid);
	}
	else
	{
		std::vector<BlockFeature> copy = blocks.blockFeature_getAll(newBlock);
		bool canEnter = true;
		for(const BlockFeature& blockFeature : copy)
		{
			if(blockFeature.blockFeatureType->blocksEntrance)
				canEnter = false;
			m_value += blockFeature.blockFeatureType->value * MaterialType::getValuePerUnitFullDisplacement(blockFeature.materialType);
			m_motiveForce += blockFeature.blockFeatureType->motiveForce;
		}
		if(!canEnter)
			m_fullDisplacement += FullDisplacement::createFromCollisionVolume(Config::maxBlockVolume);
		m_features.insert(rotatedOffset, std::move(copy));
	}
}
void ConstructedShape::removeBlock(Area& area, const BlockIndex& origin, const Facing4& facing, const BlockIndex& block)
{
	Blocks& blocks = area.getBlocks();
	const Offset3D offset = blocks.relativeOffsetTo(origin, block);
	const BlockIndex& rotatedBlock = blocks.offsetRotated(origin, offset, facing, Facing4::North);
	Offset3D rotatedOffset = blocks.relativeOffsetTo(origin, rotatedBlock);
	if(!m_features.maybeEraseNotify(rotatedOffset))
		m_solidBlocks.maybeErase(rotatedOffset);
}
void ConstructedShape::recordAndErase(Area& area, const BlockIndex& origin, const Facing4& facing)
{
	Blocks& blocks = area.getBlocks();
	for(const auto& [offset, materialType] : m_solidBlocks)
	{
		Offset3D rotatedOffset = offset;
		rotatedOffset.rotate2D(facing);
		const BlockIndex& block = blocks.offset(origin, rotatedOffset);
		assert(blocks.solid_get(block) == materialType);
		blocks.solid_setNot(block);
	}
	for(auto& pair : m_features)
	{
		Offset3D rotatedOffset = pair.first;
		rotatedOffset.rotate2D(facing);
		const BlockIndex& block = blocks.offset(origin, rotatedOffset);
		for(const BlockFeature& feature : pair.second)
			assert(blocks.blockFeature_contains(block, *feature.blockFeatureType));
		pair.second = blocks.blockFeature_getAll(block);
		blocks.blockFeature_removeAll(block);
	}
}
SetLocationAndFacingResult ConstructedShape::tryToSetLocationAndFacing(Area& area, const BlockIndex& previousLocation, const Facing4& previousFacing, const BlockIndex& location, const Facing4& facing)
{
	Blocks& blocks = area.getBlocks();
	if(previousLocation.exists())
		recordAndErase(area, previousLocation, previousFacing);
	Offset3D rollBackFrom;
	SetLocationAndFacingResult output = SetLocationAndFacingResult::Success;
	for(const auto& [offset, materialType] : m_solidBlocks)
	{
		Offset3D rotatedOffset = offset;
		rotatedOffset.rotate2D(facing);
		const BlockIndex& block = blocks.offset(location, rotatedOffset);
		if(blocks.solid_is(block) || !blocks.blockFeature_empty(block))
		{
			rollBackFrom = offset;
			output =  SetLocationAndFacingResult::PermanantlyBlocked;
			break;
		}
		if(output != SetLocationAndFacingResult::Success)
			continue;
		if(blocks.shape_getDynamicVolume(block) != 0)
		{
			output = SetLocationAndFacingResult::TemporarilyBlocked;
			rollBackFrom = offset;
		}
		else
			blocks.solid_set(block, materialType, true);
	}
	if(output != SetLocationAndFacingResult::Success)
	{
		for(const auto& [offset, materialType] : m_solidBlocks)
		{
			if(offset == rollBackFrom)
				break;
			Offset3D rotatedOffset = offset;
			rotatedOffset.rotate2D(facing);
			const BlockIndex& block = blocks.offset(location, rotatedOffset);
			blocks.solid_setNot(block);
		}
		SetLocationAndFacingResult result = tryToSetLocationAndFacing(area, BlockIndex::null(), Facing4::Null, previousLocation, previousFacing);
		assert(result == SetLocationAndFacingResult::Success);
		return output;
	}
	for(auto& [offset, features] : m_features)
	{
		Offset3D rotatedOffset = offset;
		rotatedOffset.rotate2D(facing);
		const BlockIndex& block = blocks.offset(location, rotatedOffset);
		if(blocks.solid_is(block) || !blocks.blockFeature_empty(block))
		{
			rollBackFrom = offset;
			output =  SetLocationAndFacingResult::PermanantlyBlocked;
			break;
		}
		if(blocks.shape_getDynamicVolume(block) != 0)
		{
			output = SetLocationAndFacingResult::TemporarilyBlocked;
			rollBackFrom = offset;
		}
		else
			blocks.blockFeature_setAll(block, features);
	}
	if(output != SetLocationAndFacingResult::Success)
	{
		for(const auto& [offset, materialType] : m_solidBlocks)
		{
			Offset3D rotatedOffset = offset;
			rotatedOffset.rotate2D(facing);
			const BlockIndex& block = blocks.offset(location, rotatedOffset);
			blocks.solid_setNot(block);
		}
		for(const auto& [offset, materialType] : m_features)
		{
			if(offset == rollBackFrom)
				break;
			Offset3D rotatedOffset = offset;
			rotatedOffset.rotate2D(facing);
			const BlockIndex& block = blocks.offset(location, rotatedOffset);
			blocks.blockFeature_removeAll(block);
		}
		SetLocationAndFacingResult result = tryToSetLocationAndFacing(area, BlockIndex::null(), Facing4::Null, previousLocation, previousFacing);
		assert(result == SetLocationAndFacingResult::Success);
	}
	return output;
}
std::tuple<ShapeId, ShapeId, OffsetCuboidSet> ConstructedShape::getShapeAndPathingShapeAndDecks() const
{
	SmallSet<OffsetAndVolume> shapeData;
	SmallSet<OffsetAndVolume> pathingShapeData;
	SmallSet<Offset3D> deckCandidates;
	OffsetCuboidSet decks;
	for(const auto& pair : m_solidBlocks)
	{
		shapeData.emplace(pair.first, Config::maxBlockVolume);
		pathingShapeData.emplace(pair.first, Config::maxBlockVolume);
		Offset3D above = pair.first;
		++above.z();
		deckCandidates.maybeInsert(above);
	}
	for(const auto& pair : m_features)
	{
		for(const BlockFeature& feature : pair.second)
		{
			if(feature.blockFeatureType->blocksEntrance)
			{
				shapeData.emplace(pair.first, Config::maxBlockVolume);
				break;
			}
			if(feature.blockFeatureType->canStandIn)
				deckCandidates.maybeInsert(pair.first);
			if(feature.blockFeatureType->canStandAbove)
			{
				Offset3D above = pair.first;
				++above.z();
				deckCandidates.maybeInsert(above);
			}
		}
		pathingShapeData.emplace(pair.first, Config::maxBlockVolume);
	}
	for(const Offset3D& offset : deckCandidates)
	{
		if(m_solidBlocks.contains(offset))
			continue;
		auto found = m_features.find(offset);
		if(found != m_features.end())
			for(const BlockFeature& feature : found->second)
				if(feature.blockFeatureType->blocksEntrance)
					continue;
		decks.addOrMerge(offset);
	}
	return {Shape::createCustom(shapeData), Shape::createCustom(pathingShapeData), decks};
}
ShapeId ConstructedShape::extendPathingShapeWithDecks(const DistanceInBlocks& height)
{
	SmallSet<OffsetAndVolume> onDeck;
	// Copy not reference.
	for(Offset3D offset : m_decks)
	{
		for(auto z = DistanceInBlocks::create(0); z < height; ++z)
		{
			onDeck.emplace(offset, Config::maxBlockVolume);
			++offset.z();
		}
	}
	return Shape::mutateAddMultiple(m_pathShape, onDeck);
}
ConstructedShape ConstructedShape::makeForKeelBlock(Area& area, const BlockIndex& block, const Facing4& facing)
{
	ConstructedShape output;
	Blocks& blocks = area.getBlocks();
	static const BlockFeatureType& keel = BlockFeatureType::byName("keel");
	auto keelCondition = [&](const BlockIndex& block) { return blocks.blockFeature_contains(block, keel); };
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
	auto connectedBlocks = blocks.collectAdjacentsWithCondition(block, occupiedCondition);
	for(const BlockIndex& block : connectedBlocks)
		if(block != center)
			output.addBlock(area, center, facing, block);
	for(const BlockIndex& block : keelBlocks)
		if(block != center)
			output.addBlock(area, center, facing, block);
	auto [occupiedShape, pathShape, decks] = output.getShapeAndPathingShapeAndDecks();
	output.m_occupationShape = occupiedShape;
	output.m_pathShape = pathShape;
	output.m_decks = decks;
	output.m_location = center;
	return output;
}