//TODO: Consolidate duplicate requests: store multiple actors per request.

#include <queue>


ProposedRouteNode::ProposedRouteNode(Block* b, ProposedRouteNode* p, uint32_t tmc) : m_block(b), m_previous(p), m_totalMoveCost(tmc) {}

RouteRequest::RouteRequest(Actor* a) : m_actor(a) {}

void RouteRequest::readStep()
{
	Block* start = m_actor->m_location;
	Block* end = m_actor->m_destination;
	std::unordered_set<Block*> closed;
	closed.insert(start);
	// huristic: taxi distance to destination plus total move cost
	auto compare = [&](ProposedRouteNode& a, ProposedRouteNode& b)
	{
		return (a.m_block->taxiDistance(end) * PATH_HURISTIC_CONSTANT) + a.m_totalMoveCost < (b.m_block->taxiDistance(end) * PATH_HURISTIC_CONSTANT) + b.m_totalMoveCost;
	};
	// A* algorithm.
	std::priority_queue<ProposedRouteNode, std::vector<ProposedRouteNode>, decltype(compare)> open(compare);
	open.emplace(start, nullptr, 0);
	while(!open.empty())
	{
		const ProposedRouteNode* proposedRouteNode = &open.top();
		Block* block = proposedRouteNode->m_block;
		if(block == end)
		{
			// Route found, convert to stack.
			while(proposedRouteNode != nullptr)
			{
				m_result.push_back(proposedRouteNode->m_block);
				proposedRouteNode = proposedRouteNode->m_previous;
			}
			return;
		}
		std::vector<std::pair<Block*, uint32_t>> adjacentMoveCosts;
		// Access or generate adjacent move cost data.
		if(block->m_moveCostsCache.contains(m_actor->m_shape) && block->m_moveCostsCache[m_actor->m_shape].contains(m_actor->m_moveType))
			adjacentMoveCosts = block->m_moveCostsCache[m_actor->m_shape][m_actor->m_moveType];
		else
			m_moveCostsToCache[block] = adjacentMoveCosts = block->getMoveCosts(m_actor->m_shape, m_actor->m_moveType);
		for(auto& [block, moveCost] : adjacentMoveCosts)
			if(!closed.contains(block))
			{
				open.emplace(block, proposedRouteNode, moveCost + proposedRouteNode->m_totalMoveCost);
				closed.insert(block);
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
	// Register as moving with area and schedule a first move action.
	m_actor->m_location->m_area->registerActorMoving(m_actor);
}
