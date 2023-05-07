#pragma once
#include <vector>
template<class DerivedBlock>
struct RouteNode
{

	DerivedBlock* block;
	RouteNode<DerivedBlock>* previous;
};
template<class DerivedBlock>
struct ProposedRouteStep
{
	RouteNode<DerivedBlock>* routeNode;
	uint32_t totalMoveCost;
};
template<class DerivedBlock, typename IsValidType, typename CompareType, typename IsDoneType, typename AdjacentCostsType>
class GetPath
{
public:
	std::unordered_set<DerivedBlock*> m_closed;
	GetPath(IsValidType& isValid, CompareType& compare, IsDoneType& isDone, AdjacentCostsType adjacentCosts, DerivedBlock* start, std::vector<DerivedBlock*>& output)
	{
		std::unordered_set<DerivedBlock*> closed;
		m_closed.insert(start);
		std::list<RouteNode<DerivedBlock>> routeNodes;
		std::priority_queue<ProposedRouteStep<DerivedBlock>, std::vector<ProposedRouteStep<DerivedBlock>>, decltype(compare)> open(compare);
		routeNodes.emplace_back(start, nullptr);
		open.emplace(&routeNodes.back(), 0);
		while(!open.empty())
		{
			ProposedRouteStep proposedRouteStep = open.top();
			open.pop();
			DerivedBlock* block = proposedRouteStep.routeNode->block;
			assert(block != nullptr);
			if(isDone(block))
			{
				RouteNode<DerivedBlock>* routeNode = proposedRouteStep.routeNode;
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
