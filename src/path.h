#pragma once
#include <vector>
#include <unordered_set>

#include "block.h"

namespace path
{
	struct RouteNode
	{
		Block& block;
		RouteNode* previous;
	};
	struct ProposedRouteStep
	{
		// Use pointer rather then reference so we can store in vector.
		RouteNode* routeNode;
		uint32_t totalMoveCost;
	};
	template<typename IsValidType, typename CompareType, typename IsDoneType, typename AdjacentCostsType>
	std::vector<Block*> get(IsValidType& isValid, CompareType& compare, IsDoneType& isDone, AdjacentCostsType& adjacentCosts, Block& start)
	{
		std::unordered_set<Block*> closed;
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
				RouteNode* routeNode = proposedRouteStep.routeNode;
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
	std::vector<Block*> getForActor(Actor& actor, Block& destination, bool detour = false)
	{
		// Huristic: taxi distance to destination times constant plus total move cost.
		auto priority = [&](ProposedRouteStep& proposedRouteStep)
		{
			return (proposedRouteStep.routeNode->block.taxiDistance(destination) * Config::pathHuristicConstant) + proposedRouteStep.totalMoveCost;
		};
		auto compare = [&](ProposedRouteStep& a, ProposedRouteStep& b) { return priority(a) > priority(b); };
		// Check if the actor can currently enter each block if this is a detour path.
		auto isDone = [&](Block& block){ return &block == &destination; };
		auto adjacentCosts = [&](Block& block){ return block.m_hasShapes.getMoveCosts(*actor.m_shape, actor.m_canMove.getMoveType()); };
		std::function<bool(Block&, Block&)> isValid;
		if(detour)
			isValid = [&](Block& block, Block& previous){ 
				return block.m_hasShapes.anythingCanEnterEver() && block.m_hasShapes.canEnterEverFrom(actor, previous) && block.m_hasShapes.canEnterCurrentlyFrom(actor, previous);
			};
		else
			isValid = [&](Block& block, Block& previous){ return block.m_hasShapes.anythingCanEnterEver() && block.m_hasShapes.canEnterEverFrom(actor, previous); };
		return get<decltype(isValid), decltype(compare), decltype(isDone), decltype(adjacentCosts)>(isValid, compare, isDone, adjacentCosts, *actor.m_location);
	}
	// Depth first search.
	// TODO: remove default argument on maxRange.
	template<typename Predicate>
	std::vector<Block*> getForActorToPredicate(Actor& actor, Predicate&& predicate, const uint32_t& maxRange = UINT32_MAX)
	{
		std::unordered_set<Block*> closedList;
		closedList.insert(actor.m_location);
		std::list<RouteNode*> openList;
		std::list<RouteNode> routeNodes;
		routeNodes.emplace_back(*actor.m_location, nullptr);
		openList.push_back(&routeNodes.back());
		std::vector<Block*> output;
		while(!openList.empty())
		{
			for(RouteNode* routeNode : openList)
			{
				if(maxRange > actor.m_location->taxiDistance(routeNode->block))
					continue;
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
		}
		return output; // Empty container means no result found.
	}
	template<typename Predicate>
	Block* getForActorToPredicateReturnEndOnly(Actor& actor, Predicate&& predicate, const uint32_t& maxRange = UINT32_MAX)
	{
		std::unordered_set<Block*> closedList;
		closedList.insert(actor.m_location);
		std::list<RouteNode*> openList;
		std::list<RouteNode> routeNodes;
		routeNodes.emplace_back(*actor.m_location, nullptr);
		openList.push_back(&routeNodes.back());
		while(!openList.empty())
		{
			for(RouteNode* routeNode : openList)
			{
				if(maxRange > actor.m_location->taxiDistance(routeNode->block))
					continue;
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
		}
		return nullptr;
	}
}
