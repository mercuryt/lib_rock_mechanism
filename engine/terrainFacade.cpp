// TODO: code is repeated 3 times each for access condition and destination condition definition.
#include "terrainFacade.h"
#include "area.h"
#include "findsPath.h"
#include "shape.h"
#include "simulation.h"
#include "types.h"
#include "util.h"
#include "pathRequest.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <iterator>

TerrainFacade::TerrainFacade(Area& area, const MoveType& moveType) : m_area(area), m_moveType(moveType)
{
	m_enterable.resize(m_area.getBlocks().size() * maxAdjacent);
	for(BlockIndex block : m_area.getBlocks().getAll())
		update(block);
}
void TerrainFacade::doStep()
{
	auto taskNoHuristic = [&](PathRequestIndex index){ findPathForIndexNoHuristic(index); };
	m_area.m_simulation.parallelizeTaskIndices<class Data>(m_pathRequestAccessConditionsNoHuristic.size(), Config::pathRequestsPerThread, taskNoHuristic);
	auto taskWithHuristic = [&](PathRequestIndex index){ findPathForIndexWithHuristic(index); };
	m_area.m_simulation.parallelizeTaskIndices<class Data>(m_pathRequestAccessConditionsWithHuristic.size(), Config::pathRequestsPerThread, taskWithHuristic);
	for(uint i = 0; i < m_pathRequestAccessConditionsNoHuristic.size(); ++i)
	{
		PathRequest& pathRequest = *m_pathRequestNoHuristic.at(i);
		pathRequest.callback(m_area, m_pathRequestResultsNoHuristic.at(i));
		pathRequest.reset();
	}
	clearPathRequests();
}
void TerrainFacade::findPathForIndexWithHuristic(PathRequestIndex index)
{
	m_pathRequestResultsWithHuristic.at(index) = findPathDepthFirst(m_pathRequestStartPositionWithHuristic.at(index), m_pathRequestDestinationConditionsWithHuristic.at(index), m_pathRequestAccessConditionsWithHuristic.at(index), m_pathRequestHuristic.at(index));
}
void TerrainFacade::findPathForIndexNoHuristic(PathRequestIndex index)
{
	m_pathRequestResultsNoHuristic.at(index) = findPathBreadthFirst(m_pathRequestStartPositionNoHuristic.at(index), m_pathRequestDestinationConditionsNoHuristic.at(index), m_pathRequestAccessConditionsNoHuristic.at(index));
}
void TerrainFacade::clearPathRequests()
{
	m_pathRequestStartPositionNoHuristic.clear();
	m_pathRequestAccessConditionsNoHuristic.clear();
	m_pathRequestDestinationConditionsNoHuristic.clear();
	m_pathRequestResultsNoHuristic.clear();
	m_pathRequestNoHuristic.clear();

	m_pathRequestStartPositionWithHuristic.clear();
	m_pathRequestAccessConditionsWithHuristic.clear();
	m_pathRequestDestinationConditionsWithHuristic.clear();
	m_pathRequestHuristic.clear();
	m_pathRequestResultsWithHuristic.clear();
	m_pathRequestWithHuristic.clear();
}
bool TerrainFacade::getValueForBit(BlockIndex from, BlockIndex to) const
{
	Blocks& blocks = m_area.getBlocks();
	return to != BLOCK_INDEX_MAX &&
		blocks.shape_anythingCanEnterEver(to) &&
		blocks.shape_moveTypeCanEnter(to, m_moveType) &&
		blocks.shape_moveTypeCanEnterFrom(to, m_moveType, from);
}
PathRequestIndex TerrainFacade::getPathRequestIndexNoHuristic()
{
	PathRequestIndex index = m_pathRequestAccessConditionsNoHuristic.size();
	m_pathRequestStartPositionNoHuristic.resize(index + 1);
	m_pathRequestAccessConditionsNoHuristic.resize(index + 1);
	m_pathRequestDestinationConditionsNoHuristic.resize(index + 1);
	m_pathRequestResultsNoHuristic.resize(index + 1);
	m_pathRequestNoHuristic.resize(index + 1);
	return index;
}
PathRequestIndex TerrainFacade::getPathRequestIndexWithHuristic()
{
	PathRequestIndex index = m_pathRequestAccessConditionsWithHuristic.size();
	m_pathRequestStartPositionWithHuristic.resize(index + 1);
	m_pathRequestAccessConditionsWithHuristic.resize(index + 1);
	m_pathRequestDestinationConditionsWithHuristic.resize(index + 1);
	m_pathRequestHuristic.resize(index + 1);
	m_pathRequestResultsWithHuristic.resize(index + 1);
	m_pathRequestWithHuristic.resize(index + 1);
	return index;
}
PathRequestIndex TerrainFacade::registerPathRequestNoHuristic(BlockIndex start, AccessCondition access, DestinationCondition destination, PathRequest& pathRequest)
{
	PathRequestIndex index = getPathRequestIndexNoHuristic();
	m_pathRequestStartPositionNoHuristic.at(index) = start;
	m_pathRequestAccessConditionsNoHuristic.at(index) = access;
	m_pathRequestDestinationConditionsNoHuristic.at(index) = destination;
	m_pathRequestNoHuristic.at(index) = &pathRequest;
	return index;
}
PathRequestIndex TerrainFacade::registerPathRequestWithHuristic(BlockIndex start, AccessCondition access, DestinationCondition destination, BlockIndex huristic, PathRequest& pathRequest)
{
	PathRequestIndex index = getPathRequestIndexWithHuristic();
	m_pathRequestStartPositionWithHuristic.at(index) = start;
	m_pathRequestAccessConditionsWithHuristic.at(index) = access;
	m_pathRequestDestinationConditionsWithHuristic.at(index) = destination;
	m_pathRequestHuristic.at(index) = huristic;
	m_pathRequestWithHuristic.at(index) = &pathRequest;
	return index;
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
void TerrainFacade::resizePathRequestWithHuristic(PathRequestIndex size)
{

	m_pathRequestStartPositionWithHuristic.resize(size);
	m_pathRequestAccessConditionsWithHuristic.resize(size);
	m_pathRequestDestinationConditionsWithHuristic.resize(size);
	m_pathRequestHuristic.resize(size);
	m_pathRequestWithHuristic.resize(size);
	m_pathRequestResultsWithHuristic.resize(size);
}
void TerrainFacade::resizePathRequestNoHuristic(PathRequestIndex size)
{

	m_pathRequestStartPositionNoHuristic.resize(size);
	m_pathRequestAccessConditionsNoHuristic.resize(size);
	m_pathRequestDestinationConditionsNoHuristic.resize(size);
	m_pathRequestNoHuristic.resize(size);
	m_pathRequestResultsNoHuristic.resize(size);
}
void TerrainFacade::movePathRequestNoHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex)
{
	assert(oldIndex != newIndex);
	std::iter_swap(m_pathRequestAccessConditionsNoHuristic.begin() + oldIndex, m_pathRequestAccessConditionsNoHuristic.begin() + newIndex);
	std::iter_swap(m_pathRequestDestinationConditionsNoHuristic.begin() + oldIndex, m_pathRequestDestinationConditionsNoHuristic.begin() + newIndex);
	std::iter_swap(m_pathRequestStartPositionNoHuristic.begin() + oldIndex, m_pathRequestStartPositionNoHuristic.begin() + newIndex);
	std::iter_swap(m_pathRequestNoHuristic.begin() + oldIndex, m_pathRequestNoHuristic.begin() + newIndex);
	std::iter_swap(m_pathRequestResultsNoHuristic.begin() + oldIndex, m_pathRequestResultsNoHuristic.begin() + newIndex);
	m_pathRequestNoHuristic.at(newIndex)->update(newIndex);
}
void TerrainFacade::movePathRequestWithHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex)
{
	assert(oldIndex != newIndex);
	std::iter_swap(m_pathRequestAccessConditionsWithHuristic.begin() + oldIndex, m_pathRequestAccessConditionsWithHuristic.begin() + newIndex);
	std::iter_swap(m_pathRequestDestinationConditionsWithHuristic.begin() + oldIndex, m_pathRequestDestinationConditionsWithHuristic.begin() + newIndex);
	std::iter_swap(m_pathRequestStartPositionWithHuristic.begin() + oldIndex, m_pathRequestStartPositionWithHuristic.begin() + newIndex);
	std::iter_swap(m_pathRequestWithHuristic.begin() + oldIndex, m_pathRequestWithHuristic.begin() + newIndex);
	std::iter_swap(m_pathRequestResultsWithHuristic.begin() + oldIndex, m_pathRequestResultsWithHuristic.begin() + newIndex);
	std::iter_swap(m_pathRequestHuristic.begin() + oldIndex, m_pathRequestHuristic.begin() + newIndex);
	m_pathRequestWithHuristic.at(newIndex)->update(newIndex);
}
void TerrainFacade::unregisterNoHuristic(PathRequestIndex index)
{
	assert(m_pathRequestAccessConditionsNoHuristic.size() > index);
	if(m_pathRequestAccessConditionsNoHuristic.size() == 1)
		resizePathRequestNoHuristic(0);
	else
	{
		// Swap the index with the last one.
		PathRequestIndex lastIndex = m_pathRequestAccessConditionsNoHuristic.size() - 1;
		movePathRequestNoHuristic(lastIndex, index);
		// Shrink all vectors by one.
		resizePathRequestNoHuristic(lastIndex);
	}
}
void TerrainFacade::unregisterWithHuristic(PathRequestIndex index)
{
	assert(m_pathRequestAccessConditionsWithHuristic.size() > index);
	if(m_pathRequestAccessConditionsWithHuristic.size() == 1)
		resizePathRequestWithHuristic(0);
	else
	{
		// Swap the index with the last one.
		PathRequestIndex lastIndex = m_pathRequestAccessConditionsWithHuristic.size() - 1;
		movePathRequestWithHuristic(lastIndex, index);
		// Shrink all vectors by one.
		resizePathRequestWithHuristic(lastIndex);
	}
}
FindPathResult TerrainFacade::findPath(BlockIndex start, const DestinationCondition destinationCondition, const AccessCondition accessCondition, OpenListPush openListPush, OpenListPop openListPop, OpenListEmpty openListEmpty) const
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
					return {output, adjacentIndex, true};
				}
				closed[adjacentIndex] = current;
				openListPush(adjacentIndex);
			}
	}
	return {{}, BLOCK_INDEX_MAX, false};
}
FindPathResult TerrainFacade::findPathBreadthFirst(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition) const
{
	std::deque<BlockIndex> openList;
	OpenListEmpty empty = [&openList](){ return openList.empty(); };
	OpenListPush push  = [&openList](BlockIndex index) { openList.push_back(index); };
	OpenListPop pop = [&openList](){ BlockIndex output = openList.front(); openList.pop_front(); return output; };
	return findPath(from, destinationCondition, accessCondition, push, pop, empty);
}
FindPathResult TerrainFacade::findPathDepthFirst(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, BlockIndex huristicDestination) const
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
FindPathResult TerrainFacade::findPathToForSingleBlockShape(BlockIndex start, const Shape& shape, BlockIndex target, bool detour) const
{
	DestinationCondition destinationCondition = [target](BlockIndex index, Facing){ return index == target; };
	AccessCondition accessCondition = makeAccessCondition(shape, start, {start}, detour, BLOCK_DISTANCE_MAX);
	return findPathDepthFirst(start, destinationCondition, accessCondition, target);
}
FindPathResult TerrainFacade::findPathToForMultiBlockShape(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	DestinationCondition destinationCondition = [&shape, &blocks, target](BlockIndex index, Facing facing)
	{ 
		auto occupied = shape.getBlocksOccupiedAt(blocks, index, facing);
		return std::ranges::find(occupied, target) != occupied.end();
	};
	std::vector<BlockIndex> occupied = shape.getBlocksOccupiedAt(blocks, start, startFacing);
	AccessCondition accessCondition = makeAccessCondition(shape, start, occupied, detour, BLOCK_DISTANCE_MAX);
	return findPathDepthFirst(start, destinationCondition, accessCondition, target);
}
FindPathResult TerrainFacade::findPathToAnyOfForSingleBlockShape(BlockIndex start, const Shape& shape, std::vector<BlockIndex> indecies, BlockIndex huristicDestination, bool detour) const
{
	DestinationCondition destinationCondition = [indecies](BlockIndex index, Facing){ return std::ranges::find(indecies, index) != indecies.end(); };
	AccessCondition accessCondition = makeAccessCondition(shape, start, {start}, detour, BLOCK_INDEX_MAX);
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreadthFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathToAnyOfForMultiBlockShape(BlockIndex start, const Shape& shape, Facing startFacing, std::vector<BlockIndex> indecies, BlockIndex huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	auto occupied = shape.getBlocksOccupiedAt(blocks, start, startFacing);
	// TODO: should this have a maxRange?
	AccessCondition accessCondition = makeAccessCondition(shape, start, occupied, detour, BLOCK_DISTANCE_MAX);
	DestinationCondition destinationCondition = [indecies](BlockIndex index, Facing){ return std::ranges::find(indecies, index) != indecies.end(); };
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreadthFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathToConditionForSingleBlockShape(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristicDestination, bool detour) const
{
	//TODO: should have max range.
	AccessCondition accessCondition = makeAccessCondition(shape, start, {start}, detour, BLOCK_DISTANCE_MAX);
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreadthFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathToConditionForMultiBlockShape(BlockIndex start, const Shape& shape, Facing startFacing, const DestinationCondition destinationCondition, BlockIndex huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	auto occupied = shape.getBlocksOccupiedAt(blocks, start, startFacing);
	AccessCondition accessCondition = makeAccessCondition(shape, start, occupied, detour, BLOCK_DISTANCE_MAX);
	if(huristicDestination == BLOCK_INDEX_MAX)
		return findPathBreadthFirst(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirst(start, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathTo(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, bool detour) const
{
	if(shape.isMultiTile)
		return findPathToForMultiBlockShape(start, shape, startFacing, target, detour);
	else
		return findPathToForSingleBlockShape(start, shape, target, detour);
}
FindPathResult TerrainFacade::findPathToAnyOf(BlockIndex start, const Shape& shape, Facing startFacing, std::vector<BlockIndex> indecies, BlockIndex huristicDestination, bool detour) const
{
	if(shape.isMultiTile)
		return findPathToAnyOfForMultiBlockShape(start, shape, startFacing, indecies, huristicDestination, detour);
	else
		return findPathToAnyOfForSingleBlockShape(start, shape, indecies, huristicDestination, detour);
}
FindPathResult TerrainFacade::findPathToCondition(BlockIndex start, const Shape& shape, Facing startFacing, const DestinationCondition destinationCondition, BlockIndex huristicDestination, bool detour) const
{
	if(shape.isMultiTile)
		return findPathToConditionForMultiBlockShape(start, shape, startFacing, destinationCondition, huristicDestination, detour);
	else
		return findPathToConditionForSingleBlockShape(start, shape, destinationCondition, huristicDestination, detour);
}
FindPathResult TerrainFacade::findPathAdjacentTo(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, bool detour) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	auto source = blocks.getAdjacentWithEdgeAndCornerAdjacent(target);
	std::ranges::copy_if(source, std::back_inserter(targets),
		[&](const BlockIndex block) { 
			return blocks.shape_anythingCanEnterEver(block) && blocks.shape_moveTypeCanEnter(block, m_moveType) && blocks.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(block, shape, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, shape, startFacing, targets, target, detour);
}
//TODO: this could dispatch to specific actor / item variants rather then checking actor or item twice.
FindPathResult TerrainFacade::findPathAdjacentToPolymorphic(BlockIndex start, const Shape& shape, Facing startFacing, ActorOrItemIndex actorOrItem, bool detour) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	auto source = actorOrItem.getAdjacentBlocks(m_area);
	std::ranges::copy_if(source, std::back_inserter(targets),
		[&](const BlockIndex block) { 
			return blocks.shape_anythingCanEnterEver(block) && blocks.shape_moveTypeCanEnter(block, m_moveType) && blocks.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(block, shape, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, shape, startFacing, targets, actorOrItem.getLocation(m_area), detour);
}
FindPathResult TerrainFacade::findPathAdjacentToCondition(BlockIndex start, const Shape& shape, Facing startFacing, DestinationCondition condition, BlockIndex huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	DestinationCondition destinationCondition = [blocks, shape, condition](BlockIndex index, Facing facing)
	{
		for(BlockIndex adjacent : shape.getBlocksWhichWouldBeAdjacentAt(blocks, index, facing))
			if(condition(adjacent, 0))
				return true;
		return false;
	};
	return findPathToCondition(start, shape, startFacing, condition, huristicDestination, detour);
}
FindPathResult TerrainFacade::findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, const Faction& faction, bool detour) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	auto source = blocks.getAdjacentWithEdgeAndCornerAdjacent(target);
	std::ranges::copy_if(source, std::back_inserter(targets), 
		[&](BlockIndex block){
	       		return blocks.shape_anythingCanEnterEver(block) && !blocks.isReserved(block, faction) && 
			blocks.shape_moveTypeCanEnter(block, m_moveType) && blocks.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(block, shape, m_moveType); 
	});
	if(targets.empty())
		return { };
	// TODO: optimization: If the shape is single tile and the target is enterable ever then path to target directly and discard the final block.
	return findPathToAnyOf(start, shape, startFacing, targets, target, detour);
}
FindPathResult TerrainFacade::findPathAdjacentToAndUnreservedPolymorphic(BlockIndex start, const Shape& shape, Facing startFacing, ActorOrItemIndex actorOrItem, const Faction& faction, bool detour) const
{
	std::vector<BlockIndex> targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(actorOrItem.getAdjacentBlocks(m_area), targets.begin(), 
		[&](BlockIndex block){ return !blocks.isReserved(block, faction) && blocks.shape_moveTypeCanEnter(block, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, shape, startFacing, targets, actorOrItem.getLocation(m_area), detour);
}
AccessCondition TerrainFacade::makeAccessConditionForActor(ActorIndex actor, bool detour, DistanceInBlocks maxRange) const
{
	auto& blocks = m_area.m_actors.getBlocks(actor);
	std::vector<BlockIndex> currentlyOccupied(blocks.begin(), blocks.end());
	return makeAccessCondition(m_area.m_actors.getShape(actor), m_area.m_actors.getLocation(actor), currentlyOccupied, detour, maxRange);
}
AccessCondition TerrainFacade::makeAccessCondition(const Shape& shape, BlockIndex start, std::vector<BlockIndex> initalBlocks, bool detour, DistanceInBlocks maxRange) const
{
	Blocks& blocks = m_area.getBlocks();
	if(shape.isMultiTile)
	{
		if(detour)
		{
			return [this, initalBlocks, &shape, &blocks, maxRange, start](BlockIndex index, Facing facing) -> bool { 
				return 
					blocks.taxiDistance(start, index) <= maxRange &&
					m_area.getBlocks().shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(index, shape, m_moveType, facing, initalBlocks);
			};
		}
		else
			return [this, &shape, &blocks, maxRange, start]([[maybe_unused]] BlockIndex index, Facing facing){
				return 
					blocks.taxiDistance(start, index) <= maxRange &&
					blocks.shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, m_moveType, facing);
			};
	}
	else
	{
		if(detour)
		{
			CollisionVolume volume = shape.getCollisionVolumeAtLocationBlock();
			assert(initalBlocks.size() == 1);
			BlockIndex initalBlock = initalBlocks.front();
			return [volume, &blocks, initalBlock, maxRange, start](BlockIndex index, Facing)
			{
				return 
					blocks.taxiDistance(start, index) <= maxRange &&
					(index == initalBlock || blocks.shape_getDynamicVolume(index) + volume <= Config::maxBlockVolume);
			};
		}
		else
			return [blocks, maxRange, start](BlockIndex index, Facing) { return blocks.taxiDistance(start, index) <= maxRange; };
	}
}
void AreaHasTerrainFacades::doStep()
{
	for(auto& pair : m_data)
		pair.second.doStep();
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
