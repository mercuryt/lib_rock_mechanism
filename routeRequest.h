#pragma once
#include "shape.h"

class Block;
class RouteRequest
{
public:
	Actor* m_actor;
	bool m_detour;
	std::vector<Block*> m_result;
	std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>> m_moveCostsToCache;
	bool m_cacheHit;

	RouteRequest(Actor* a, bool detour);
	
	// Find the route.
	void readStep();
	// Cache found route and move costs. Apply to actor and schedule first move step.
	void writeStep();
};
