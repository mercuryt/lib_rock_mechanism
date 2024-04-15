#include "hasShape.h"
#include "block.h"
#include <utility>
struct RouteNode
{
	Block& block;
	const RouteNode* previous;
};
struct ProposedRouteStep
{
	// Use pointer rather then reference so we can store in vector.
	RouteNode* routeNode;
	MoveCost totalMoveCost;
};
/*
// Only to be used when pathing with turn rate restrictions.
struct ClosedList
{
	std::unordered_map<const Block*, std::array<bool, 4>> blocksAndFacings;
	// Returns false if combination of block and facing exist already.
	bool insert(const Block& block, Facing facing)
	{
		auto found = blocksAndFacings.find(&block);
		std::array<bool, 4>& facings = found == blocksAndFacings.end() ?
			(blocksAndFacings[&block] = {false, false, false, false}) :
			found->second;
		if(facings.at(facing))
			return false;
		facings[facing] = true;
		return true;
	}
};
*/
FindsPath::FindsPath(const HasShape& hs, bool detour) : m_hasShape(hs), m_target(nullptr), m_detour(detour), m_useCurrentLocation(false), m_huristicDestination(nullptr), m_maxRange(UINT32_MAX) { }
/*
// Depth first search.
void FindsPath::depthFirstSearch(std::function<bool(const Block&, const Block&)>& isValid, std::function<bool(const ProposedRouteStep&, const ProposedRouteStep&)>& compare, std::function<bool(const Block&)>& isDone, std::function<std::vector<std::pair<Block*, MoveCost>>(Block&)>& adjacentCosts, Block& start)
{
	assert(m_route.empty());
	std::unordered_set<const Block*> closed;
	closed.insert(&start);
	std::list<RouteNode> routeNodes;
	std::priority_queue<ProposedRouteStep, std::vector<ProposedRouteStep>, decltype(compare)> open(compare);
	routeNodes.emplace_back(start, nullptr);
	open.emplace(&routeNodes.back(), 0u);
	while(!open.empty())
	{
		const ProposedRouteStep& proposedRouteStep = open.top();
		const RouteNode& routeNode = *proposedRouteStep.routeNode;
		const Block& block = routeNode.block;
		MoveCost totalMoveCost = proposedRouteStep.totalMoveCost;
		open.pop();
		for(auto [adjacent, moveCost] : adjacentCosts(const_cast<Block&>(block)))
			if(isValid(*adjacent, block) && !closed.contains(adjacent))
			{
				if(isDone(*adjacent))
				{
					const RouteNode* routeNode = proposedRouteStep.routeNode;
					// Route found, push to output.
					// Exclude the starting point.
					m_route.push_back(adjacent);
					while(routeNode->previous != nullptr)
					{
						m_route.push_back(&routeNode->block);
						routeNode = routeNode->previous;
					}
					std::reverse(m_route.begin(), m_route.end());
					return;
				}
				routeNodes.emplace_back(*adjacent, proposedRouteStep.routeNode);
				open.emplace(&routeNodes.back(), moveCost + totalMoveCost);
				closed.insert(adjacent);
			}
	}
}
*/
void FindsPath::pathToBlock(const Block& destination)
{
	std::function<bool(const Block&, Facing)> predicate = [&](const Block& location, [[maybe_unused]] Facing facing) { return location == destination; };
	pathToPredicateWithHuristicDestination(predicate, destination);
}
void FindsPath::pathAdjacentToBlock(const Block& target)
{
	std::function<bool(const Block&, Facing facing)> predicate = [&](const Block& location, Facing facing)
	{
		std::function<bool(const Block&)> isTarget = [&](const Block& adjacent) { return adjacent == target; };
		Block* adjacent = const_cast<HasShape&>(m_hasShape).getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, isTarget);
		if(adjacent == nullptr)
			return false;
		m_target = adjacent;
		return true;
	};
	pathToPredicateWithHuristicDestination(predicate, target);
}
void FindsPath::pathToPredicate(std::function<bool(const Block&, Facing facing)>& predicate)
{
	assert(m_route.empty());
	std::unordered_set<Block*> closedList;
	closedList.insert(m_hasShape.m_location);
	std::list<RouteNode*> openList;
	std::list<RouteNode> routeNodes;
	routeNodes.emplace_back(*m_hasShape.m_location, nullptr);
	openList.push_back(&routeNodes.back());
	while(!openList.empty())
	{
		const RouteNode* routeNode = openList.front();
		openList.pop_front();
		if(m_maxRange >= m_hasShape.m_location->taxiDistance(routeNode->block))
		{
			// Is in range.
			for(auto [block, moveCost] : getMoveCosts(routeNode->block))
			{
				// Move cost is used with depth first search but not breadth first search.
				(void)moveCost;
				if(closedList.contains(block))
					continue;
				closedList.insert(block);
				Facing facing = block->facingToSetWhenEnteringFrom(routeNode->block);
				if(m_detour && !block->m_hasShapes.canEnterCurrentlyWithFacing(m_hasShape, facing))
					continue;
				if(predicate(*block, facing))
				{
					// Destination found.
					m_route.push_back(const_cast<Block*>(block));
					while(routeNode->previous != nullptr)
					{
						m_route.push_back(&routeNode->block);
						routeNode = routeNode->previous;
					}
					std::reverse(m_route.begin(), m_route.end());
					if(m_route.empty())
						m_useCurrentLocation = true;
					return;
				}
				// Block is not destination, and either this isn't a detour request or block can be entered currently, add it to openList.
				routeNodes.emplace_back(*block, routeNode);
				openList.push_back(&routeNodes.back());
			}
		}
	}
}
void FindsPath::pathToAdjacentToPredicate(std::function<bool(const Block&)>& predicate)
{
	std::function<bool(const Block&, Facing)> condition = [&](const Block& location, Facing facing)
	{
		Block* adjacent = const_cast<HasShape&>(m_hasShape).getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate);
		if(adjacent == nullptr)
			return false;
		m_target = adjacent;
		return true;
	};
	if(m_huristicDestination == nullptr)
		pathToPredicate(condition);
	else
		pathToPredicateWithHuristicDestination(condition, *m_huristicDestination);
}
void FindsPath::pathToOccupiedPredicate(std::function<bool(const Block&)>& predicate)
{
	std::function<bool(const Block&, Facing)> condition = [&](const Block& location, Facing facing)
	{
		Block* adjacent = const_cast<HasShape&>(m_hasShape).getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(location, facing, predicate);
		if(adjacent == nullptr)
			return false;
		m_target = adjacent;
		return true;
	};
	if(m_huristicDestination == nullptr)
		pathToPredicate(condition);
	else
		pathToPredicateWithHuristicDestination(condition, *m_huristicDestination);
}
void FindsPath::pathToUnreservedAdjacentToPredicate(std::function<bool(const Block&)>& predicate, const Faction& faction)
{
	std::function<bool(const Block&, Facing)> condition = [&](const Block& location, Facing facing)
	{
		if(!m_hasShape.allBlocksAtLocationAndFacingAreReservable(location, facing, faction))
			return false;
		Block* adjacent = const_cast<HasShape&>(m_hasShape).getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate);
		if(adjacent == nullptr)
			return false;
		m_target = adjacent;
		return true;
	};
	if(m_huristicDestination == nullptr)
		pathToPredicate(condition);
	else
		pathToPredicateWithHuristicDestination(condition, *m_huristicDestination);
}
void FindsPath::pathToUnreservedPredicate(std::function<bool(const Block&)>& predicate, const Faction& faction)
{
	std::function<bool(const Block&, Facing)> condition = [&](const Block& location, Facing facing)
	{
		if(!m_hasShape.allBlocksAtLocationAndFacingAreReservable(location, facing, faction))
			return false;
		Block* occupied = const_cast<HasShape&>(m_hasShape).getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(location, facing, predicate);
		return occupied != nullptr;
	};
	if(m_huristicDestination == nullptr)
		pathToPredicate(condition);
	else
		pathToPredicateWithHuristicDestination(condition, *m_huristicDestination);
}
void FindsPath::pathToUnreservedAdjacentToHasShape(const HasShape& hasShape, const Faction& faction)
{
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return hasShape.m_blocks.contains(const_cast<Block*>(&block)); };
	pathToUnreservedAdjacentToPredicate(predicate, faction);
}
//TODO: Would template / lambda be faster then std::function? Do we need to store it somewhere?
void FindsPath::pathToPredicateWithHuristicDestination(std::function<bool(const Block&, Facing facing)>& predicate, const Block& huristicDestination)
{
	assert(m_route.empty());
	std::unordered_set<const Block*> closedList;
	closedList.insert(m_hasShape.m_location);
	// Huristic: taxi distance to destination times constant plus total move cost.
	std::function<uint32_t(const ProposedRouteStep&)> priority = [&](const ProposedRouteStep& proposedRouteStep)
	{
		return (proposedRouteStep.routeNode->block.taxiDistance(huristicDestination) * Config::pathHuristicConstant) + proposedRouteStep.totalMoveCost;
	};
	// Order by ProposedRouteStep::priority.
	std::function<bool(const ProposedRouteStep& a, const ProposedRouteStep& b)> compare = [&](const ProposedRouteStep& a, const ProposedRouteStep& b) { return priority(a) > priority(b); };
	std::priority_queue<ProposedRouteStep, std::vector<ProposedRouteStep>, decltype(compare)> openList(compare);
	std::list<RouteNode> routeNodes;
	routeNodes.emplace_back(*m_hasShape.m_location, nullptr);
	openList.emplace(&routeNodes.back(), 0u);
	while(!openList.empty())
	{
		const ProposedRouteStep& proposedRouteStep = openList.top();
		MoveCost totalMoveCost = proposedRouteStep.totalMoveCost;
		const RouteNode* routeNode = proposedRouteStep.routeNode;
		const Block& block = routeNode->block;
		openList.pop();
		//TODO: Swap this if with the below for to prevent out of range blocks being added to the open list.
		if(m_maxRange >= m_hasShape.m_location->taxiDistance(block))
		{
			// Is in range.
			for(auto [adjacent, moveCost] : getMoveCosts(block))
			{
				if(closedList.contains(adjacent))
					continue;
				closedList.insert(adjacent);
				// TODO: Hot path for shapes with radial symetry which disregards facing.
				Facing facing = adjacent->facingToSetWhenEnteringFrom(block);
				if(m_detour && !adjacent->m_hasShapes.canEnterCurrentlyWithFacing(m_hasShape, facing))
					continue;
				if(predicate(*adjacent, facing))
				{
					// Destination found.
					m_route.push_back(const_cast<Block*>(adjacent));
					while(routeNode->previous != nullptr)
					{
						m_route.push_back(&routeNode->block);
						routeNode = routeNode->previous;
					}
					std::reverse(m_route.begin(), m_route.end());
					if(m_route.empty())
						m_useCurrentLocation = true;
					return;
				}
				// Adjacent is not destination, and either this isn't a detour request or block can be entered currently, add it to openList.
				routeNodes.emplace_back(*adjacent, routeNode);
				openList.emplace(&routeNodes.back(), moveCost + totalMoveCost);
			}
		}
	}
}
void FindsPath::pathToAreaEdge()
{
	std::function<bool(const Block&, Facing)> predicate = [&](const Block& location, Facing facing)
	{
		for(Block* occupied : const_cast<HasShape&>(m_hasShape).getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const_cast<Block&>(location), facing))
			if(occupied->m_isEdge)
				return true;
		return false;
	};
	pathToPredicate(predicate);
}
std::vector<std::pair<Block*, MoveCost>> FindsPath::getMoveCosts(const Block& block)
{
	if(block.m_hasShapes.hasCachedMoveCosts(*m_hasShape.m_shape, m_hasShape.getMoveType()))
		return block.m_hasShapes.getCachedMoveCosts(*m_hasShape.m_shape, m_hasShape.getMoveType());
	// TODO: Only check if cache contains when re-entering, otherwise assert that it does not.
	else if(m_moveCostsToCache.contains(const_cast<Block*>(&block)))
		return m_moveCostsToCache[const_cast<Block*>(&block)];
	else
		return m_moveCostsToCache[const_cast<Block*>(&block)] = block.m_hasShapes.makeMoveCosts(*m_hasShape.m_shape, m_hasShape.getMoveType());
}
void FindsPath::cacheMoveCosts()
{
	for(auto& pair : m_moveCostsToCache)
		pair.first->m_hasShapes.tryToCacheMoveCosts(*m_hasShape.m_shape, m_hasShape.getMoveType(), pair.second);
}
std::vector<Block*> FindsPath::getOccupiedBlocksAtEndOfPath()
{
	return const_cast<HasShape&>(m_hasShape).getBlocksWhichWouldBeOccupiedAtLocationAndFacing(*m_route.back(), getFacingAtDestination());
}
std::vector<Block*> FindsPath::getAdjacentBlocksAtEndOfPath()
{
	return const_cast<HasShape&>(m_hasShape).getAdjacentAtLocationWithFacing(*m_route.back(), getFacingAtDestination());
}
Facing FindsPath::getFacingAtDestination() const
{
	assert(!m_route.empty());
	return m_route.back()->facingToSetWhenEnteringFrom(m_route.size() == 1 ? *m_hasShape.m_location : *m_route.at(m_route.size() - 1));
}
bool FindsPath::areAllBlocksAtDestinationReservable(const Faction* faction) const
{
	if(faction ==  nullptr)
		return true;
	if(m_route.empty())
	{
		assert(m_useCurrentLocation);
		return m_hasShape.allBlocksAtLocationAndFacingAreReservable(*m_hasShape.m_location, m_hasShape.m_facing, *faction);
	}
	return m_hasShape.allBlocksAtLocationAndFacingAreReservable(*m_route.back(), getFacingAtDestination(), *faction);
}
void FindsPath::reserveBlocksAtDestination(CanReserve& canReserve)
{
	if(!canReserve.hasFaction())
		return;
	for(Block* occupied : getOccupiedBlocksAtEndOfPath())
		occupied->m_reservable.reserveFor(canReserve, 1u);
}
void FindsPath::reset()
{
	m_target = nullptr;
	m_route.clear();
}
