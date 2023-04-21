#pragma once
#include <vector>
class BLOCK;
struct RouteNode
{

	BLOCK* block;
	RouteNode* previous;
	//RouteNode(BLOCK* b, RouteNode* p) : block(b), previous(p) {}
};
struct ProposedRouteStep
{
	RouteNode* routeNode;
	uint32_t totalMoveCost;
	//ProposedRouteStep(RouteNode* rn, uint32_t tmc) : routeNode(rn), totalMoveCost(tmc) {}
};
template<typename IsValidType, typename CompareType, typename IsDoneType, typename AdjacentCostsType>
class GetPath
{
public:
	std::unordered_set<BLOCK*> m_closed;
	GetPath(IsValidType& isValid, CompareType& compare, IsDoneType& isDone, AdjacentCostsType adjacentCosts, BLOCK* start, std::vector<BLOCK*>& output)
	{
		std::unordered_set<BLOCK*> closed;
		m_closed.insert(start);
		std::list<RouteNode> routeNodes;
		std::priority_queue<ProposedRouteStep, std::vector<ProposedRouteStep>, decltype(compare)> open(compare);
		routeNodes.emplace_back(start, nullptr);
		open.emplace(&routeNodes.back(), 0);
		while(!open.empty())
		{
			ProposedRouteStep proposedRouteStep = open.top();
			open.pop();
			BLOCK* block = proposedRouteStep.routeNode->block;
			assert(block != nullptr);
			if(isDone(block))
			{
				RouteNode* routeNode = proposedRouteStep.routeNode;
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
