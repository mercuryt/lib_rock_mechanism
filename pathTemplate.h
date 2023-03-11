#include <vector>
class Block;
struct RouteNode
{

	Block* block;
	RouteNode* previous;
	//RouteNode(Block* b, RouteNode* p) : block(b), previous(p) {}
};
struct ProposedRouteStep
{
	RouteNode* routeNode;
	uint32_t totalMoveCost;
	//ProposedRouteStep(RouteNode* rn, uint32_t tmc) : routeNode(rn), totalMoveCost(tmc) {}
};
template<typename IsValidType, typename CompareType, typename IsDoneType, typename AdjacentCostsType>
void getPath(isValidType& isValid, CompareType& compare, IsDoneType& isDone, AdjacentCostsType adjacentCosts, Block* start, std::vector<Block*>& output)
{
	std::unordered_set<Block*> closed;
	closed.insert(start);
	std::list<RouteNode> routeNodes;
	std::priority_queue<ProposedRouteStep, std::vector<ProposedRouteStep>, decltype(compare)> open(compare);
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
			RouteNode* routeNode = proposedRouteStep.routeNode;
			// Route found, push to output.
			// Exclude the starting point.
			while(routeNode->previous != nullptr)
			{
				output.push_back(routeNode->block);
				assert(routeNode->block != nullptr);
				routeNode = routeNode->previous;
			}
			std::reverse(m_result.begin(), m_result.end());
			return;
		}
		for(auto& [adjacent, moveCost] : adjacentCosts(block))
			if(isValid(adjacent) && !closed.contains(adjacent))
			{
				routeNodes.emplace_back(adjacent, proposedRouteStep.routeNode);
				open.emplace(&routeNodes.back(), moveCost + proposedRouteStep.totalMoveCost);
				closed.insert(adjacent);
			}
	}
}
