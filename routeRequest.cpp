#include "pathTemplate.h"
RouteRequest::RouteRequest(Actor* a) : m_actor(a), m_cacheHit(false) {}
void RouteRequest::readStep()
{
	Block* start = m_actor->m_location;
	Block* end = m_actor->m_destination;
	assert(start != nullptr);
	assert(end != nullptr);
	//TODO: Would it be better to check for cache hit before fork?
	if(start->m_routeCache.contains(m_actor->m_shape) && start->m_routeCache.at(m_actor->m_shape).contains(m_actor->m_moveType) &&
			start->m_routeCache.at(m_actor->m_shape).at(m_actor->m_moveType).contains(end))
	{
		m_cacheHit = true;
		return;
	}
	// Huristic: taxi distance to destination times constant plus total move cost.
	auto priority = [&](ProposedRouteStep& proposedRouteStep)
	{
		return (proposedRouteStep.routeNode->block->taxiDistance(end) * s_pathHuristicConstant) + proposedRouteStep.totalMoveCost;
	};
	auto compare = [&](ProposedRouteStep& a, ProposedRouteStep& b) { return priority(a) > priority(b); };
	auto isValid = [&](Block* block){ return block->anyoneCanEnterEver() && block->canEnterEver(m_actor); };
	auto isDone = [&](Block* block){ return block == end; };
	std::vector<std::pair<Block*, uint32_t>> adjacentMoveCosts;
	auto adjacentCosts = [&](Block* block){
		if(block->m_moveCostsCache.contains(m_actor->m_shape) && block->m_moveCostsCache[m_actor->m_shape].contains(m_actor->m_moveType))
			adjacentMoveCosts = block->m_moveCostsCache[m_actor->m_shape][m_actor->m_moveType];
		else
			m_moveCostsToCache[block] = adjacentMoveCosts = block->getMoveCosts(m_actor->m_shape, m_actor->m_moveType);
		return adjacentMoveCosts;
	};
	GetPath getPath(isValid, compare, isDone, adjacentCosts, start, m_result);
}
void RouteRequest::writeStep()
{
	if(m_cacheHit)
	{
		m_actor->m_route = m_actor->m_location->m_routeCache.at(m_actor->m_shape).at(m_actor->m_moveType).at(m_actor->m_destination);
	}
	else
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
	}
	m_actor->m_routeIter = m_actor->m_route->begin();
	// Schedule a first move action with area.
	m_actor->m_location->m_area->scheduleMove(m_actor);
}
