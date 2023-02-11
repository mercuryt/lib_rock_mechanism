#pragma once
#include "shape.h"

class Block;

struct ProposedRouteNode
{
	Block* m_block;
	ProposedRouteNode* m_previous;
	uint32_t m_totalMoveCost;
	ProposedRouteNode(Block* b, ProposedRouteNode* p, uint32_t tmc);
};

class RouteRequest
{
public:
	Actor* m_actor;
	std::vector<Block*> m_result;
	std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>> m_moveCostsToCache;

	RouteRequest(Actor* a);
	
	// Find the route.
	void readStep();
	// Cache found route and move costs. Apply to waiting actors.
	void writeStep();
};
