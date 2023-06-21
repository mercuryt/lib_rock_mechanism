#pragma once
#include <vector>
#include <unordered_set>

namespace path
{
	template<class Block>
	struct RouteNode
	{
		Block& block;
		RouteNode<Block>& previous;
	};
	template<class Block>
	struct ProposedRouteStep
	{
		RouteNode<Block>& routeNode;
		uint32_t totalMoveCost;
	};
	template<class Block, typename IsValidType, typename CompareType, typename IsDoneType, typename AdjacentCostsType>
	std::vector<Block*> get(IsValidType&& isValid, CompareType&& compare, IsDoneType&& isDone, AdjacentCostsType&& adjacentCosts, Block* start)
	{
		std::unordered_set<Block*> m_closed;
		std::unordered_set<Block*> closed;
		m_closed.insert(start);
		std::list<RouteNode<Block>> routeNodes;
		std::priority_queue<ProposedRouteStep<Block>, std::vector<ProposedRouteStep<Block>>, decltype(compare)> open(compare);
		routeNodes.emplace_back(start, nullptr);
		open.emplace(&routeNodes.back(), 0);
		std::vector<Block*> output;
		while(!open.empty())
		{
			ProposedRouteStep proposedRouteStep = open.top();
			open.pop();
			Block* block = proposedRouteStep.routeNode.block;
			assert(block != nullptr);
			if(isDone(block))
			{
				RouteNode<Block>* routeNode = proposedRouteStep.routeNode;
				// Route found, push to output.
				// Exclude the starting point.
				while(routeNode->previous != nullptr)
				{
					assert(routeNode->block != nullptr);
					output.push_back(routeNode->block);
					routeNode = routeNode->previous;
				}
				std::reverse(output.begin(), output.end());
				return output;
			}
			for(auto& [adjacent, moveCost] : adjacentCosts(block))
				if(isValid(adjacent) && !m_closed.contains(adjacent))
				{
					routeNodes.emplace_back(adjacent, proposedRouteStep.routeNode);
					open.emplace(&routeNodes.back(), moveCost + proposedRouteStep.totalMoveCost);
					m_closed.insert(adjacent);
				}
		}
		return output; // Empty container means no result found.
	}
	template<class Block, class Actor>
	std::vector<Block*> getForActor(Actor& actor, Block& destination, bool detour = false;)
	{
		// Huristic: taxi distance to destination times constant plus total move cost.
		auto priority = [&](ProposedRouteStep<Block>& proposedRouteStep)
		{
			return (proposedRouteStep.routeNode->block->taxiDistance(*end) * Config::pathHuristicConstant) + proposedRouteStep.totalMoveCost;
		};
		auto compare = [&](ProposedRouteStep<Block>& a, ProposedRouteStep<Block>& b) { return priority(a) > priority(b); };
		// Check if the actor can currently enter each block if this is a detour path.
		auto isDone = [&](Block* block){ return block == end; };
		std::vector<std::pair<Block*, uint32_t>> adjacentMoveCosts;
		auto adjacentCosts = [&](Block* block){
			if(block->m_moveCostsCache.contains(m_actor.m_shape) && block->m_moveCostsCache.at(m_actor.m_shape).contains(m_actor.m_moveType))
				adjacentMoveCosts = block->m_moveCostsCache.at(m_actor.m_shape).at(m_actor.m_moveType);
			else
				m_moveCostsToCache[block] = adjacentMoveCosts = block->getMoveCosts(*m_actor.m_shape, *m_actor.m_moveType);
			return adjacentMoveCosts;
		};
		if(m_detour)
		{
			auto isValid = [&](Block* block){ 
				return block->anyoneCanEnterEver() && block->canEnterEver(m_actor) && block->actorCanEnterCurrently(m_actor);
			};
			return get<Block, decltype(isValid), decltype(compare), decltype(isDone), decltype(adjacentCosts)> getPath(isValid, compare, isDone, adjacentCosts, start, m_result);
		}
		else
		{
			auto isValid = [&](Block* block){ return block->anyoneCanEnterEver() && block->canEnterEver(m_actor); };
			return get<Block, decltype(isValid), decltype(compare), decltype(isDone), decltype(adjacentCosts)> getPath(isValid, compare, isDone, adjacentCosts, start, m_result);
		}
	}
	template<class Block, class Actor, typename Predicate>
	std::vector<Block*> getForActorToPredicate(Actor& actor, Predicate&& predicate)
	{
		std::unordered_set<Block*> closedList;
		closedList.insert(actor.m_location);
		std::list<RouteNode*> openList;
		std::list<RouteNode<Block>> routeNodes;
		routeNodes.emplace_back(actor.m_location, nullptr);
		open.emplace(&routeNodes.back());
		std::vector<Block*> output;
		while(!openList.empty())
		{
			for(RouteNode<Block>* routeNode : openList)
			{
				if(predicate(routeNode->block))
				{
					// Result found.
					while(routeNode->previous != nullptr)
					{
						output.push_back(routeNode->block);
						routeNode = routeNode->previous;
					}
					std::reverse(output.begin(), output.end());
					return output;
				}
				for(Block* adjacent : routeNode->block->m_adjacentsVector)
				{
					if(!adjacent->anyoneCanEnterEver())
						continue;
					if(!closedList.contains(adjacent))
					{
						closedList.insert(adjacent);
						if(adjacent->canEnterEver(actor) && adjacent->canEnterFrom(actor, routeNode->block))
						{
							routeNodes.emplace_back(adjacent, routeNode);
							openList.push_back(&routeNodes.back());
						}
					}
				}
			}
		}
		return output; // Empty container means no result found.
	}
	template<class Block, class Actor, typename Predicate>
	Block* getForActorToPredicateReturnEndOnly(Actor& actor, Predicate&& predicate)
	{
		std::unordered_set<Block*> closedList;
		closedList.insert(actor.m_location);
		std::list<RouteNode*> openList;
		std::list<RouteNode<Block>> routeNodes;
		routeNodes.emplace_back(actor.m_location, nullptr);
		open.emplace(&routeNodes.back());
		while(!openList.empty())
		{
			for(RouteNode<Block>* routeNode : openList)
			{
				if(predicate(routeNode->block))
					return routeNode->block;
				}
				for(Block* adjacent : routeNode->block->m_adjacentsVector)
				{
					if(!adjacent->anyoneCanEnterEver())
						continue;
					if(!closedList.contains(adjacent))
					{
						closedList.insert(adjacent);
						if(adjacent->canEnterEver(actor) && adjacent->canEnterFrom(actor, routeNode->block))
						{
							routeNodes.emplace_back(adjacent, routeNode);
							openList.push_back(&routeNodes.back());
						}
					}
				}
			}
		}
		return nullptr;
	}
}
