// TODO: code is repeated 3 times each for access condition and destination condition definition.
#include "terrainFacade.h"
#include "area.h"
#include "findsPath.h"
#include "shape.h"
#include "types.h"
#include <algorithm>
#include <queue>
#include <unordered_map>

TerrainFacade::TerrainFacade(Area& area, const MoveType& moveType) : m_area(area), m_moveType(moveType)
{
	m_enterable.resize(m_area.getBlocks().size() * maxAdjacent);
	for(BlockIndex block : m_area.getBlocks().getAll())
		update(block);
}
bool TerrainFacade::getValueForBit(BlockIndex from, BlockIndex to) const
{
	Blocks& blocks = m_area.getBlocks();
	return to != BLOCK_INDEX_MAX &&
		blocks.shape_anythingCanEnterEver(to) &&
		blocks.shape_moveTypeCanEnter(to, m_moveType) &&
		blocks.shape_moveTypeCanEnterFrom(to, m_moveType, from);
}
void TerrainFacade::update(BlockIndex index)
{
	Blocks& blocks = m_area.getBlocks();
	int i = 0;
	for(BlockIndex adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacentUnfiltered(index))
	{
		m_enterable[(index * maxAdjacent) + i] = getValueForBit(index, adjacent);
		++i;
	}
}
std::vector<BlockIndex> TerrainFacade::findPath(BlockIndex start, const DestinationCondition destinationCondition, const AccessCondition accessCondition, OpenListPush openListPush, OpenListPop openListPop, OpenListEmpty openListEmpty) const
{
	Blocks& blocks = m_area.getBlocks();
	std::unordered_map<BlockIndex, BlockIndex> closed;
	closed[start] = BLOCK_INDEX_MAX;
	openListPush(start);
	while(!openListEmpty())
	{
		BlockIndex current = openListPop();
		for(uint8_t adjacentCount = 0; adjacentCount < maxAdjacent; ++adjacentCount)
			if(canEnterFrom(current, adjacentCount))
			{
				BlockIndex adjacentIndex = blocks.indexAdjacentToAtCount(current, adjacentCount);
				assert(adjacentIndex != BLOCK_INDEX_MAX);
				if(closed.contains(adjacentIndex))
					continue;
				//TODO: Make variant for radially semetric shapes which ignores facing.
				Facing facing = blocks.facingToSetWhenEnteringFrom(adjacentIndex, current);
				if(!accessCondition(adjacentIndex, facing))
				{
					closed[adjacentIndex] = BLOCK_INDEX_MAX;
					continue;
				}
				if(destinationCondition(adjacentIndex, facing))
				{
					std::vector<BlockIndex> output;
					output.push_back(adjacentIndex);
					while(current != start)
					{
						output.push_back(current);
						current = closed[current];
					}
					std::reverse(output.begin(), output.end());
					return output;
				}
				closed[adjacentIndex] = current;
				openListPush(adjacentIndex);
			}
	}
	return {};
}
std::vector<BlockIndex> TerrainFacade::findPathBreathFirst(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition) const
{
	std::deque<BlockIndex> openList;
	OpenListEmpty empty = [&openList](){ return openList.empty(); };
	OpenListPush push  = [&openList](BlockIndex index) { openList.push_back(index); };
	OpenListPop pop = [&openList](){ BlockIndex output = openList.front(); openList.pop_front(); return output; };
	return findPath(from, destinationCondition, accessCondition, push, pop, empty);
}
std::vector<BlockIndex> TerrainFacade::findPathDepthFirst(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, BlockIndex huristicDestination) const
{
	Blocks& blocks = m_area.getBlocks();
	std::set<std::pair<DistanceInBlocks, BlockIndex>> openList;
	OpenListEmpty empty = [&openList](){ return openList.empty(); };
	OpenListPush push  = [&blocks, &openList, huristicDestination](BlockIndex index) 
	{
		DistanceInBlocks distance = blocks.taxiDistance(index, huristicDestination);
	       	openList.emplace(distance, index); 
	};
	OpenListPop pop = [&openList](){ BlockIndex output = openList.begin()->second; openList.erase(openList.begin()); return output; };
	return findPath(from, destinationCondition, accessCondition, push, pop, empty);
}
bool TerrainFacade::canEnterFrom(BlockIndex block, uint8_t adjacentCount) const
{
	Blocks& blocks = m_area.getBlocks();
	BlockIndex other = blocks.indexAdjacentToAtCount(block, adjacentCount);
	assert(m_enterable[(block * maxAdjacent) + adjacentCount] == getValueForBit(block, other));
	return m_enterable[(block * maxAdjacent) + adjacentCount];
}
std::vector<BlockIndex> TerrainFacade::findPathToForSingleBlockShape(BlockIndex start, BlockIndex target, bool detour, CollisionVolume volume) const
{
	DestinationCondition destinationCondition = [target](BlockIndex index, Facing){ return index == target; };
	Blocks& blocks = m_area.getBlocks();
	AccessCondition accessCondition;
	if(detour)
	{
		assert(volume);
		accessCondition = [volume, &blocks](BlockIndex index, Facing){
			return blocks.shape_getDynamicVolume(index) + volume <= Config::maxBlockVolume;
		};
	}
	else
		accessCondition = [](BlockIndex, Facing){ return true; };
	return findPathDepthFirst(start, destinationCondition, accessCondition, target);
}
std::vector<BlockIndex> TerrainFacade::findPathToForMultiBlockShape(BlockIndex start, const Shape& shape, BlockIndex target, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	AccessCondition accessCondition;
	if(detour)
		accessCondition = [this, &shape, &blocks](BlockIndex index, Facing facing){
			return blocks.shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(index, shape, m_moveType, facing);
		};
	else
		accessCondition = [this, &shape, &blocks](BlockIndex index, Facing facing){
			return blocks.shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, m_moveType, facing);
		};
	DestinationCondition destinationCondition = [&shape, &blocks, target](BlockIndex index, Facing facing)
	{ 
		auto occupied = shape.getBlocksOccupiedAt(blocks, index, facing);
		return std::ranges::find(occupied, target) != occupied.end();
	};
	return findPathDepthFirst(start, destinationCondition, accessCondition, target);
}
std::vector<BlockIndex> TerrainFacade::findPathToAnyOfForSingleBlockShape(BlockIndex start, std::vector<BlockIndex> indecies, BlockIndex huristicDestination, bool detour, CollisionVolume volume) const
{
	DestinationCondition destinationCondition = [indecies](BlockIndex index, Facing){ return std::ranges::find(indecies, index) != indecies.end(); };
	Blocks& blocks = m_area.getBlocks();
	AccessCondition accessCondition;
	if(detour)
	{
		assert(volume);
		accessCondition = [volume, &blocks](BlockIndex index, Facing){
			return blocks.shape_getDynamicVolume(index) + volume <= Config::maxBlockVolume;
		};
	}
	else
		accessCondition = [](BlockIndex, Facing){ return true; };
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreathFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathToAnyOfForMultiBlockShape(BlockIndex start, const Shape& shape, std::vector<BlockIndex> indecies, BlockIndex huristicDestination, bool detour) const
{
	DestinationCondition destinationCondition = [indecies](BlockIndex index, Facing){ return std::ranges::find(indecies, index) != indecies.end(); };
	Blocks& blocks = m_area.getBlocks();
	AccessCondition accessCondition;
	if(detour)
		accessCondition = [this, &shape, &blocks](BlockIndex index, Facing facing){
			return blocks.shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(index, shape, m_moveType, facing);
		};
	else
		accessCondition = [this, &shape, &blocks](BlockIndex index, Facing facing){
			return blocks.shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, m_moveType, facing);
		};
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreathFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathToConditionForSingleBlockShape(BlockIndex start, const DestinationCondition destinationCondition, BlockIndex huristicDestination, bool detour, CollisionVolume volume) const
{
	Blocks& blocks = m_area.getBlocks();
	AccessCondition accessCondition;
	if(detour)
	{
		assert(volume);
		accessCondition = [volume, &blocks](BlockIndex index, Facing){
			return blocks.shape_getDynamicVolume(index) + volume <= Config::maxBlockVolume;
		};
	}
	else
		accessCondition = [](BlockIndex, Facing){ return true; };
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreathFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathToConditionForMultiBlockShape(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristicDestination, bool detour) const
{
	AccessCondition accessCondition;
	Blocks& blocks = m_area.getBlocks();
	if(detour)
		accessCondition = [this, &shape, &blocks]([[maybe_unused]] BlockIndex index, Facing facing){
			return blocks.shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(index, shape, m_moveType, facing);
		};
	else
		accessCondition = [this, &shape, &blocks]([[maybe_unused]] BlockIndex index, Facing facing){
			return blocks.shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, m_moveType, facing);
		};
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreathFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathTo(BlockIndex start, const Shape& shape, BlockIndex target, bool detour) const
{
	if(shape.isMultiTile)
		return findPathToForMultiBlockShape(start, shape, target, detour);
	else
		return findPathToForSingleBlockShape(start, target, detour, shape.getCollisionVolumeAtLocationBlock());
}
std::vector<BlockIndex> TerrainFacade::findPathToAnyOf(BlockIndex start, const Shape& shape, std::vector<BlockIndex> indecies, BlockIndex huristicDestination, bool detour) const
{
	if(shape.isMultiTile)
		return findPathToAnyOfForMultiBlockShape(start, shape, indecies, huristicDestination, detour);
	else
		return findPathToAnyOfForSingleBlockShape(start, indecies, huristicDestination, detour, shape.getCollisionVolumeAtLocationBlock());
}
std::vector<BlockIndex> TerrainFacade::findPathToCondition(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristicDestination, bool detour) const
{
	if(shape.isMultiTile)
		return findPathToConditionForMultiBlockShape(start, shape, destinationCondition, huristicDestination, detour);
	else
		return findPathToConditionForSingleBlockShape(start, destinationCondition, huristicDestination, detour, shape.getCollisionVolumeAtLocationBlock());
}
std::vector<BlockIndex> TerrainFacade::findPathAdjacentTo(BlockIndex start, const Shape& shape, BlockIndex target, bool detour) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(blocks.getAdjacentWithEdgeAndCornerAdjacent(target), targets.begin(), 
		[&](BlockIndex block){ return blocks.shape_moveTypeCanEnter(block, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, shape, targets, target, detour);
}
std::vector<BlockIndex> TerrainFacade::findPathAdjacentTo(BlockIndex start, const Shape& shape, const HasShape& hasShape, bool detour) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(hasShape.getAdjacentBlocks(), targets.begin(), 
		[&](BlockIndex block){ return blocks.shape_moveTypeCanEnter(block, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, shape, targets, hasShape.m_location, detour);
}
std::vector<BlockIndex> TerrainFacade::findPathAdjacentToCondition(BlockIndex start, const Shape& shape, DestinationCondition condition, BlockIndex huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	DestinationCondition destinationCondition = [blocks, shape, condition](BlockIndex index, Facing facing)
	{
		for(BlockIndex adjacent : shape.getBlocksWhichWouldBeAdjacentAt(blocks, index, facing))
			if(condition(adjacent, 0))
				return true;
		return false;
	};
	return findPathToCondition(start, shape, condition, huristicDestination, detour);
}
std::vector<BlockIndex> TerrainFacade::findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, BlockIndex target, const Faction& faction, bool detour) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(blocks.getAdjacentWithEdgeAndCornerAdjacent(target), targets.begin(), 
		[&](BlockIndex block){ return !blocks.isReserved(block, faction) && blocks.shape_moveTypeCanEnter(block, m_moveType); 
	});
	if(targets.empty())
		return { };
	// TODO: optimization: If the shape is single tile and the target is enterable ever then path to target directly and discard the final block.
	return findPathToAnyOf(start, shape, targets, target, detour);
}
std::vector<BlockIndex> TerrainFacade::findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, const HasShape& hasShape, const Faction& faction, bool detour) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(hasShape.getAdjacentBlocks(), targets.begin(), 
		[&](BlockIndex block){ return !blocks.isReserved(block, faction) && blocks.shape_moveTypeCanEnter(block, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, shape, targets, hasShape.m_location, detour);
}
void AreaHasTerrainFacades::update(BlockIndex index)
{
	for(auto& pair : m_data)
		pair.second.update(index);
}
TerrainFacade& AreaHasTerrainFacades::at(const MoveType& moveType)
{
	assert(m_data.contains(&moveType));
	return m_data.at(&moveType);
}
void AreaHasTerrainFacades::maybeRegisterMoveType(const MoveType& moveType)
{
	//TODO: generate terrain facade in thread.
	auto found = m_data.find(&moveType);
	if(found == m_data.end())
	{
		auto [iter, success] = m_data.try_emplace(&moveType, m_area, moveType);
		assert(success);
	}
}
void AreaHasTerrainFacades::updateBlockAndAdjacent(BlockIndex block)
{
	Blocks& blocks = m_area.getBlocks();
	if(blocks.shape_anythingCanEnterEver(block))
		update(block);
	for(BlockIndex adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacent(block))
		if(blocks.shape_anythingCanEnterEver(adjacent))
			update(adjacent);
}
