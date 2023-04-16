/*
 * This class provides a thread task object, with read and write steps.
 * Read step finds a path in the route cache or generates one and stores it with any newly generated adjacentMoveCosts.
 * Write step assigns the path to the actor and to the actor.m_location->m_routeCache, stores newly generated adjacentMoveCosts in location, and schedules the first step on the new path.
 */
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

	static void readSteps(std::vector<RouteRequest>::iterator begin, std::vector<RouteRequest>::iterator end);

	RouteRequest(Actor* a, bool detour = false);
	
	// Find the route.
	void readStep();
	// Cache found route and move costs. Apply to actor and schedule first move step.
	void writeStep();
};
