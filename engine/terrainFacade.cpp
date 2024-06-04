#include "terrainFacade.h"
#include "area.h"
#include "types.h"
#include "shape.h"
#include <algorithm>
#include <queue>
#include <unordered_map>

TerrainFacade::TerrainFacade(Area& area, const MoveType& moveType) : m_area(area), m_moveType(moveType)
{
	m_enterable.resize(m_area.getBlocks().size() * maxAdjacent);
}
void TerrainFacade::initalize()
{
	for(BlockIndex block : m_area.getBlocks().getAll())
		update(block);
}
void TerrainFacade::update(BlockIndex index)
{
	Blocks& blocks = m_area.getBlocks();
	int i = 0;
	for(BlockIndex adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacent(index))
	{
		m_enterable[(index * maxAdjacent) + i] = adjacent == BLOCK_INDEX_MAX ?
			false :
			blocks.shape_moveTypeCanEnterFrom(adjacent, m_moveType, index);
		++i;
	}
}
std::vector<BlockIndex> TerrainFacade::findPath(BlockIndex current, const DestinationCondition destinationCondition, const AccessCondition accessCondition, OpenListPush openListPush, OpenListPop openListPop, OpenListEmpty openListEmpty) const
{
	Blocks& blocks = m_area.getBlocks();
	std::unordered_map<BlockIndex, BlockIndex> closed;
	closed[current] = BLOCK_INDEX_MAX;
	openListPush(current);
	while(!openListEmpty())
	{
		current = openListPop();
		for(uint8_t adjacentCount = 0; adjacentCount < maxAdjacent; ++adjacentCount)
			if(canEnterFrom(current, adjacentCount))
			{
				BlockIndex adjacentIndex = blocks.offsetForAdjacentCount(current, adjacentCount);
				assert(adjacentIndex != BLOCK_INDEX_MAX);
				Facing facing = blocks.facingToSetWhenEnteringFrom(adjacentIndex, current);
				if(closed.contains(adjacentIndex) || !accessCondition(adjacentIndex, facing))
					continue;
				if(destinationCondition(adjacentIndex, facing))
				{
					std::vector<BlockIndex> output;
					output.push_back(adjacentIndex);
					while(current != BLOCK_INDEX_MAX)
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
bool TerrainFacade::canEnterFrom(BlockIndex blockIndex, uint8_t adjacentCount) const
{
	return m_enterable[(blockIndex * maxAdjacent) + adjacentCount];
}
bool TerrainFacade::shapeCanFitWithAnyFacing(BlockIndex blockIndex, const Shape& shape) const
{
	for(Facing facing = 0; facing < FACING_MAX; ++ facing)
		if(shapeCanFitWithFacing(blockIndex, shape, facing))
			return true;
	return false;
}
bool TerrainFacade::shapeCanFitWithFacing(BlockIndex blockIndex, const Shape& shape, Facing facing) const
{
	Blocks& blocks = m_area.getBlocks();
	for(auto[x, y, z, v] : shape.positionsWithFacing(facing))
	{
		BlockIndex wouldBeOccupiedIndex = blocks.offset(blockIndex, x, y, z);
		if(!m_enterable[wouldBeOccupiedIndex])
			return false;
	}
	return true;
}
std::vector<BlockIndex> TerrainFacade::findPathToForSingleBlockShape(BlockIndex start, BlockIndex target) const
{
	DestinationCondition destinationCondition = [target](BlockIndex index, Facing){ return index == target; };
	AccessCondition accessCondition = [](BlockIndex, Facing){ return true; };
	return findPathDepthFirst(start, destinationCondition, accessCondition, target);
}
std::vector<BlockIndex> TerrainFacade::findPathToForMultiBlockShape(BlockIndex start, const Shape& shape, BlockIndex target) const
{
	Blocks& blocks = m_area.getBlocks();
	DestinationCondition destinationCondition = [&shape, &blocks, target](BlockIndex index, Facing facing)
	{ 
		auto occupied = shape.getBlocksOccupiedAt(blocks, index, facing);
		return std::ranges::find(occupied, target) != occupied.end();
	};
	AccessCondition accessCondition = [this, &shape](BlockIndex index, Facing){
		return shapeCanFitWithAnyFacing(index, shape);
	};
	return findPathDepthFirst(start, destinationCondition, accessCondition, target);
}
std::vector<BlockIndex> TerrainFacade::findPathToAnyOfForSingleBlockShape(BlockIndex start, std::vector<BlockIndex> indecies, BlockIndex huristicDestination) const
{
	DestinationCondition destinationCondition = [indecies](BlockIndex index, Facing){ return std::ranges::find(indecies, index) != indecies.end(); };
	AccessCondition accessCondition = [](BlockIndex, Facing){ return true; };
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreathFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathToAnyOfForMultiBlockShape(BlockIndex start, const Shape& shape, std::vector<BlockIndex> indecies, BlockIndex huristicDestination) const
{
	DestinationCondition destinationCondition = [indecies](BlockIndex index, Facing){ return std::ranges::find(indecies, index) != indecies.end(); };
	AccessCondition accessCondition = [this, &shape]([[maybe_unused]] BlockIndex index, Facing){
		return shapeCanFitWithAnyFacing(index, shape);
	};
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreathFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathToConditionForSingleBlockShape(BlockIndex start, const DestinationCondition destinationCondition, BlockIndex huristicDestination) const
{
	AccessCondition accessCondition = [](BlockIndex, Facing){ return true; };
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreathFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathToConditionForMultiBlockShape(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristicDestination) const
{
	AccessCondition accessCondition = [this, &shape](BlockIndex index, Facing){
		return shapeCanFitWithAnyFacing(index, shape);
	};
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreathFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathTo(BlockIndex start, const Shape& shape, BlockIndex target) const
{
	if(shape.isMultiTile)
		return findPathToForMultiBlockShape(start, shape, target);
	else
		return findPathToForSingleBlockShape(start, target);
}
std::vector<BlockIndex> TerrainFacade::findPathToAnyOf(BlockIndex start, const Shape& shape, std::vector<BlockIndex> indecies, BlockIndex huristicDestination) const
{
	if(shape.isMultiTile)
		return findPathToAnyOfForMultiBlockShape(start, shape, indecies, huristicDestination);
	else
		return findPathToAnyOfForSingleBlockShape(start, indecies, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathToCondition(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristicDestination) const
{
	if(shape.isMultiTile)
		return findPathToConditionForMultiBlockShape(start, shape, destinationCondition, huristicDestination);
	else
		return findPathToConditionForSingleBlockShape(start, destinationCondition, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathAdjacentTo(BlockIndex start, const Shape& shape, BlockIndex target) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(blocks.getAdjacentWithEdgeAndCornerAdjacent(target), targets.begin(), 
		[&](BlockIndex block){ return blocks.shape_moveTypeCanEnter(block, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, shape, targets, target);
}
std::vector<BlockIndex> TerrainFacade::findPathAdjacentTo(BlockIndex start, const Shape& shape, const HasShape& hasShape) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(hasShape.getAdjacentBlocks(), targets.begin(), 
		[&](BlockIndex block){ return blocks.shape_moveTypeCanEnter(block, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, shape, targets, hasShape.m_location);
}
std::vector<BlockIndex> TerrainFacade::findPathAdjacentToCondition(BlockIndex start, const Shape& shape, DestinationCondition condition, BlockIndex huristicDestination) const
{
	Blocks& blocks = m_area.getBlocks();
	DestinationCondition destinationCondition = [blocks, shape, condition](BlockIndex index, Facing facing)
	{
		for(BlockIndex adjacent : shape.getBlocksWhichWouldBeAdjacentAt(blocks, index, facing))
			if(condition(adjacent, 0))
				return true;
		return false;
	};
	return findPathToCondition(start, shape, condition, huristicDestination);
}
std::vector<BlockIndex> TerrainFacade::findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, BlockIndex target, const Faction& faction) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(blocks.getAdjacentWithEdgeAndCornerAdjacent(target), targets.begin(), 
		[&](BlockIndex block){ return !blocks.isReserved(block, faction) && blocks.shape_moveTypeCanEnter(block, m_moveType); 
	});
	if(targets.empty())
		return { };
	// TODO: optimization: If the shape is single tile and the target is enterable ever then path to target directly and discard the final block.
	return findPathToAnyOf(start, shape, targets, target);
}
std::vector<BlockIndex> TerrainFacade::findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, const HasShape& hasShape, const Faction& faction) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(hasShape.getAdjacentBlocks(), targets.begin(), 
		[&](BlockIndex block){ return !blocks.isReserved(block, faction) && blocks.shape_moveTypeCanEnter(block, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, shape, targets, hasShape.m_location);
}
void AreaHasTerrainFacades::update(BlockIndex index)
{
	for(auto& pair : m_data)
		pair.second.update(index);
}
TerrainFacade& AreaHasTerrainFacades::at(const MoveType& moveType)
{
	auto found = m_data.find(&moveType);
	if(found != m_data.end())
		return found->second;
	else
	{
		auto [iter, success] = m_data.try_emplace(&moveType, m_area, moveType);
		assert(success);
		return iter->second;
	}
}
