#pragma once
#include <vector>
template<class Block>
struct RouteNode
{

	Block* block;
	RouteNode<Block>* previous;
};
template<class Block>
struct ProposedRouteStep
{
	RouteNode<Block>* routeNode;
	uint32_t totalMoveCost;
};
template<class Block, typename IsValidType, typename CompareType, typename IsDoneType, typename AdjacentCostsType>
class GetPath
{
public:
	std::unordered_set<Block*> m_closed;
	GetPath(IsValidType& isValid, CompareType& compare, IsDoneType& isDone, AdjacentCostsType adjacentCosts, Block* start, std::vector<Block*>& output)
	{
		std::unordered_set<Block*> closed;
		m_closed.insert(start);
		std::list<RouteNode<Block>> routeNodes;
		std::priority_queue<ProposedRouteStep<Block>, std::vector<ProposedRouteStep<Block>>, decltype(compare)> open(compare);
		routeNodes.emplace_back(start, nullptr);
		open.emplace(&routeNodes.back(), 0);
		while(!open.empty())
		{
			ProposedRouteStep proposedRouteStep = open.top();
			open.pop();
			Block* block = proposedRouteStep.routeNode->block;
			assert(block != nullptr);
			if(isDone(block))
			{
				RouteNode<Block>* routeNode = proposedRouteStep.routeNode;
				// Route found, push to output.
				// Exclude the starting point.
				while(routeNode->previous != nullptr)
				{
					output.push_back(routeNode->block);
					assert(routeNode->block != nullptr);
					routeNode = routeNode->previous;
				}
				std::reverse(output.begin(), output.end());
				return;
			}
			for(auto& [adjacent, moveCost] : adjacentCosts(block))
				if(isValid(adjacent) && !m_closed.contains(adjacent))
				{
					routeNodes.emplace_back(adjacent, proposedRouteStep.routeNode);
					open.emplace(&routeNodes.back(), moveCost + proposedRouteStep.totalMoveCost);
					m_closed.insert(adjacent);
				}
		}
	}
};
