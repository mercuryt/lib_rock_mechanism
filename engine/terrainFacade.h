/*
 * TerrainFacade:
 * Stores enterability data for a specific move type for pair of blocks in a binary format.
 * Also stores path requests in a series of vectors divided by presence or absence of destination huristic.
 * This divide is done for instruction cache locality.
 */ 
#pragma once

#include "config.h"
#include "designations.h"
#include "index.h"
#include "pathMemo.h"
#include "types.h"
#include "callbackTypes.h"
#include "dataVector.h"
#include "../lib/dynamic_bitset.hpp"

#include <pthread.h>

struct MoveType;
struct Shape;
struct FluidType;
class Area;
class TerrainFacade;
class ActorOrItemIndex;
class PathRequest;
class PathMemoBreadthFirst;
class PathMemoDepthFirst;

constexpr int maxAdjacent = 26;
// TODO: optimization: single block shapes don't need facing

struct FindPathResult
{
	BlockIndices path;
	BlockIndex blockThatPassedPredicate;
	bool useCurrentPosition;
};
class TerrainFacade final
{
	// Without Huristic.
	DataVector<BlockIndex, PathRequestIndex> m_pathRequestStartPositionNoHuristic;
	DataVector<AccessCondition, PathRequestIndex> m_pathRequestAccessConditionsNoHuristic;
	DataVector<DestinationCondition, PathRequestIndex> m_pathRequestDestinationConditionsNoHuristic;
	// Result is stored instead of being immideatly dispatched to PathRequest to reduce cache thrashing.
	// Stored as vector of structs rather then multiple vectors of primitives to prevent cache line false shareing.
	// 	sizeof(FindPathResult) == 32, so 2 can fit on a cache line.
	// 	So if Config::pathRequestsPerThread is a multiple of 2 there will never be a shared cache line being written to.
	DataVector<FindPathResult, PathRequestIndex> m_pathRequestResultsNoHuristic;
	DataVector<PathRequest*, PathRequestIndex> m_pathRequestNoHuristic;
	// With Huristic.
	DataVector<BlockIndex, PathRequestIndex> m_pathRequestStartPositionWithHuristic;
	DataVector<AccessCondition, PathRequestIndex> m_pathRequestAccessConditionsWithHuristic;
	DataVector<DestinationCondition, PathRequestIndex> m_pathRequestDestinationConditionsWithHuristic;
	DataVector<BlockIndex, PathRequestIndex> m_pathRequestHuristic;
	DataVector<FindPathResult, PathRequestIndex> m_pathRequestResultsWithHuristic;
	DataVector<PathRequest*, PathRequestIndex> m_pathRequestWithHuristic;
	sul::dynamic_bitset<> m_enterable;
	Area& m_area;
	const MoveType& m_moveType;
	// DestinationCondition could test against a set of destination indecies or load the actual block to do more complex checks.
	// AccessCondition could test for larger shapes or just return true for 1x1x1 size.
	// TODO: huristic destination.
	[[nodiscard]] FindPathResult findPath(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, OpenListPush openListPush, OpenListPop openListPop, OpenListEmpty openListEmpty) const;
	[[nodiscard]] FindPathResult findPathBreadthFirst(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, PathMemoBreadthFirst& memo) const;
	[[nodiscard]] FindPathResult findPathDepthFirst(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, BlockIndex huristicDestination, PathMemoDepthFirst& memo) const;
	// Non batched pathing uses that WithoutMemo variants.
	[[nodiscard]] FindPathResult findPathBreadthFirstWithoutMemo(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition) const;
	[[nodiscard]] FindPathResult findPathDepthFirstWithoutMemo(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, BlockIndex huristicDestination) const;
	[[nodiscard]] bool canEnterFrom(BlockIndex blockIndex, AdjacentIndex adjacentIndex) const;
	[[nodiscard]] FindPathResult findPathToForSingleBlockShape(BlockIndex start, const Shape& shape, BlockIndex target, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToForMultiBlockShape(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToAnyOfForSingleBlockShape(BlockIndex start, const Shape& shape, BlockIndices indecies, BlockIndex huristincDestination, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToAnyOfForMultiBlockShape(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndices indecies, BlockIndex huristincDestination, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToConditionForSingleBlockShape(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristincDestination, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToConditionForMultiBlockShape(BlockIndex start, const Shape& shape, Facing startFacing, const DestinationCondition destinationCondition, BlockIndex huristincDestination, bool detour = false) const;
	bool getValueForBit(BlockIndex from, BlockIndex to) const;
	PathRequestIndex getPathRequestIndexNoHuristic();
	PathRequestIndex getPathRequestIndexWithHuristic();
	void movePathRequestNoHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex);
	void movePathRequestWithHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex);
	void resizePathRequestNoHuristic(PathRequestIndex size);
	void resizePathRequestWithHuristic(PathRequestIndex size);
public:
	TerrainFacade(Area& area, const MoveType& moveType);
	void doStep();
	void findPathForIndexWithHuristic(PathRequestIndex index, PathMemoDepthFirst& memo);
	void findPathForIndexNoHuristic(PathRequestIndex index, PathMemoBreadthFirst& memo);
	PathRequestIndex registerPathRequestNoHuristic(BlockIndex start, AccessCondition acces, DestinationCondition destination, PathRequest& pathRequest);
	PathRequestIndex registerPathRequestWithHuristic(BlockIndex start, AccessCondition acces, DestinationCondition destination, BlockIndex huristic, PathRequest& pathRequest);
	void unregisterNoHuristic(PathRequestIndex index);
	void unregisterWithHuristic(PathRequestIndex index);
	void update(BlockIndex block);
	void clearPathRequests();
	[[nodiscard]] FindPathResult findPathTo(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToAnyOf(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndices indecies, BlockIndex huristicDestination = BlockIndex::null(), bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToCondition(BlockIndex start, const Shape& shape, Facing startFacing, const DestinationCondition destinationCondition, BlockIndex huristicDestination = BlockIndex::null(), bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentTo(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToPolymorphic(BlockIndex start, const Shape& shape, Facing startFacing, ActorOrItemIndex actorOrItem, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, Facing startFacing, BlockIndex target, const FactionId faction, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToAndUnreservedPolymorphic(BlockIndex start, const Shape& shape, Facing startFacing, ActorOrItemIndex actorOrItem, const FactionId faction, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToCondition(BlockIndex start, const Shape& shape, Facing startFacing, const DestinationCondition destinationCondition, BlockIndex huristicDestination = BlockIndex::null(), bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToConditionAndUnreserved(BlockIndex start, const Shape& shape, Facing startFacing, const DestinationCondition destinationCondition, const FactionId faction, BlockIndex huristicDestination = BlockIndex::null(), bool detour = false) const;
	[[nodiscard]] size_t getPathRequestCount() const { return m_pathRequestAccessConditionsNoHuristic.size() + m_pathRequestAccessConditionsWithHuristic.size(); }
	[[nodiscard]] AccessCondition makeAccessConditionForActor(ActorIndex actor, bool detour, DistanceInBlocks maxRange) const;
	[[nodiscard]] AccessCondition makeAccessCondition(const Shape& shape, BlockIndex start, BlockIndices initalBlocks, bool detour, DistanceInBlocks maxRange) const;
};
class AreaHasTerrainFacades
{
	std::unordered_map<const MoveType*, TerrainFacade> m_data;
	Area& m_area;
	void update(BlockIndex block);
public:
	AreaHasTerrainFacades(Area& area) : m_area(area) { }
	void doStep();
	void updateBlockAndAdjacent(BlockIndex block);
	TerrainFacade& at(const MoveType&);
	void maybeRegisterMoveType(const MoveType& moveType);
};
