//TODO: Consolidate duplicate requests: store multiple actors per request.

#include <queue>


RouteNode::RouteNode(Block* b, RouteNode* rn) : block(b), previous(rn) {}

ProposedRouteStep::ProposedRouteStep(RouteNode* rn, uint32_t tmc) : routeNode(rn),  totalMoveCost(tmc) {}

RouteRequest::RouteRequest(Actor* a) : m_actor(a) {}

void RouteRequest::readStep()
{
	Block* start = m_actor->m_location;
	Block* end = m_actor->m_destination;
	std::unordered_set<Block*> closed;
	closed.insert(start);
	// huristic: taxi distance to destination plus total move cost
	auto compare = [&](ProposedRouteStep& a, ProposedRouteStep& b)
	{
		return (a.routeNode->block->taxiDistance(end) * PATH_HURISTIC_CONSTANT) + a.totalMoveCost < (b.routeNode->block->taxiDistance(end) * PATH_HURISTIC_CONSTANT) + b.totalMoveCost;
	};
	std::vector<RouteNode> routeNodes;
	// A* algorithm.
	std::priority_queue<ProposedRouteStep, std::vector<ProposedRouteStep>, decltype(compare)> open(compare);
	routeNodes.emplace_back(start, nullptr);
	open.emplace(&routeNodes.back(), 0);
	while(!open.empty())
	{
		const ProposedRouteStep& proposedRouteStep = open.top();
		Block* block = proposedRouteStep.routeNode->block;
		if(block == end)
		{
			RouteNode* routeNode = proposedRouteStep.routeNode;
			// Route found, convert to stack.
			while(routeNode != nullptr)
			{
				m_result.push_back(routeNode->block);
				routeNode = routeNode->previous;
			}
			std::reverse(m_result.begin(), m_result.end());
			return;
		}
		std::vector<std::pair<Block*, uint32_t>> adjacentMoveCosts;
		// Access or generate adjacent move cost data.
		if(block->m_moveCostsCache.contains(m_actor->m_shape) && block->m_moveCostsCache[m_actor->m_shape].contains(m_actor->m_moveType))
			adjacentMoveCosts = block->m_moveCostsCache[m_actor->m_shape][m_actor->m_moveType];
		else
			m_moveCostsToCache[block] = adjacentMoveCosts = block->getMoveCosts(m_actor->m_shape, m_actor->m_moveType);
		for(auto& [adjacent, moveCost] : adjacentMoveCosts)
			if(!closed.contains(adjacent))
			{
				routeNodes.emplace_back(adjacent, proposedRouteStep.routeNode);
				open.emplace(&routeNodes.back(), moveCost + proposedRouteStep.totalMoveCost);
				closed.insert(adjacent);
			}
		open.pop();
	}
}

void RouteRequest::writeStep()
{
	// Cache move costs.
	for(auto& [block, moveCosts] : m_moveCostsToCache)
		block->m_moveCostsCache[m_actor->m_shape][m_actor->m_moveType] = std::move(moveCosts);
	// Notify if no path is found.
	if(m_result.empty())
	{
		m_actor->m_location->m_area->notifyNoRouteFound(m_actor);
		return;
	}
	// Clear invalid cached routes.
	if(m_actor->m_location->m_routeCacheVersion != m_actor->m_location->m_area->m_routeCacheVersion)
	{
		m_actor->m_location->m_routeCache.clear();
		m_actor->m_location->m_routeCacheVersion = m_actor->m_location->m_area->m_routeCacheVersion;
	}
	// Cache route in origin block.
	if(!m_actor->m_location->m_routeCache[m_actor->m_shape][m_actor->m_moveType].contains(m_actor->m_destination))
		m_actor->m_location->m_routeCache[m_actor->m_shape][m_actor->m_moveType][m_actor->m_destination] = std::make_shared<std::vector<Block*>>(m_result);
	// Store route in actor.
	m_actor->m_route = std::make_shared<std::vector<Block*>>(m_result);
	m_actor->m_routeIter = m_actor->m_route->begin();
	// Schedule a first move action with area.
	m_actor->m_location->m_area->scheduleMove(m_actor);
}
