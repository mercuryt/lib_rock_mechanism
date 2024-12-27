#include "terrainFacade.hpp"
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
#include "findPathInnerLoops.h"
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
	m_pathRequestsNoHuristic.sortBy([&](const PathRequestNoHuristicData& a, const PathRequestNoHuristicData& b){
		return a.spatialHash < b.spatialHash;
	});
	//#pragma omp parallel for
	for(auto [begin, end] : ranges)
	{
		auto pair = m_area.m_simulation.m_hasPathMemos.getBreadthFirst(m_area);
		auto& memo = *pair.first;
		auto index = pair.second;
		assert(memo.empty());
		for(uint i = begin; i < end; ++i)
		{
			PathRequestIndex pathRequestIndex = PathRequestIndex::create(i);
			auto& pathRequestData = m_pathRequestsNoHuristic[pathRequestIndex];
			// Nullify pointer to path request about to be processed.
			ActorIndex actorIndex = pathRequestData.pathRequest->actor.getIndex(actors.m_referenceData);
			if(actorIndex.exists())
				actors.move_pathRequestClear(actorIndex);
			pathRequestData.result = pathRequestData.pathRequest->readStep(m_area, *this, memo);
			memo.reset();
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
	m_pathRequestsWithHuristic.sortBy([&](const PathRequestWithHuristicData& a, const PathRequestWithHuristicData& b){
		return a.spatialHash < b.spatialHash;
	});
	//#pragma omp parallel for
	for(auto [begin, end] : ranges)
	{
		auto pair = m_area.m_simulation.m_hasPathMemos.getDepthFirst(m_area);
		auto& memo = *pair.first;
		auto index = pair.second;
		assert(memo.empty());
		for(uint i = begin; i < end; ++i)
		{
			PathRequestIndex pathRequestIndex = PathRequestIndex::create(i);
			auto& pathRequestData = m_pathRequestsWithHuristic[pathRequestIndex];
			// Nullify pointer to path request about to be processed.
			ActorIndex actorIndex = pathRequestData.pathRequest->actor.getIndex(actors.m_referenceData);
			if(actorIndex.exists())
				actors.move_pathRequestClear(actorIndex);
			pathRequestData.result = pathRequestData.pathRequest->readStep(m_area, *this, memo);
			memo.reset();
		}
		m_area.m_simulation.m_hasPathMemos.releaseDepthFirst(index);
	}
	// Write step.
	// Move the callback and result data so we can flush the data store and generate new requests for next step within callbacks.
	auto pathRequestsNoHuristic = std::move(m_pathRequestsNoHuristic);
	auto pathRequestsWithHuristic = std::move(m_pathRequestsWithHuristic);
	clearPathRequests();
	// Breadth First.
	for(PathRequestNoHuristicData& pathRequestData : pathRequestsNoHuristic)
		pathRequestData.pathRequest->writeStep(m_area, pathRequestData.result);
	// Depth First.
	for(PathRequestWithHuristicData& pathRequestData : pathRequestsWithHuristic)
		pathRequestData.pathRequest->writeStep(m_area, pathRequestData.result);
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
void TerrainFacade::registerPathRequestNoHuristic(std::unique_ptr<PathRequestBreadthFirst> pathRequest)
{
	m_pathRequestsNoHuristic.emplaceBack(std::move(pathRequest), m_area.getBlocks().getCoordinates(pathRequest->start).hilbertNumber());
}
void TerrainFacade::registerPathRequestWithHuristic(std::unique_ptr<PathRequestDepthFirst> pathRequest)
{
	m_pathRequestsWithHuristic.emplaceBack(std::move(pathRequest), m_area.getBlocks().getCoordinates(pathRequest->start).hilbertNumber());
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
	m_pathRequestsNoHuristic[newIndex] = std::move(m_pathRequestsNoHuristic[oldIndex]);
}
void TerrainFacade::movePathRequestWithHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex)
{
	assert(oldIndex != newIndex);
	m_pathRequestsWithHuristic[newIndex] = std::move(m_pathRequestsWithHuristic[oldIndex]);
}
void TerrainFacade::unregisterNoHuristic(PathRequest& pathRequest)
{
	auto found = std::ranges::find(m_pathRequestsNoHuristic, &pathRequest, [&](const PathRequestNoHuristicData& data) { return data.pathRequest.get();});
	assert(found != m_pathRequestsNoHuristic.end());
	m_pathRequestsNoHuristic.remove(found);
}
void TerrainFacade::unregisterWithHuristic(PathRequest& pathRequest)
{
	auto found = std::ranges::find(m_pathRequestsWithHuristic, &pathRequest, [&](const PathRequestWithHuristicData& data) { return data.pathRequest.get();});
	assert(found != m_pathRequestsWithHuristic.end());
	m_pathRequestsWithHuristic.remove(found);
}
bool TerrainFacade::canEnterFrom(const BlockIndex& block, AdjacentIndex adjacentCount) const
{
	Blocks& blocks = m_area.getBlocks();
	BlockIndex other = blocks.indexAdjacentToAtCount(block, adjacentCount);
	assert(m_enterable[(block.get() * maxAdjacent) + adjacentCount.get()] == getValueForBit(block, other));
	return m_enterable[(block.get() * maxAdjacent) + adjacentCount.get()];
}
FindPathResult TerrainFacade::findPathTo(PathMemoDepthFirst& memo, const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour, bool adjacent, const FactionId& faction) const
{
	auto destinationCondition = [target](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex> { return {block == target, block}; };
	constexpr bool anyOccupiedBlock = false;
	memo.setDestination(target);
	return PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedBlock>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, adjacent, faction, DistanceInBlocks::max());
}
FindPathResult TerrainFacade::findPathToWithoutMemo(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour, bool adjacent, const FactionId& faction) const
{
	auto destinationCondition = [target](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex> { return {block == target, block}; };
	constexpr bool anyOccupiedBlock = false;
	return findPathDepthFirstWithoutMemo<anyOccupiedBlock, decltype(destinationCondition)>(start, startFacing, destinationCondition, target, shape, m_moveType, detour, adjacent, faction, DistanceInBlocks::max());
}

FindPathResult TerrainFacade::findPathToAnyOf(PathMemoDepthFirst& memo, const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, BlockIndices blocks, const BlockIndex huristicDestination, bool detour, const FactionId& faction) const
{

	auto destinationCondition = [huristicDestination, blocks](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex> { return {blocks.contains(block), block}; };
	constexpr bool anyOccupiedBlock = true;
	constexpr bool adjacent = false;
	return PathInnerLoops::findPath<PathMemoDepthFirst, anyOccupiedBlock>(destinationCondition, m_area, *this, memo, shape, m_moveType, start, startFacing, detour, adjacent, faction, DistanceInBlocks::max());
}
FindPathResult TerrainFacade::findPathToAnyOfWithoutMemo(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, BlockIndices blocks, const BlockIndex huristicDestination, bool detour, const FactionId& faction) const
{
	auto destinationCondition = [huristicDestination, blocks](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex> { return {blocks.contains(block), block}; };
	constexpr bool anyOccupiedBlock = true;
	constexpr bool adjacent = false;
	return findPathDepthFirstWithoutMemo<anyOccupiedBlock>(start, startFacing, destinationCondition, huristicDestination, shape, m_moveType, detour, adjacent, faction, DistanceInBlocks::max());
}
FindPathResult TerrainFacade::findPathToBlockDesignation(PathMemoBreadthFirst& memo, const BlockDesignation designation, const FactionId& faction, const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, bool detour, bool adjacent, const DistanceInBlocks& maxRange) const
{
	auto destinationCondition = [](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex> { return {true, block}; };
	constexpr bool anyOccupiedBlock = true;
	return findPathToBlockDesignationAndCondition<anyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, memo, designation, start, startFacing, shape, detour, adjacent, faction, maxRange);
}
//TODO: this could dispatch to specific actor / item variants rather then checking actor or item twice.
FindPathResult TerrainFacade::findPathAdjacentToPolymorphicWithoutMemo(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const ActorOrItemIndex& actorOrItem, bool detour) const
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
	return findPathToAnyOfWithoutMemo(start, startFacing, shape, targets, actorOrItem.getLocation(m_area), detour);
}
FindPathResult TerrainFacade::findPathToEdge(PathMemoBreadthFirst& memo, const BlockIndex &start, const Facing &startFacing, const ShapeId &shape, bool detour) const
{
	Blocks& blocks = m_area.getBlocks();
	auto destinationCondition = [&](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex>
	{
		return {blocks.isEdge(block), block};
	};
	constexpr bool useAnyOccupiedBlock = true;
	constexpr bool adjacent = false;
	constexpr FactionId faction;
	return findPathToConditionBreadthFirst<useAnyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, memo, start, startFacing, shape, detour, adjacent, faction, DistanceInBlocks::max());
}
bool TerrainFacade::accessable(const BlockIndex& start, const Facing& startFacing, const BlockIndex& to, const ActorIndex& actor) const
{
	Actors& actors = m_area.getActors();
	auto result = findPathToWithoutMemo(start, startFacing, actors.getShape(actor), to);
	return !result.path.empty() || result.useCurrentPosition;
}
void AreaHasTerrainFacades::doStep()
{
	for(auto& pair : m_data)
		pair.second->doStep();
}
void AreaHasTerrainFacades::update(const BlockIndex& index)
{
	for(auto& pair : m_data)
		pair.second->update(index);
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
		pair.second->clearPathRequests();
}
TerrainFacade& AreaHasTerrainFacades::getForMoveType(const MoveTypeId& moveType)
{
	assert(m_data.contains(moveType));
	return m_data[moveType];
}