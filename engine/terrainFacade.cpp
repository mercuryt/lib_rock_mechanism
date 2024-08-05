// TODO: code is repeated 3 times each for access condition and destination condition definition.
#include "terrainFacade.h"
#include "actors/actors.h"
#include "area.h"
#include "config.h"
#include "index.h"
#include "onePassVector.h"
#include "pathMemo.h"
#include "shape.h"
#include "simulation.h"
#include "types.h"
#include "util.h"
#include "pathRequest.h"
#include "blocks/blocks.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <iterator>

TerrainFacade::TerrainFacade(Area& area, const MoveType& moveType) : m_area(area), m_moveType(moveType)
{
	m_enterable.resize(m_area.getBlocks().size() * maxAdjacent);
	for(BlockIndex block : m_area.getBlocks().getAll())
		update(block);
	// Two results fit on a  cache line, by ensuring that the number of tasks per thread is a multiple of 2 we prevent false shareing.
	assert(Config::pathRequestsPerThread % 2 == 0);
}
void TerrainFacade::doStep()
{
	// Read step.
	// Breadth first.
	uint numberOfRequests = m_pathRequestStartPositionNoHuristic.size();
	uint batches = std::ceil((float)numberOfRequests / (float)Config::pathRequestsPerThread);
	auto& memosBreadth = m_area.m_simulation.m_hasPathMemos.getBreadthFirstWithMinimumSize(batches, m_area);
	uint i = 0;
	uint memoIndex = 0;
	while(i < numberOfRequests)
	{
		auto rangeEnd = std::min(numberOfRequests, i + Config::pathRequestsPerThread);
		#pragma omp parallel
			for(; i < rangeEnd; ++i)
				findPathForIndexNoHuristic(PathRequestIndex::create(i), memosBreadth.at(memoIndex));
		++memoIndex;
	}
	// Depth First.
	numberOfRequests = m_pathRequestStartPositionWithHuristic.size();
	batches = std::ceil((float)numberOfRequests / (float)Config::pathRequestsPerThread);
	auto& memosDepth = m_area.m_simulation.m_hasPathMemos.getDepthFirstWithMinimumSize(batches, m_area);
	i = 0;
	memoIndex = 0;
	while(i < numberOfRequests)
	{
		auto rangeEnd = std::min(numberOfRequests, i + Config::pathRequestsPerThread);
		#pragma omp parallel
			for(; i < rangeEnd; ++i)
				findPathForIndexWithHuristic(PathRequestIndex::create(i), memosDepth.at(memoIndex));
		++memoIndex;
	}
	// Write step.
	// Read First.
	for(PathRequestIndex i = PathRequestIndex::create(0); i < m_pathRequestAccessConditionsNoHuristic.size(); ++i)
	{
		PathRequest& pathRequest = *m_pathRequestNoHuristic.at(i);
		pathRequest.callback(m_area, m_pathRequestResultsNoHuristic.at(i));
		pathRequest.reset();
	}
	// Depth First.
	for(PathRequestIndex i = PathRequestIndex::create(0); i < m_pathRequestAccessConditionsWithHuristic.size(); ++i)
	{
		PathRequest& pathRequest = *m_pathRequestWithHuristic.at(i);
		pathRequest.callback(m_area, m_pathRequestResultsWithHuristic.at(i));
		pathRequest.reset();
	}
	clearPathRequests();
}
void TerrainFacade::findPathForIndexWithHuristic(PathRequestIndex index, PathMemoDepthFirst& memo)
{
	m_pathRequestResultsWithHuristic.at(index) = findPathDepthFirst(m_pathRequestStartPositionWithHuristic.at(index), m_pathRequestDestinationConditionsWithHuristic.at(index), m_pathRequestAccessConditionsWithHuristic.at(index), m_pathRequestHuristic.at(index), memo);
	memo.reset();
}
void TerrainFacade::findPathForIndexNoHuristic(PathRequestIndex index, PathMemoBreadthFirst& memo)
{
	m_pathRequestResultsNoHuristic.at(index) = findPathBreadthFirst(m_pathRequestStartPositionNoHuristic.at(index), m_pathRequestDestinationConditionsNoHuristic.at(index), m_pathRequestAccessConditionsNoHuristic.at(index), memo);
	memo.reset();
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
	return to.exists() &&
		blocks.shape_anythingCanEnterEver(to) &&
		blocks.shape_moveTypeCanEnter(to, m_moveType) &&
		blocks.shape_moveTypeCanEnterFrom(to, m_moveType, from);
}
PathRequestIndex TerrainFacade::getPathRequestIndexNoHuristic()
{
	PathRequestIndex index = PathRequestIndex::create(m_pathRequestAccessConditionsNoHuristic.size());
	size_t newSize = m_pathRequestAccessConditionsNoHuristic.size() + 1;
	m_pathRequestStartPositionNoHuristic.resize(newSize);
	m_pathRequestAccessConditionsNoHuristic.resize(newSize);
	m_pathRequestDestinationConditionsNoHuristic.resize(newSize);
	m_pathRequestResultsNoHuristic.resize(newSize);
	m_pathRequestNoHuristic.resize(newSize);
	return index;
}
PathRequestIndex TerrainFacade::getPathRequestIndexWithHuristic()
{
	PathRequestIndex index = PathRequestIndex::create(m_pathRequestAccessConditionsWithHuristic.size());
	size_t newSize = m_pathRequestAccessConditionsWithHuristic.size() + 1;
	m_pathRequestStartPositionWithHuristic.resize(newSize);
	m_pathRequestAccessConditionsWithHuristic.resize(newSize);
	m_pathRequestDestinationConditionsWithHuristic.resize(newSize);
	m_pathRequestHuristic.resize(newSize);
	m_pathRequestResultsWithHuristic.resize(newSize);
	m_pathRequestWithHuristic.resize(newSize);
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
		m_enterable[(index.get() * maxAdjacent) + i] = getValueForBit(index, adjacent);
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
	m_pathRequestAccessConditionsNoHuristic.at(newIndex) = m_pathRequestAccessConditionsNoHuristic.at(oldIndex);
	m_pathRequestDestinationConditionsNoHuristic.at(newIndex) = m_pathRequestDestinationConditionsNoHuristic.at(oldIndex);
	m_pathRequestStartPositionNoHuristic.at(newIndex) = m_pathRequestStartPositionNoHuristic.at(oldIndex);
	m_pathRequestNoHuristic.at(newIndex) = m_pathRequestNoHuristic.at(oldIndex);
	m_pathRequestResultsNoHuristic.at(newIndex) = m_pathRequestResultsNoHuristic.at(oldIndex);
	m_pathRequestNoHuristic.at(newIndex)->update(newIndex);
}
void TerrainFacade::movePathRequestWithHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex)
{
	assert(oldIndex != newIndex);
	m_pathRequestAccessConditionsWithHuristic.at(newIndex) = m_pathRequestAccessConditionsWithHuristic.at(oldIndex);
	m_pathRequestDestinationConditionsWithHuristic.at(newIndex) = m_pathRequestDestinationConditionsWithHuristic.at(oldIndex);
	m_pathRequestStartPositionWithHuristic.at(newIndex) = m_pathRequestStartPositionWithHuristic.at(oldIndex);
	m_pathRequestWithHuristic.at(newIndex) = m_pathRequestWithHuristic.at(oldIndex);
	m_pathRequestResultsWithHuristic.at(newIndex) = m_pathRequestResultsWithHuristic.at(oldIndex);
	m_pathRequestHuristic.at(newIndex) = m_pathRequestHuristic.at(oldIndex);
	m_pathRequestWithHuristic.at(newIndex)->update(newIndex);
}
void TerrainFacade::unregisterNoHuristic(PathRequestIndex index)
{
	assert(m_pathRequestAccessConditionsNoHuristic.size() > index);
	if(m_pathRequestAccessConditionsNoHuristic.size() == 1)
		resizePathRequestNoHuristic(PathRequestIndex::create(0));
	else
	{
		// Swap the index with the last one.
		PathRequestIndex lastIndex = PathRequestIndex::create(m_pathRequestAccessConditionsNoHuristic.size() - 1);
		movePathRequestNoHuristic(lastIndex, index);
		// Shrink all vectors by one.
		resizePathRequestNoHuristic(lastIndex);
	}
}
void TerrainFacade::unregisterWithHuristic(PathRequestIndex index)
{
	assert(m_pathRequestAccessConditionsWithHuristic.size() > index);
	if(m_pathRequestAccessConditionsWithHuristic.size() == 1)
		resizePathRequestWithHuristic(PathRequestIndex::create(0));
	else
	{
		// Swap the index with the last one.
		PathRequestIndex lastIndex = PathRequestIndex::create(m_pathRequestAccessConditionsWithHuristic.size() - 1);
		movePathRequestWithHuristic(lastIndex, index);
		// Shrink all vectors by one.
		resizePathRequestWithHuristic(lastIndex);
	}
}
FindPathResult TerrainFacade::findPath(BlockIndex start, const DestinationCondition destinationCondition, const AccessCondition accessCondition, OpenListPush openListPush, OpenListPop openListPop, OpenListEmpty openListEmpty) const
{
	Blocks& blocks = m_area.getBlocks();
	BlockIndexMap<BlockIndex> closed;
	closed[start] = BlockIndex::null();
	openListPush(start);
	while(!openListEmpty())
	{
		BlockIndex current = openListPop();
		for(AdjacentIndex adjacentCount = AdjacentIndex::create(0); adjacentCount < maxAdjacent; ++adjacentCount)
			if(canEnterFrom(current, adjacentCount))
			{
				BlockIndex adjacentIndex = blocks.indexAdjacentToAtCount(current, adjacentCount);
				assert(adjacentIndex.exists());
				if(closed.contains(adjacentIndex))
					continue;
				//TODO: Make variant for radially semetric shapes which ignores facing.
				Facing facing = blocks.facingToSetWhenEnteringFrom(adjacentIndex, current);
				if(!accessCondition(adjacentIndex, facing))
				{
					closed[adjacentIndex] = BlockIndex::null();
					continue;
				}
				if(destinationCondition(adjacentIndex, facing))
				{
					BlockIndices output;
					output.add(adjacentIndex);
					while(current != start)
					{
						output.add(current);
						current = closed[current];
					}
					std::reverse(output.begin(), output.end());
					return {output, adjacentIndex, true};
				}
				closed[adjacentIndex] = current;
				openListPush(adjacentIndex);
			}
	}
	return {{}, BlockIndex::null(), false};
}
FindPathResult TerrainFacade::findPathBreadthFirst(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, PathMemoBreadthFirst& memo) const
{
	OpenListEmpty empty = [&memo](){ return memo.openEmpty(); };
	OpenListPush push  = [&memo](BlockIndex index) { memo.setOpen(index); };
	OpenListPop pop = [&memo](){ return memo.next(); };
	return findPath(from, destinationCondition, accessCondition, push, pop, empty);
}
FindPathResult TerrainFacade::findPathDepthFirst(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, BlockIndex huristicDestination, PathMemoDepthFirst& memo) const
{
	memo.setup(m_area, huristicDestination);
	OpenListEmpty empty = [&memo](){ return memo.openEmpty(); };
	OpenListPush push  = [ &memo, huristicDestination](BlockIndex index) { memo.setOpen(index); };
	OpenListPop pop = [&memo](){ return memo.next(); };
	return findPath(from, destinationCondition, accessCondition, push, pop, empty);
}
FindPathResult TerrainFacade::findPathBreadthFirstWithoutMemo(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition) const
{
	auto& hasMemos = m_area.m_simulation.m_hasPathMemos;
	auto pair = hasMemos.getBreadthFirstSingle(m_area);
	auto& memo = pair.first;
	auto index = pair.second;
	auto output = findPathBreadthFirst(from, destinationCondition, accessCondition, memo);
	hasMemos.releaseBreadthFirstSingle(index);
	return output;
}
FindPathResult TerrainFacade::findPathDepthFirstWithoutMemo(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, BlockIndex huristicDestination) const
{
	auto& hasMemos = m_area.m_simulation.m_hasPathMemos;
	auto pair = hasMemos.getDepthFirstSingle(m_area);
	auto& memo = pair.first;
	auto index = pair.second;
	auto output = findPathDepthFirst(from, destinationCondition, accessCondition, huristicDestination, memo);
	hasMemos.releaseDepthFirstSingle(index);
	return output;
}
bool TerrainFacade::canEnterFrom(BlockIndex block, AdjacentIndex adjacentCount) const
{
	Blocks& blocks = m_area.getBlocks();
	BlockIndex other = blocks.indexAdjacentToAtCount(block, adjacentCount);
	assert(m_enterable[(block.get() * maxAdjacent) + adjacentCount.get()] == getValueForBit(block, other));
	return m_enterable[(block.get() * maxAdjacent) + adjacentCount.get()];
}
FindPathResult TerrainFacade::findPathToForSingleBlockShape(BlockIndex start, const Shape& shape, BlockIndex target, bool detour) const
{
	DestinationCondition destinationCondition = [target](BlockIndex index, Facing){ return index == target; };
	AccessCondition accessCondition = makeAccessCondition(shape, start, {start}, detour, DistanceInBlocks::null());
	return findPathDepthFirstWithoutMemo(start, destinationCondition, accessCondition, target);
}
FindPathResult TerrainFacade::findPathToForMultiBlockShape(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	DestinationCondition destinationCondition = [&shape, &blocks, target](BlockIndex index, Facing facing)
	{ 
		auto occupied = shape.getBlocksOccupiedAt(blocks, index, facing);
		return std::ranges::find(occupied, target) != occupied.end();
	};
	BlockIndices occupied = shape.getBlocksOccupiedAt(blocks, start, startFacing);
	AccessCondition accessCondition = makeAccessCondition(shape, start, occupied, detour, DistanceInBlocks::null());
	return findPathDepthFirstWithoutMemo(start, destinationCondition, accessCondition, target);
}
FindPathResult TerrainFacade::findPathToAnyOfForSingleBlockShape(BlockIndex start, const Shape& shape, BlockIndices indecies, BlockIndex huristicDestination, bool detour) const
{
	DestinationCondition destinationCondition = [indecies](BlockIndex index, Facing){ return std::ranges::find(indecies, index) != indecies.end(); };
	AccessCondition accessCondition = makeAccessCondition(shape, start, {start}, detour, DistanceInBlocks::null());
	if(!huristicDestination.exists())
		return findPathBreadthFirstWithoutMemo(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirstWithoutMemo(start, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathToAnyOfForMultiBlockShape(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndices indecies, BlockIndex huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	auto occupied = shape.getBlocksOccupiedAt(blocks, start, startFacing);
	// TODO: should this have a maxRange?
	AccessCondition accessCondition = makeAccessCondition(shape, start, occupied, detour, DistanceInBlocks::null());
	DestinationCondition destinationCondition = [indecies](BlockIndex index, Facing){ return std::ranges::find(indecies, index) != indecies.end(); };
	if(!huristicDestination.exists())
		return findPathBreadthFirstWithoutMemo(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirstWithoutMemo(start, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathToConditionForSingleBlockShape(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristicDestination, bool detour) const
{
	//TODO: should have max range.
	AccessCondition accessCondition = makeAccessCondition(shape, start, {start}, detour, DistanceInBlocks::null());
	if(!huristicDestination.exists())
		return findPathBreadthFirstWithoutMemo(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirstWithoutMemo(start, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathToConditionForMultiBlockShape(BlockIndex start, const Shape& shape, Facing startFacing, const DestinationCondition destinationCondition, BlockIndex huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	auto occupied = shape.getBlocksOccupiedAt(blocks, start, startFacing);
	AccessCondition accessCondition = makeAccessCondition(shape, start, occupied, detour, DistanceInBlocks::null());
	if(!huristicDestination.exists())
		return findPathBreadthFirstWithoutMemo(start, destinationCondition, accessCondition);
	else
		return findPathDepthFirstWithoutMemo(start, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathTo(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, bool detour) const
{
	if(shape.isMultiTile)
		return findPathToForMultiBlockShape(start, shape, startFacing, target, detour);
	else
		return findPathToForSingleBlockShape(start, shape, target, detour);
}
FindPathResult TerrainFacade::findPathToAnyOf(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndices indecies, BlockIndex huristicDestination, bool detour) const
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
	BlockIndices targets;
	Blocks& blocks = m_area.getBlocks();
	auto source = blocks.getAdjacentWithEdgeAndCornerAdjacent(target);
	source.copy_if(targets.back_inserter(), 
		[&](const BlockIndex& block) { 
			return blocks.shape_anythingCanEnterEver(block) && 
			blocks.shape_moveTypeCanEnter(block, m_moveType) && 
			blocks.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(block, shape, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, shape, startFacing, targets, target, detour);
}
//TODO: this could dispatch to specific actor / item variants rather then checking actor or item twice.
FindPathResult TerrainFacade::findPathAdjacentToPolymorphic(BlockIndex start, const Shape& shape, Facing startFacing, ActorOrItemIndex actorOrItem, bool detour) const
{
	BlockIndices targets;
	Blocks& blocks = m_area.getBlocks();
	auto source = actorOrItem.getAdjacentBlocks(m_area);
	source.copy_if(targets.back_inserter(),
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
	DestinationCondition destinationCondition = [&blocks, &shape, condition](BlockIndex index, Facing facing)
	{
		for(BlockIndex adjacent : shape.getBlocksWhichWouldBeAdjacentAt(blocks, index, facing))
			if(condition(adjacent, Facing::create(0)))
				return true;
		return false;
	};
	return findPathToCondition(start, shape, startFacing, condition, huristicDestination, detour);
}
FindPathResult TerrainFacade::findPathAdjacentToConditionAndUnreserved(BlockIndex start, const Shape& shape, Facing startFacing, const DestinationCondition condition, const FactionId faction, BlockIndex huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	DestinationCondition destinationCondition = [&blocks, &shape, &faction, condition](BlockIndex index, Facing facing)
	{
		for(BlockIndex adjacent : shape.getBlocksWhichWouldBeAdjacentAt(blocks, index, facing))
			if(!blocks.isReserved(adjacent, faction) && condition(adjacent, Facing::create(0)))
				return true;
		return false;
	};
	return findPathToCondition(start, shape, startFacing, condition, huristicDestination, detour);
}
FindPathResult TerrainFacade::findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, const FactionId faction, bool detour) const
{
	BlockIndices targets;
	Blocks& blocks = m_area.getBlocks();
	auto source = blocks.getAdjacentWithEdgeAndCornerAdjacent(target);
	source.copy_if(targets.back_inserter(), 
		[&](BlockIndex block){
	       		return blocks.shape_anythingCanEnterEver(block) && !blocks.isReserved(block, faction) && 
			blocks.shape_moveTypeCanEnter(block, m_moveType) && blocks.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(block, shape, m_moveType); 
	});
	if(targets.empty())
		return { };
	// TODO: optimization: If the shape is single tile and the target is enterable ever then path to target directly and discard the final block.
	return findPathToAnyOf(start, shape, startFacing, targets, target, detour);
}
FindPathResult TerrainFacade::findPathAdjacentToAndUnreservedPolymorphic(BlockIndex start, const Shape& shape, Facing startFacing, ActorOrItemIndex actorOrItem, const FactionId faction, bool detour) const
{
	BlockIndices targets;
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
	Actors& actors = m_area.getActors();
	auto& blocks = actors.getBlocks(actor);
	BlockIndices currentlyOccupied(blocks);
	return makeAccessCondition(actors.getShape(actor), actors.getLocation(actor), currentlyOccupied, detour, maxRange);
}
AccessCondition TerrainFacade::makeAccessCondition(const Shape& shape, BlockIndex start, BlockIndices initalBlocks, bool detour, DistanceInBlocks maxRange) const
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
			return [&blocks, maxRange, start](BlockIndex index, Facing) { return blocks.taxiDistance(start, index) <= maxRange; };
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
