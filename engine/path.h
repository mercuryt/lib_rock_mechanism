#pragma once

#include "block.h"
#include "actor.h"

#include <vector>
#include <unordered_set>
#include <functional>

namespace path
{
	// Depth first search for a given actor's movement.
	inline std::pair<std::vector<Block*>, std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>>> getForActor(const Actor& actor, const Block& destination, bool detour = false)
	{
		// Huristic: taxi distance to destination times constant plus total move cost.
		std::function<uint32_t(const ProposedRouteStep&)> priority = [&](const ProposedRouteStep& proposedRouteStep)
		{
			return (proposedRouteStep.routeNode->block.taxiDistance(destination) * Config::pathHuristicConstant) + proposedRouteStep.totalMoveCost;
		};
		std::function<bool(const ProposedRouteStep& a, const ProposedRouteStep& b)> compare = [&](const ProposedRouteStep& a, const ProposedRouteStep& b) { return priority(a) > priority(b); };
		std::function<bool(const Block&)> isDone = [&](const Block& block){ return &block == &destination; };
	       	std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>> moveCostsToCache;
		auto shape = *actor.m_shape;
		auto moveType = actor.m_canMove.getMoveType();
		std::function<std::vector<std::pair<Block*, uint32_t>>(Block&)> adjacentCosts = [&](Block& block)
		{
			assert(!moveCostsToCache.contains(&block));
			if(block.m_hasShapes.hasCachedMoveCosts(shape, moveType))
				return block.m_hasShapes.getCachedMoveCosts(shape, moveType);
			else
				return moveCostsToCache[&block] = block.m_hasShapes.makeMoveCosts(shape, moveType);
		};
		std::function<bool(const Block&, const Block&)> isValid;
		if(detour)
			// Check if the actor can currently enter each block if this is a detour path.
			isValid = [&](const Block& block, const Block& previous)
			{ 
				return block.m_hasShapes.anythingCanEnterEver() && block.m_hasShapes.canEnterEverFrom(actor, previous) && block.m_hasShapes.canEnterCurrentlyFrom(actor, previous);
			};
		else
			// Check if the actor can ever enter each block if this is not a detour path.
			isValid = [&](const Block& block, const Block& previous)
			{ 
				return block.m_hasShapes.anythingCanEnterEver() && block.m_hasShapes.canEnterEverFrom(actor, previous); 
			};
		std::vector<Block*> route = get(isValid, compare, isDone, adjacentCosts, *actor.m_location);
		return std::make_pair(std::move(route), moveCostsToCache);
	}
	// Breadth first search.
	// TODO: remove default argument on maxRange.
	// TODO: variant for multi block actors which checks each occupied block instead of just m_location.
	inline std::vector<Block*> getForActorToPredicate(const Actor& actor, std::function<bool(const Block& block, Facing facing)>& predicate, const uint32_t& maxRange = UINT32_MAX)
	{
		assert(!predicate(*actor.m_location, actor.m_facing));
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
				// Is in range.
				//TODO: Optimization: Use / generate cached adjacents and move costs.
				for(Block* adjacent : routeNode->block.getAdjacentWithEdgeAndCornerAdjacent())
				{
					if(!adjacent->m_hasShapes.anythingCanEnterEver() || locationClosedList.contains(adjacent))
						continue;
					locationClosedList.insert(adjacent);
					Facing facing = adjacent->facingToSetWhenEnteringFrom(routeNode->block);
					if(predicate(*adjacent, facing))
					{
						// Result found.
						output.push_back(adjacent);
						while(routeNode->previous != nullptr)
						{
							output.push_back(&routeNode->block);
							routeNode = routeNode->previous;
						}
						std::reverse(output.begin(), output.end());
						return output;
					}
					if(adjacent->m_hasShapes.canEnterEverWithFacing(actor, facing))
					{
						routeNodes.emplace_back(*adjacent, routeNode);
						openList.push_back(&routeNodes.back());
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
	inline std::vector<Block*> getForActorToExitArea(const Actor& actor, const uint32_t& maxRange = UINT32_MAX)
	{
		std::function<bool(const Block& block, Facing facing)> condition = [&](const Block& block, Facing facing) 
		{ 
			(void)facing;
			return block.m_isEdge; 
		};
		return path::getForActorToPredicate(actor, condition, maxRange);
	}
}
