/*
 * TerrainFacade:
 * Stores enterability data for a specific move type for pair of blocks in a binary format.
 * Also stores path requests in a series of vectors divided by presence or absence of destination huristic.
 * This divide is done for instruction cache locality.
 * TODO: try replacing std::function accessCondition and destinationCondition with a single findPath std::function member which calls a static class method in the class derived from PathRequest which calls a findPath template function.
 * 	
 */ 
#pragma once

#include "config.h"
#include "designations.h"
#include "index.h"
#include "pathMemo.h"
#include "reference.h"
#include "types.h"
#include "callbackTypes.h"
#include "dataVector.h"

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
	bool useCurrentPosition = false;
};
struct PathRequestNoHuristic
{
	FindPathResult result;
	AccessCondition accessCondition;
	DestinationCondition destinationCondition;
	BlockIndex startLocation;
	ActorReference actor;
	Facing startFacing;
	PathRequestNoHuristic() = default;
	PathRequestNoHuristic(AccessCondition& ac, DestinationCondition& dc, const BlockIndex& sl, const ActorReference& a, const Facing& sf) :
		accessCondition(ac), destinationCondition(dc), startLocation(sl), actor(a), startFacing(sf) { }
	PathRequestNoHuristic(const PathRequestNoHuristic& other) = default;
	PathRequestNoHuristic(PathRequestNoHuristic&& other) noexcept = default;
	PathRequestNoHuristic& operator=(const PathRequestNoHuristic& other) = default;
	PathRequestNoHuristic& operator=(PathRequestNoHuristic&& other) = default;
};
struct PathRequestWithHuristic
{
	FindPathResult result;
	AccessCondition accessCondition;
	DestinationCondition destinationCondition;
	BlockIndex startLocation;
	BlockIndex huristicDestination;
	ActorReference actor;
	Facing startFacing;
	PathRequestWithHuristic() = default;
	PathRequestWithHuristic(AccessCondition& ac, DestinationCondition& dc, const BlockIndex& sl, const BlockIndex& hd, const ActorReference& a, const Facing& sf) :
		accessCondition(ac), destinationCondition(dc), startLocation(sl), huristicDestination(hd), actor(a), startFacing(sf) { }
	PathRequestWithHuristic(const PathRequestWithHuristic& other) = default;
	PathRequestWithHuristic(PathRequestWithHuristic&& other) noexcept = default;
	PathRequestWithHuristic& operator=(const PathRequestWithHuristic& other) = default;
	PathRequestWithHuristic& operator=(PathRequestWithHuristic&& other) = default;
};
class TerrainFacade final
{
	DataVector<PathRequestNoHuristic, PathRequestIndex> m_pathRequestsNoHuristic;
	DataVector<PathRequestWithHuristic, PathRequestIndex> m_pathRequestsWithHuristic;
	// Not indexed by BlockIndex because size is mulipied by max adjacent.
	std::vector<bool> m_enterable;
	Area& m_area;
	MoveTypeId m_moveType;
	// DestinationCondition& could test against a set of destination indecies or load the actual block to do more complex checks.
	// AccessCondition& could test for larger shapes or just return true for 1x1x1 size.
	// TODO: huristic destination.
	[[nodiscard]] FindPathResult findPath(const BlockIndex& from, const Facing& startFacing, DestinationCondition& destinationCondition, AccessCondition& accessCondition, const OpenListPush& openListPush, const OpenListPop& openListPop, const OpenListEmpty& openListEmpty, const ClosedListAdd& closedListAdd, const ClosedListContains& closedListContains, const ClosedListGetPath& closedListGetPath) const;
	[[nodiscard]] FindPathResult findPathBreadthFirst(const BlockIndex& start, const Facing& startFacing, DestinationCondition& destinationCondition, AccessCondition& accessCondition, PathMemoBreadthFirst& memo) const;
	[[nodiscard]] FindPathResult findPathDepthFirst(const BlockIndex& start, const Facing& startFacing, DestinationCondition& destinationCondition, AccessCondition& accessCondition, const BlockIndex& huristicDestination, PathMemoDepthFirst& memo) const;
	// Non batched pathing uses that WithoutMemo variants.
	[[nodiscard]] FindPathResult findPathBreadthFirstWithoutMemo(const BlockIndex& start, const Facing& startFacing, DestinationCondition& destinationCondition, AccessCondition& accessCondition) const;
	[[nodiscard]] FindPathResult findPathDepthFirstWithoutMemo(const BlockIndex& from, const Facing& startFacing, DestinationCondition& destinationCondition, AccessCondition& accessCondition, const BlockIndex& huristicDestination) const;
	[[nodiscard]] FindPathResult findPathToForSingleBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToForMultiBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToAnyOfForSingleBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, BlockIndices indecies, const BlockIndex& huristincDestination, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToAnyOfForMultiBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, BlockIndices indecies, const BlockIndex& huristincDestination, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToConditionForSingleBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, DestinationCondition& destinationCondition, const BlockIndex& huristincDestination, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToConditionForMultiBlockShape(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, DestinationCondition& destinationCondition, const BlockIndex& huristincDestination, bool detour = false) const;
	PathRequestIndex getPathRequestIndexNoHuristic();
	PathRequestIndex getPathRequestIndexWithHuristic();
	void movePathRequestNoHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex);
	void movePathRequestWithHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex);
public:
	TerrainFacade(Area& area, const MoveTypeId& moveType);
	void doStep();
	void findPathWithHuristic(PathRequestWithHuristic& request, PathMemoDepthFirst& memo);
	void findPathNoHuristic(PathRequestNoHuristic& request, PathMemoBreadthFirst& memo);
	PathRequestIndex registerPathRequestNoHuristic(const BlockIndex& start, const Facing& startFacing, AccessCondition& acces, DestinationCondition& destination, PathRequest& pathRequest);
	PathRequestIndex registerPathRequestWithHuristic(const BlockIndex& start, const Facing& startFacing, AccessCondition& acces, DestinationCondition& destination, const BlockIndex& huristic, PathRequest& pathRequest);
	void unregisterNoHuristic(PathRequestIndex index);
	void unregisterWithHuristic(PathRequestIndex index);
	void update(const BlockIndex& block);
	void clearPathRequests();
	[[nodiscard]] bool canEnterFrom(const BlockIndex& blockIndex, AdjacentIndex adjacentIndex) const;
	[[nodiscard]] bool getValueForBit(const BlockIndex& from, const BlockIndex& to) const;
	[[nodiscard]] FindPathResult findPathTo(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToAnyOf(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, BlockIndices indecies, const BlockIndex huristicDestination = BlockIndex::null(), bool detour = false) const;
	[[nodiscard]] FindPathResult findPathToCondition(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, DestinationCondition& destinationCondition, BlockIndex huristicDestination = BlockIndex::null(), bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentTo(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToPolymorphic(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const ActorOrItemIndex& actorOrItem, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToAndUnreserved(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const BlockIndex& target, const FactionId& faction, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToAndUnreservedPolymorphic(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, const ActorOrItemIndex& actorOrItem, const FactionId& faction, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToCondition(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, DestinationCondition& destinationCondition, BlockIndex huristicDestination = BlockIndex::null(), bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToConditionAndUnreserved(const BlockIndex& start, const Facing& startFacing, const ShapeId& shape, DestinationCondition& destinationCondition, const FactionId& faction, BlockIndex huristicDestination = BlockIndex::null(), bool detour = false) const;
	[[nodiscard]] size_t getPathRequestCount() const { return m_pathRequestsNoHuristic.size() + m_pathRequestsWithHuristic.size(); }
	[[nodiscard]] AccessCondition makeAccessConditionForActor(const ActorIndex& actor, const BlockIndex& start, bool detour, const DistanceInBlocks maxRange = DistanceInBlocks::max()) const;
	[[nodiscard]] AccessCondition makeAccessCondition(const ShapeId& shape, const BlockIndex& start, const BlockIndices& initalBlocks, bool detour, const DistanceInBlocks maxRange = DistanceInBlocks::max()) const;
	// For testing.
	[[nodiscard]] bool accessable(const BlockIndex& from, const Facing& startFacing, const BlockIndex& to, const ActorIndex& actor) const;
	[[nodiscard]] bool empty() const { return m_pathRequestsNoHuristic.empty() && m_pathRequestsWithHuristic.empty(); }
};
class AreaHasTerrainFacades
{
	MoveTypeMap<TerrainFacade> m_data;
	Area& m_area;
	void update(const BlockIndex& block);
public:
	AreaHasTerrainFacades(Area& area) : m_area(area) { }
	void doStep();
	void updateBlockAndAdjacent(const BlockIndex& block);
	void maybeRegisterMoveType(const MoveTypeId& moveType);
	void clearPathRequests();
	[[nodiscard]] TerrainFacade& getForMoveType(const MoveTypeId& moveTypeId);
	[[nodiscard]] bool empty() const { for(const auto& pair : m_data) { if(!pair.second.empty()) return false; } return true; }
};