#pragma once
#include "types.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
class Block;
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
	std::vector<Block*> m_route;
	std::unordered_map<Block*, std::vector<std::pair<Block*, MoveCost>>> m_moveCostsToCache;
	Block* m_target;
	bool m_detour;
public:
	bool m_useCurrentLocation;
	const Block* m_huristicDestination;
	DistanceInBlocks m_maxRange;
	FindsPath(const HasShape& hs, bool detour);
	void depthFirstSearch(std::function<bool(const Block&, const Block&)>& isValid, std::function<bool(const ProposedRouteStep&, const ProposedRouteStep&)>& compare, std::function<bool(const Block&)>& isDone, std::function<std::vector<std::pair<Block*, MoveCost>>(Block&)>& adjacentCosts, Block& start);
	void pathToBlock(const Block& destination);
	void pathAdjacentToBlock(const Block& destination);
	void pathToPredicate(std::function<bool(const Block&, Facing facing)>& predicate);
	void pathToAdjacentToPredicate(std::function<bool(const Block&)>& predicate);
	void pathToOccupiedPredicate(std::function<bool(const Block&)>& predicate);
	void pathToUnreservedAdjacentToPredicate(std::function<bool(const Block&)>& predicate, const Faction& faction);
	void pathToUnreservedPredicate(std::function<bool(const Block&)>& predicate, const Faction& faction);
	void pathToUnreservedAdjacentToHasShape(const HasShape& hasShape, const Faction& faction);
	void pathToPredicateWithHuristicDestination(std::function<bool(const Block&, Facing facing)>& predicate, const Block& huristicDestination);
	void pathToAreaEdge();
	void cacheMoveCosts();
	void reserveBlocksAtDestination(CanReserve& canReserve);
	void reset();
	//TODO: make return reference and assert found().
	[[nodiscard]] Block* getBlockWhichPassedPredicate() { return m_target; }
	[[nodiscard]] std::vector<std::pair<Block*, MoveCost>> getMoveCosts(const Block& block);
	[[nodiscard]] Facing getFacingAtDestination() const;
	[[nodiscard]] std::vector<Block*> getAdjacentBlocksAtEndOfPath();
	[[nodiscard]] std::vector<Block*> getOccupiedBlocksAtEndOfPath();
	[[nodiscard]] std::vector<Block*>& getPath() { return m_route; }
	[[nodiscard]] bool found() const { return !m_route.empty(); }
	[[nodiscard]] bool areAllBlocksAtDestinationReservable(const Faction* faction) const;
	// For testing.
	[[maybe_unused, nodiscard]] std::unordered_map<Block*, std::vector<std::pair<Block*, MoveCost>>>& getMoveCostsToCache() { return m_moveCostsToCache; }
};
