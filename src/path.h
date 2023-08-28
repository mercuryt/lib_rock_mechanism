#pragma once

#include "block.h"
#include "actor.h"

#include <vector>
#include <unordered_set>

namespace path
{
	struct RouteNode
	{
		Block& block;
		const RouteNode* previous;
	};
	struct ProposedRouteStep
	{
		// Use pointer rather then reference so we can store in vector.
		RouteNode* routeNode;
		uint32_t totalMoveCost;
	};
	// Depth first search.
	template<typename IsValidType, typename CompareType, typename IsDoneType, typename AdjacentCostsType>
	std::vector<Block*> get(IsValidType& isValid, CompareType& compare, IsDoneType& isDone, AdjacentCostsType& adjacentCosts, Block& start)
	{
		std::unordered_set<const Block*> closed;
		closed.insert(&start);
		std::list<RouteNode> routeNodes;
		std::priority_queue<ProposedRouteStep, std::vector<ProposedRouteStep>, decltype(compare)> open(compare);
		routeNodes.emplace_back(start, nullptr);
		open.emplace(&routeNodes.back(), 0);
		std::vector<Block*> output;
		while(!open.empty())
		{
			ProposedRouteStep proposedRouteStep = open.top();
			open.pop();
			Block* block = &proposedRouteStep.routeNode->block;
			assert(block != nullptr);
			if(isDone(*block))
			{
				const RouteNode* routeNode = proposedRouteStep.routeNode;
				// Route found, push to output.
				// Exclude the starting point.
				while(routeNode->previous != nullptr)
				{
					output.push_back(&routeNode->block);
					routeNode = routeNode->previous;
				}
				std::reverse(output.begin(), output.end());
				return output;
			}
			for(auto& [adjacent, moveCost] : adjacentCosts(*block))
				if(isValid(*adjacent, *block) && !closed.contains(adjacent))
				{
					routeNodes.emplace_back(*adjacent, proposedRouteStep.routeNode);
					open.emplace(&routeNodes.back(), moveCost + proposedRouteStep.totalMoveCost);
					closed.insert(adjacent);
				}
		}
		return output; // Empty container means no result found.
	}
	// Depth first search for a given actor's movement.
	inline std::pair<std::vector<Block*>, std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>>> getForActor(const Actor& actor, const Block& destination, bool detour = false)
	{
		// Huristic: taxi distance to destination times constant plus total move cost.
		auto priority = [&](const ProposedRouteStep& proposedRouteStep)
		{
			return (proposedRouteStep.routeNode->block.taxiDistance(destination) * Config::pathHuristicConstant) + proposedRouteStep.totalMoveCost;
		};
		auto compare = [&](const ProposedRouteStep& a, const ProposedRouteStep& b) { return priority(a) > priority(b); };
		// Check if the actor can currently enter each block if this is a detour path.
		auto isDone = [&](const Block& block){ return &block == &destination; };
	       	std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>> moveCostsToCache;
		auto adjacentCosts = [&](Block& block)
		{
		       	return moveCostsToCache[&block] = block.m_hasShapes.getMoveCosts(*actor.m_shape, actor.m_canMove.getMoveType());
		};
		std::function<bool(const Block&, const Block&)> isValid;
		if(detour)
			isValid = [&](const Block& block, const Block& previous)
			{ 
				return block.m_hasShapes.anythingCanEnterEver() && block.m_hasShapes.canEnterEverFrom(actor, previous) && block.m_hasShapes.canEnterCurrentlyFrom(actor, previous);
			};
		else
			isValid = [&](const Block& block, const Block& previous)
			{ 
				return block.m_hasShapes.anythingCanEnterEver() && block.m_hasShapes.canEnterEverFrom(actor, previous); 
			};
		return std::make_pair(
				get<decltype(isValid), decltype(compare), decltype(isDone), decltype(adjacentCosts)>(isValid, compare, isDone, adjacentCosts, *actor.m_location),
				std::move(moveCostsToCache)
			);
	}
	// Breadth first search.
	// TODO: remove default argument on maxRange.
	// TODO: variant for multi block actors which checks each occupied block instead of just m_location.
	template<typename Predicate>
	std::vector<Block*> getForActorToPredicate(const Actor& actor, Predicate&& predicate, const uint32_t& maxRange = UINT32_MAX)
	{
		std::unordered_set<Block*> locationClosedList;
		locationClosedList.insert(actor.m_location);
		std::list<RouteNode*> openList;
		std::list<RouteNode> routeNodes;
		routeNodes.emplace_back(*actor.m_location, nullptr);
		openList.push_back(&routeNodes.back());
		std::vector<Block*> output;
		while(!openList.empty())
		{
			const RouteNode* routeNode = openList.front();
			if(maxRange >= actor.m_location->taxiDistance(routeNode->block))
			{
				if(predicate(routeNode->block))
				{
					// Result found.
					while(routeNode->previous != nullptr)
					{
						output.push_back(&routeNode->block);
						routeNode = routeNode->previous;
					}
					std::reverse(output.begin(), output.end());
					return output;
				}
				//TODO: Optimization: maintain an m_adjacentsWhichAnythingCanEnterEver vector.
				for(Block* adjacent : routeNode->block.m_adjacentsVector)
				{
					if(!adjacent->m_hasShapes.anythingCanEnterEver())
						continue;
					if(!locationClosedList.contains(adjacent))
					{
						locationClosedList.insert(adjacent);
						if(adjacent->m_hasShapes.canEnterEverFrom(actor, routeNode->block))
						{
							routeNodes.emplace_back(*adjacent, routeNode);
							openList.push_back(&routeNodes.back());
						}
					}
				}
			}
			openList.pop_front();
		}
		return output; // Empty container means no result found.
	}
	template<typename Predicate>
	Block* getForActorFromLocationToPredicateReturnEndOnly(const Actor& actor, Block& location, Predicate&& predicate, const uint32_t& maxRange = UINT32_MAX)
	{
		std::unordered_set<const Block*> closedList;
		closedList.insert(&location);
		std::list<RouteNode*> openList;
		std::list<RouteNode> routeNodes;
		routeNodes.emplace_back(location, nullptr);
		openList.push_back(&routeNodes.back());
		while(!openList.empty())
		{
			const RouteNode* routeNode = openList.front();
			if(maxRange >= location.taxiDistance(routeNode->block))
			{
				if(predicate(routeNode->block))
					return &routeNode->block;
				for(Block* adjacent : routeNode->block.m_adjacentsVector)
				{
					if(!adjacent->m_hasShapes.anythingCanEnterEver())
						continue;
					if(!closedList.contains(adjacent))
					{
						closedList.insert(adjacent);
						if(adjacent->m_hasShapes.canEnterEverFrom(actor, routeNode->block))
						{
							routeNodes.emplace_back(*adjacent, routeNode);
							openList.push_back(&routeNodes.back());
						}
					}
				}
			}
			openList.pop_front();
		}
		return nullptr;
	}
	template<typename Predicate>
		Block* getForActorToPredicateReturnEndOnly(const Actor& actor, Predicate&& predicate, const uint32_t& maxRange = UINT32_MAX)
		{
			return getForActorFromLocationToPredicateReturnEndOnly(actor, *actor.m_location, predicate, maxRange);
		}
}
