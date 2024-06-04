#pragma once
#include "types.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
class HasShape;
class Shape;
class MoveType;
struct ProposedRouteStep;
struct Faction;
class CanReserve;
//TODO: Preseed closed list with currently unenterable tiles which are occupied by followers.
class FindsPath
{
	const HasShape& m_hasShape;
	std::vector<BlockIndex> m_route;
	std::unordered_map<BlockIndex, std::vector<std::pair<BlockIndex, MoveCost>>> m_moveCostsToCache;
	BlockIndex m_target = BLOCK_INDEX_MAX;
	bool m_detour = false;
public:
	bool m_useCurrentLocation = false;
	BlockIndex m_huristicDestination = BLOCK_INDEX_MAX;
	DistanceInBlocks m_maxRange = BLOCK_DISTANCE_MAX;
	FindsPath(const HasShape& hs, bool detour);
	void depthFirstSearch(std::function<bool(BlockIndex, BlockIndex)>& isValid, std::function<bool(const ProposedRouteStep&, const ProposedRouteStep&)>& compare, std::function<bool(BlockIndex)>& isDone, std::function<std::vector<std::pair<BlockIndex, MoveCost>>(BlockIndex)>& adjacentCosts, BlockIndex start);
	void pathToBlock(BlockIndex destination);
	void pathAdjacentToBlock(BlockIndex destination);
	void pathToPredicate(DestinationCondition& predicate);
	void pathToAdjacentToPredicate(std::function<bool(BlockIndex)>& predicate);
	void pathToOccupiedPredicate(std::function<bool(BlockIndex)>& predicate);
	void pathToUnreservedAdjacentToPredicate(std::function<bool(BlockIndex)>& predicate, Faction& faction);
	void pathToUnreservedPredicate(std::function<bool(BlockIndex)>& predicate, Faction& faction);
	void pathToUnreservedAdjacentToHasShape(const HasShape& hasShape, Faction& faction);
	void pathToPredicateWithHuristicDestination(DestinationCondition& predicate, BlockIndex huristicDestination);
	void pathToAreaEdge();
	void reserveBlocksAtDestination(CanReserve& canReserve);
	void reset();
	//TODO: make return reference and assert found().
	[[nodiscard]] BlockIndex getBlockWhichPassedPredicate() { return m_target; }
	[[nodiscard]] std::vector<std::pair<BlockIndex, MoveCost>> getMoveCosts(BlockIndex block);
	[[nodiscard]] Facing getFacingAtDestination() const;
	[[nodiscard]] std::vector<BlockIndex> getAdjacentBlocksAtEndOfPath();
	[[nodiscard]] std::vector<BlockIndex> getOccupiedBlocksAtEndOfPath();
	[[nodiscard]] std::vector<BlockIndex>& getPath() { return m_route; }
	[[nodiscard]] bool found() const { return !m_route.empty(); }
	[[nodiscard]] bool areAllBlocksAtDestinationReservable(Faction* faction) const;
	// For testing.
	[[maybe_unused, nodiscard]] std::unordered_map<BlockIndex, std::vector<std::pair<BlockIndex, MoveCost>>>& getMoveCostsToCache() { return m_moveCostsToCache; }
};
