// TODO: code is repeated 3 times each for access condition and destination condition definition.
#include "terrainFacade.h"
#include "actors/actors.h"
#include "area.h"
#include "callbackTypes.h"
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
#include <iterator>

TerrainFacade::TerrainFacade(Area& area, const MoveTypeId& moveType) : m_area(area), m_moveType(moveType)
{
	m_enterable.resize(m_area.getBlocks().size() * maxAdjacent);
	for(BlockIndex block : m_area.getBlocks().getAll())
		update(block);
	// Two results fit on a  cache line, by ensuring that the number of tasks per thread is a multiple of 2 we prevent false shareing.
	assert(Config::pathRequestsPerThread % 2 == 0);
}
void TerrainFacade::doStep()
{
	Actors& actors = m_area.getActors();
	// Read step.
	// Breadth first.
	uint numberOfRequests = m_pathRequestsNoHuristic.size();
	uint i = 0;
	std::vector<std::pair<uint, uint>> ranges;
	while(i < numberOfRequests)
	{
		uint end = std::min(numberOfRequests, i + Config::pathRequestsPerThread);
		ranges.emplace_back(i, end);
		i = end;
	}
	/*
	Blocks& blocks = m_area.getBlocks();
	m_pathRequestsNoHuristic.sortBy([&](const PathRequestNoHuristic& a, const PathRequestNoHuristic& b){
		return blocks.getCoordinates(a.startLocation).hilbertNumber() < blocks.getCoordinates(b.startLocation).hilbertNumber();
	});
	m_pathRequestsWithHuristic.sortBy([&](const PathRequestWithHuristic& a, const PathRequestWithHuristic& b){
		return blocks.getCoordinates(a.startLocation).hilbertNumber() < blocks.getCoordinates(b.startLocation).hilbertNumber();
	});
	*/
	//#pragma omp parallel for
	for(auto [begin, end] : ranges)
	{
		auto pair = m_area.m_simulation.m_hasPathMemos.getBreadthFirst(m_area);
		auto& memo = *pair.first;
		auto index = pair.second;
		assert(memo.empty());
		for(uint i = begin; i < end; ++i)
		{
			assert(actors.move_hasPathRequest(m_pathRequestsNoHuristic[PathRequestIndex::create(i)].actor.getIndex(actors.m_referenceData)));
			findPathForIndexNoHuristic(PathRequestIndex::create(i), memo);
			assert(memo.empty());
		}
		m_area.m_simulation.m_hasPathMemos.releaseBreadthFirst(index);
	}
	// Depth First.
	numberOfRequests = m_pathRequestsWithHuristic.size();
	i = 0;
	ranges.clear();
	while(i < numberOfRequests)
	{
		uint end = std::min(numberOfRequests, i + Config::pathRequestsPerThread);
		ranges.emplace_back(i, end);
		i = end;
	}
	//#pragma omp parallel for
	for(auto [begin, end] : ranges)
	{
		auto pair = m_area.m_simulation.m_hasPathMemos.getDepthFirst(m_area);
		auto& memo = *pair.first;
		auto index = pair.second;
		assert(memo.empty());
		for(uint i = begin; i < end; ++i)
		{
			findPathForIndexWithHuristic(PathRequestIndex::create(i), memo);
			assert(memo.empty());
		}
		m_area.m_simulation.m_hasPathMemos.releaseDepthFirst(index);
	}
	// Write step.
	// Move the callback and result data so we can flush the data store and generate new requests for next step within callbacks.
	auto pathRequestsNoHuristic = std::move(m_pathRequestsNoHuristic);
	auto pathRequestsWithHuristic = std::move(m_pathRequestsWithHuristic);
	clearPathRequests();
	// Breadth First.
	for(PathRequestIndex i = PathRequestIndex::create(0); i < pathRequestsNoHuristic.size(); ++i)
	{
		// Move path request here and nullify the moved from location so that the callback can create a new request.
		ActorIndex actor = pathRequestsNoHuristic[i].actor.getIndex(actors.m_referenceData);
		assert(actors.move_hasPathRequest(actor));
		std::unique_ptr<PathRequest> pathRequest = actors.move_movePathRequestData(actor);
		if constexpr(DEBUG)
			pathRequestsNoHuristic[i].actor.clear();
		pathRequest->callback(m_area, pathRequestsNoHuristic[i].result);
	}
	// Depth First.
	for(PathRequestIndex i = PathRequestIndex::create(0); i < pathRequestsWithHuristic.size(); ++i)
	{
		// Move path request here and nullify the moved from location so that the callback can create a new request.
		std::unique_ptr<PathRequest> pathRequest = actors.move_movePathRequestData(pathRequestsWithHuristic[i].actor.getIndex(actors.m_referenceData));
		pathRequest->callback(m_area, pathRequestsWithHuristic[i].result);
	}
}
void TerrainFacade::findPathForIndexWithHuristic(PathRequestIndex index, PathMemoDepthFirst& memo)
{
	PathRequestWithHuristic& request = m_pathRequestsWithHuristic[index];
	request.result = findPathDepthFirst(
		request.startLocation,
		request.startFacing,
		request.destinationCondition,
		request.accessCondition,
		request.huristicDestination,
		memo);
	memo.reset();
}
void TerrainFacade::findPathForIndexNoHuristic(PathRequestIndex index, PathMemoBreadthFirst& memo)
{
	PathRequestNoHuristic& request = m_pathRequestsNoHuristic[index];
	request.result = findPathBreadthFirst(
		request.startLocation,
		request.startFacing,
		request.destinationCondition,
		request.accessCondition,
		memo
	);
	memo.reset();
}
void TerrainFacade::clearPathRequests()
{
	m_pathRequestsNoHuristic.clear();
	m_pathRequestsWithHuristic.clear();
}
bool TerrainFacade::getValueForBit(const BlockIndex& from, const BlockIndex& to) const
{
	Blocks& blocks = m_area.getBlocks();
	return to.exists() &&
		blocks.shape_anythingCanEnterEver(to) &&
		blocks.shape_moveTypeCanEnter(to, m_moveType) &&
		blocks.shape_moveTypeCanEnterFrom(to, m_moveType, from);
}
PathRequestIndex TerrainFacade::getPathRequestIndexNoHuristic()
{
	PathRequestIndex index = PathRequestIndex::create(m_pathRequestsNoHuristic.size());
	m_pathRequestsNoHuristic.resize(index + 1);
	return index;
}
PathRequestIndex TerrainFacade::getPathRequestIndexWithHuristic()
{
	PathRequestIndex index = PathRequestIndex::create(m_pathRequestsWithHuristic.size());
	m_pathRequestsWithHuristic.resize(index + 1);
	return index;
}
PathRequestIndex TerrainFacade::registerPathRequestNoHuristic(const BlockIndex& start, const Facing& startFacing, AccessCondition& access, DestinationCondition& destination, PathRequest& pathRequest)
{
	assert(start.exists());
	ActorReference ref = m_area.getActors().m_referenceData.getReference(pathRequest.getActor());
	m_pathRequestsNoHuristic.emplaceBack(access, destination, start, ref, startFacing);
	PathRequestIndex index = PathRequestIndex::create(m_pathRequestsNoHuristic.size() - 1);
	pathRequest.update(index);
	return index;
}
PathRequestIndex TerrainFacade::registerPathRequestWithHuristic(const BlockIndex& start, const Facing& startFacing, AccessCondition& access, DestinationCondition& destination, const BlockIndex& huristic, PathRequest& pathRequest)
{
	assert(start.exists());
	ActorReference ref = m_area.getActors().m_referenceData.getReference(pathRequest.getActor());
	m_pathRequestsWithHuristic.emplaceBack(access, destination, start, huristic, ref, startFacing);
	PathRequestIndex index = PathRequestIndex::create(m_pathRequestsWithHuristic.size() - 1);
	pathRequest.update(index);
	return index;
}
void TerrainFacade::update(const BlockIndex& index)
{
	Blocks& blocks = m_area.getBlocks();
	uint i = 0;
	uint base = index.get() * maxAdjacent; 
	for(BlockIndex adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacentUnfiltered(index))
	{
		m_enterable[base + i] = getValueForBit(index, adjacent);
		++i;
	}
}
void TerrainFacade::movePathRequestNoHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex)
{
	assert(oldIndex != newIndex);
	m_pathRequestsNoHuristic[newIndex] = m_pathRequestsNoHuristic[oldIndex];
	ActorIndex actor = m_pathRequestsNoHuristic[newIndex].actor.getIndex(m_area.getActors().m_referenceData);
	m_area.getActors().move_updatePathRequestTerrainFacadeIndex(actor, newIndex);
}
void TerrainFacade::movePathRequestWithHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex)
{
	assert(oldIndex != newIndex);
	m_pathRequestsWithHuristic[newIndex] = m_pathRequestsWithHuristic[oldIndex];
	ActorIndex actor = m_pathRequestsWithHuristic[newIndex].actor.getIndex(m_area.getActors().m_referenceData);
	m_area.getActors().move_updatePathRequestTerrainFacadeIndex(actor, newIndex);
}
void TerrainFacade::unregisterNoHuristic(PathRequestIndex index)
{
	assert(m_pathRequestsNoHuristic.size() > index);
	if(m_pathRequestsNoHuristic.size() == 1)
		m_pathRequestsNoHuristic.clear();
	else
	{
		// Swap the index with the last one.
		PathRequestIndex lastIndex = PathRequestIndex::create(m_pathRequestsNoHuristic.size() - 1);
		if(index != lastIndex)
			movePathRequestNoHuristic(lastIndex, index);
		m_pathRequestsNoHuristic.popBack();
	}
}
void TerrainFacade::unregisterWithHuristic(PathRequestIndex index)
{
	assert(m_pathRequestsWithHuristic.size() > index);
	if(m_pathRequestsWithHuristic.size() == 1)
		m_pathRequestsWithHuristic.clear();
	else
	{
		// Swap the index with the last one.
		PathRequestIndex lastIndex = PathRequestIndex::create(m_pathRequestsWithHuristic.size() - 1);
		if(index != lastIndex)
			movePathRequestWithHuristic(lastIndex, index);
		m_pathRequestsWithHuristic.popBack();
	}
}
FindPathResult TerrainFacade::findPath(const BlockIndex& start, const Facing& startFacing, DestinationCondition& destinationCondition, AccessCondition& accessCondition, const OpenListPush& openListPush, const OpenListPop& openListPop, const OpenListEmpty& openListEmpty, const ClosedListAdd& closedListAdd, const ClosedListContains& closedListContains, const ClosedListGetPath& closedListGetPath) const
{
	Blocks& blocks = m_area.getBlocks();
	auto [result, blockWhichPassedPredicate] = destinationCondition(start, startFacing);
	if(result)
		return {{}, blockWhichPassedPredicate, true};
	// Use BlockIndex::max to indicate the start.
	openListPush(start);
	closedListAdd(start, BlockIndex::max());
	while(!openListEmpty())
	{
		BlockIndex current = openListPop();
		[[maybe_unused]] uint indexInt = current.get();
		for(AdjacentIndex adjacentCount = AdjacentIndex::create(0); adjacentCount < maxAdjacent; ++adjacentCount)
		{
			[[maybe_unused]] uint adjacentCountInt = adjacentCount.get();
			if(canEnterFrom(current, adjacentCount))
			{
				BlockIndex adjacentIndex = blocks.indexAdjacentToAtCount(current, adjacentCount);
				[[maybe_unused]] uint adjacentIndexInt = adjacentIndex.get();
				assert(adjacentIndex.exists());
				if(closedListContains(adjacentIndex))
					continue;
				//TODO: Make variant for radially semetric shapes which ignores facing.
				Facing facing = blocks.facingToSetWhenEnteringFrom(adjacentIndex, current);
				if(!accessCondition(adjacentIndex, facing))
					continue;
				auto [result, blockWhichPassedPredicate] = destinationCondition(adjacentIndex, facing);
				if(result)
				{
					auto path = closedListGetPath(current);
					// No need to add the adjacent to the closed list and then extract it again for the path, just add to the end of the path directly.
					path.add(adjacentIndex);
					return {path, blockWhichPassedPredicate, false};
				}
				openListPush(adjacentIndex);
				closedListAdd(adjacentIndex, current);
			}
		}
	}
	return {{}, BlockIndex::null(), false};
}
FindPathResult TerrainFacade::findPathBreadthFirst(const BlockIndex& start, const Facing& startFacing, DestinationCondition& destinationCondition, AccessCondition& accessCondition, PathMemoBreadthFirst& memo) const
{
	OpenListEmpty empty = [&memo](){ return memo.openEmpty(); };
	OpenListPush push  = [&memo](const BlockIndex& index) { memo.setOpen(index); };
	OpenListPop pop = [&memo](){ return memo.next(); };
	ClosedListAdd closedAdd = [&memo](const BlockIndex& index, const BlockIndex& previous) { return memo.setClosed(index, previous); };
	ClosedListContains closedContains = [&memo](const BlockIndex& index) { return memo.isClosed(index); };
	ClosedListGetPath getPath = [&memo](const BlockIndex& end) { return memo.getPath(end); };
	return findPath(start, startFacing, destinationCondition, accessCondition, push, pop, empty, closedAdd, closedContains, getPath);
}
FindPathResult TerrainFacade::findPathDepthFirst(const BlockIndex& start, const Facing& startFacing, DestinationCondition& destinationCondition, AccessCondition& accessCondition, const BlockIndex& huristicDestination, PathMemoDepthFirst& memo) const
{
	OpenListEmpty empty = [&memo](){ return memo.openEmpty(); };
	OpenListPush push  = [ &memo, huristicDestination, this](const BlockIndex& index) { memo.setOpen(index, huristicDestination, m_area); };
	OpenListPop pop = [&memo](){ return memo.next(); };
	ClosedListAdd closedAdd = [&memo](const BlockIndex& index, const BlockIndex& previous) { return memo.setClosed(index, previous); };
	ClosedListContains closedContains = [&memo](const BlockIndex& index) { return memo.isClosed(index); };
	ClosedListGetPath getPath = [&memo](const BlockIndex& end) { return memo.getPath(end); };
	return findPath(start, startFacing, destinationCondition, accessCondition, push, pop, empty, closedAdd, closedContains, getPath);
}
FindPathResult TerrainFacade::findPathBreadthFirstWithoutMemo(const BlockIndex& start, const Facing& startFacing, DestinationCondition& destinationCondition, AccessCondition& accessCondition) const
{
	auto& hasMemos = m_area.m_simulation.m_hasPathMemos;
	auto pair = hasMemos.getBreadthFirst(m_area);
	auto& memo = *pair.first;
	auto index = pair.second;
	auto output = findPathBreadthFirst(start, startFacing, destinationCondition, accessCondition, memo);
	memo.reset();
	hasMemos.releaseBreadthFirst(index);
	return output;
}
FindPathResult TerrainFacade::findPathDepthFirstWithoutMemo(const BlockIndex& start, const Facing& startFacing, DestinationCondition& destinationCondition, AccessCondition& accessCondition, const BlockIndex& huristicDestination) const
{
	auto& hasMemos = m_area.m_simulation.m_hasPathMemos;
	auto pair = hasMemos.getDepthFirst(m_area);
	auto& memo = *pair.first;
	auto index = pair.second;
	auto output = findPathDepthFirst(start, startFacing, destinationCondition, accessCondition, huristicDestination, memo);
	memo.reset();
	hasMemos.releaseDepthFirst(index);
	return output;
}
bool TerrainFacade::canEnterFrom(const BlockIndex& block, AdjacentIndex adjacentCount) const
{
	Blocks& blocks = m_area.getBlocks();
	BlockIndex other = blocks.indexAdjacentToAtCount(block, adjacentCount);
	assert(m_enterable[(block.get() * maxAdjacent) + adjacentCount.get()] == getValueForBit(block, other));
	return m_enterable[(block.get() * maxAdjacent) + adjacentCount.get()];
}
FindPathResult TerrainFacade::findPathToForSingleBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour) const
{
	DestinationCondition destinationCondition = [target](const BlockIndex& index, Facing){
		return std::make_pair(index == target, index);
	};
	AccessCondition accessCondition = makeAccessCondition(shape, start, {start}, detour);
	return findPathDepthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition, target);
}
FindPathResult TerrainFacade::findPathToForMultiBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	DestinationCondition destinationCondition = [shape, &blocks, target](const BlockIndex& index, const Facing& facing)
	{ 
		auto occupied = Shape::getBlocksOccupiedAt(shape, blocks, index, facing);
		bool result = std::ranges::find(occupied, target) != occupied.end();
		return std::make_pair(result, index);

	};
	BlockIndices occupied = Shape::getBlocksOccupiedAt(shape, blocks, start, startFacing);
	AccessCondition accessCondition = makeAccessCondition(shape, start, occupied, detour);
	return findPathDepthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition, target);
}
FindPathResult TerrainFacade::findPathToAnyOfForSingleBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, BlockIndices indecies, const BlockIndex& huristicDestination, bool detour) const
{
	DestinationCondition destinationCondition = [indecies](const BlockIndex& index, Facing){
		bool result = std::ranges::find(indecies, index) != indecies.end();
		return std::make_pair(result, index);
	};
	AccessCondition accessCondition = makeAccessCondition(shape, start, {start}, detour);
	if(!huristicDestination.exists())
		return findPathBreadthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition);
	else
		return findPathDepthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathToAnyOfForMultiBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, BlockIndices indecies, const BlockIndex& huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	auto occupied = Shape::getBlocksOccupiedAt(shape, blocks, start, startFacing);
	// TODO: should this have a maxRange?
	AccessCondition accessCondition = makeAccessCondition(shape, start, occupied, detour);
	DestinationCondition destinationCondition = [indecies](const BlockIndex& index, Facing){
		bool result = std::ranges::find(indecies, index) != indecies.end();
		return std::make_pair(result, index);
	};
	if(!huristicDestination.exists())
		return findPathBreadthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition);
	else
		return findPathDepthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathToConditionForSingleBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, DestinationCondition& destinationCondition, const BlockIndex& huristicDestination, bool detour) const
{
	//TODO: should have max range.
	AccessCondition accessCondition = makeAccessCondition(shape, start, {start}, detour);
	if(!huristicDestination.exists())
		return findPathBreadthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition);
	else
		return findPathDepthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathToConditionForMultiBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, DestinationCondition& destinationCondition, const BlockIndex& huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	auto occupied = Shape::getBlocksOccupiedAt(shape, blocks, start, startFacing);
	AccessCondition accessCondition = makeAccessCondition(shape, start, occupied, detour);
	if(!huristicDestination.exists())
		return findPathBreadthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition);
	else
		return findPathDepthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition, huristicDestination);
}
FindPathResult TerrainFacade::findPathTo(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour) const
{
	if(Shape::getIsMultiTile(shape))
		return findPathToForMultiBlockShape(start, startFacing, shape, target, detour);
	else
		return findPathToForSingleBlockShape(start, startFacing, shape, target, detour);
}
FindPathResult TerrainFacade::findPathToAnyOf(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, BlockIndices indecies, const BlockIndex huristicDestination, bool detour) const
{
	if(Shape::getIsMultiTile(shape))
		return findPathToAnyOfForMultiBlockShape(start, startFacing, shape, indecies, huristicDestination, detour);
	else
		return findPathToAnyOfForSingleBlockShape(start, startFacing, shape, indecies, huristicDestination, detour);
}
FindPathResult TerrainFacade::findPathToCondition(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, DestinationCondition& destinationCondition, const BlockIndex huristicDestination, bool detour) const
{
	if(Shape::getIsMultiTile(shape))
		return findPathToConditionForMultiBlockShape(start, startFacing, shape, destinationCondition, huristicDestination, detour);
	else
		return findPathToConditionForSingleBlockShape(start, startFacing, shape, destinationCondition, huristicDestination, detour);
}
FindPathResult TerrainFacade::findPathAdjacentTo(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour) const
{
	BlockIndices targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(
		blocks.getAdjacentWithEdgeAndCornerAdjacent(target),
		targets.back_inserter(),
		[&](const BlockIndex& block) { 
			return blocks.shape_anythingCanEnterEver(block) && 
			blocks.shape_moveTypeCanEnter(block, m_moveType) && 
			blocks.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(block, shape, m_moveType); 
		}
	);
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, startFacing, shape, targets, target, detour);
}
//TODO: this could dispatch to specific actor / item variants rather then checking actor or item twice.
FindPathResult TerrainFacade::findPathAdjacentToPolymorphic(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const ActorOrItemIndex& actorOrItem, bool detour) const
{
	BlockIndices targets;
	Blocks& blocks = m_area.getBlocks();
	auto source = actorOrItem.getAdjacentBlocks(m_area);
	source.copy_if(targets.back_inserter(),
		[&](const BlockIndex& block) { 
			return blocks.shape_anythingCanEnterEver(block) && blocks.shape_moveTypeCanEnter(block, m_moveType) && blocks.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(block, shape, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, startFacing, shape, targets, actorOrItem.getLocation(m_area), detour);
}
FindPathResult TerrainFacade::findPathAdjacentToCondition(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, DestinationCondition& condition, const BlockIndex huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	DestinationCondition destinationCondition = [&blocks, shape, condition](const BlockIndex& index, const Facing& facing)
	{
		for(BlockIndex adjacent : Shape::getBlocksWhichWouldBeAdjacentAt(shape, blocks, index, facing))
			if(condition(adjacent, Facing::create(0)).first)
				return std::make_pair(true, adjacent);
		return std::make_pair(false, BlockIndex::null());
	};
	return findPathToCondition(start, startFacing, shape, condition, huristicDestination, detour);
}
FindPathResult TerrainFacade::findPathAdjacentToConditionAndUnreserved(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, DestinationCondition& condition, const FactionId& faction, const BlockIndex huristicDestination, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	DestinationCondition destinationCondition = [&blocks, shape, &faction, condition](const BlockIndex& index, const Facing& facing)
	{
		for(BlockIndex adjacent : Shape::getBlocksWhichWouldBeAdjacentAt(shape, blocks, index, facing))
			if(!blocks.isReserved(adjacent, faction) && condition(adjacent, Facing::create(0)).first)
				return std::make_pair(true, adjacent);
		return std::make_pair(false, BlockIndex::null());
	};
	return findPathToCondition(start, startFacing, shape, condition, huristicDestination, detour);
}
FindPathResult TerrainFacade::findPathAdjacentToAndUnreserved(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, const FactionId& faction, bool detour) const
{
	BlockIndices targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(
		blocks.getAdjacentWithEdgeAndCornerAdjacent(target),
		targets.back_inserter(), 
		[&](const BlockIndex& block){
	       		return blocks.shape_anythingCanEnterEver(block) && !blocks.isReserved(block, faction) && 
			blocks.shape_moveTypeCanEnter(block, m_moveType) && blocks.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(block, shape, m_moveType); 
		}
	);
	if(targets.empty())
		return { };
	// TODO: optimization: If the shape is single tile and the target is enterable ever then path to target directly and discard the final block.
	return findPathToAnyOf(start, startFacing, shape, targets, target, detour);
}
FindPathResult TerrainFacade::findPathAdjacentToAndUnreservedPolymorphic(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const ActorOrItemIndex& actorOrItem, const FactionId& faction, bool detour) const
{
	BlockIndices targets;
	Blocks& blocks = m_area.getBlocks();
	std::ranges::copy_if(actorOrItem.getAdjacentBlocks(m_area), targets.begin(), 
		[&](const BlockIndex& block){ return !blocks.isReserved(block, faction) && blocks.shape_moveTypeCanEnter(block, m_moveType); 
	});
	if(targets.empty())
		return { };
	return findPathToAnyOf(start, startFacing, shape, targets, actorOrItem.getLocation(m_area), detour);
}
AccessCondition TerrainFacade::makeAccessConditionForActor(const ActorIndex& actor, const BlockIndex& start, bool detour, const DistanceInBlocks maxRange) const
{
	Actors& actors = m_area.getActors();
	if(actors.isLeading(actor))
	{
		return makeAccessCondition(actors.lineLead_getLargestShape(actor), start, actors.lineLead_getOccupiedBlocks(actor), detour, maxRange);
	}
		return makeAccessCondition(actors.getShape(actor), start, actors.getBlocks(actor), detour, maxRange);
}
AccessCondition TerrainFacade::makeAccessCondition(const ShapeId& shape, const BlockIndex& start, const BlockIndices& initalBlocks, bool detour, const DistanceInBlocks maxRange) const
{
	Blocks& blocks = m_area.getBlocks();
	if(Shape::getIsMultiTile(shape))
	{
		if(detour)
		{
			return [this, initalBlocks, shape, &blocks, maxRange, start](const BlockIndex& index, const Facing& facing) -> bool { 
				return 
					blocks.taxiDistance(start, index) <= maxRange &&
					m_area.getBlocks().shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(index, shape, m_moveType, facing, initalBlocks);
			};
		}
		else
			return [this, shape, &blocks, maxRange, start](const BlockIndex& index, const Facing& facing){
				return 
					blocks.taxiDistance(start, index) <= maxRange &&
					blocks.shape_shapeAndMoveTypeCanEnterEverWithFacing(index, shape, m_moveType, facing);
			};
	}
	else
	{
		if(detour)
		{
			CollisionVolume volume = Shape::getCollisionVolumeAtLocationBlock(shape);
			assert(initalBlocks.size() == 1);
			BlockIndex initalBlock = initalBlocks.front();
			return [volume, &blocks, initalBlock, maxRange, start](const BlockIndex& index, Facing)
			{
				return 
					blocks.taxiDistance(start, index) <= maxRange &&
					(index == initalBlock || blocks.shape_getDynamicVolume(index) + volume <= Config::maxBlockVolume);
			};
		}
		else if(maxRange == DistanceInBlocks::max())
			return [](const BlockIndex&, const Facing&) { return true; };
		else
			return [&blocks, maxRange, start](const BlockIndex& index, const Facing&) { return blocks.taxiDistance(start, index) <= maxRange; };
	}
}
bool TerrainFacade::accessable(const BlockIndex& start, const Facing& startFacing, const BlockIndex& to, const ActorIndex& actor) const
{
	DestinationCondition destinationCondition = [to](const BlockIndex& block, Facing) -> std::pair<bool, BlockIndex>{
		if(block == to) return {true, block}; else return {false, BlockIndex::null()};
	};
	const bool detour = false;
	AccessCondition accessCondition = makeAccessConditionForActor(actor, start, detour, DistanceInBlocks::max());
	auto result = findPathDepthFirstWithoutMemo(start, startFacing, destinationCondition, accessCondition, to);
	return !result.path.empty() || result.useCurrentPosition;
}
void AreaHasTerrainFacades::doStep()
{
	for(auto& pair : m_data)
		pair.second.doStep();
}
void AreaHasTerrainFacades::update(const BlockIndex& index)
{
	for(auto& pair : m_data)
		pair.second.update(index);
}
void AreaHasTerrainFacades::maybeRegisterMoveType(const MoveTypeId& moveType)
{
	//TODO: generate terrain facade in thread.
	auto found = m_data.find(moveType);
	if(found == m_data.end())
		m_data.emplace(moveType, m_area, moveType);
}
void AreaHasTerrainFacades::updateBlockAndAdjacent(const BlockIndex& block)
{
	Blocks& blocks = m_area.getBlocks();
	if(blocks.shape_anythingCanEnterEver(block))
		update(block);
	for(BlockIndex adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacent(block))
		if(blocks.shape_anythingCanEnterEver(adjacent))
			update(adjacent);
}
void AreaHasTerrainFacades::clearPathRequests()
{
	for(auto& pair : m_data)
		pair.second.clearPathRequests();
}
TerrainFacade& AreaHasTerrainFacades::getForMoveType(const MoveTypeId& moveType)
{
	assert(m_data.contains(moveType));
	return m_data[moveType];
}
