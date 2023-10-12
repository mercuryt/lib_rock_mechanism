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
class Faction;
class CanReserve;
class FindsPath
{
	const HasShape& m_hasShape;
	std::vector<Block*> m_route;
	std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>> m_moveCostsToCache;
	const Shape& m_shape;
	const MoveType& m_moveType;
public:
	bool m_detour;
	uint32_t m_maxRange;
	bool m_adjacent;
	Block* m_target;
	const Block* m_huristicDestination;
	FindsPath(const HasShape& hs);
	void depthFirstSearch(std::function<bool(const Block&, const Block&)>& isValid, std::function<bool(const ProposedRouteStep&, const ProposedRouteStep&)>& compare, std::function<bool(const Block&)>& isDone, std::function<std::vector<std::pair<Block*, uint32_t>>(Block&)>& adjacentCosts, Block& start);
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
	[[nodiscard]] std::vector<std::pair<Block*, uint32_t>> getMoveCosts(const Block& block);
	[[nodiscard]] Facing getFacingAtDestination() const;
	[[nodiscard]] std::vector<Block*> getAdjacentBlocksAtEndOfPath();
	[[nodiscard]] std::vector<Block*> getOccupiedBlocksAtEndOfPath();
	[[nodiscard]] std::vector<Block*>& getPath() { return m_route; }
	[[nodiscard]] bool found() const { return !m_route.empty(); }
	[[nodiscard]] bool areAllBlocksAtDestinationReservable(const Faction* faction) const;
	// For testing.
	[[maybe_unused, nodiscard]] std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>>& getMoveCostsToCache() { return m_moveCostsToCache; }
};
