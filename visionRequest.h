#pragma once 

#include <vector>
#include <stack>
#include <unordered_set>

class Block;
class Actor;
class VisionRequest
{
public:
	Actor& m_actor;
	std::unordered_set<Actor*> m_actors;
	std::vector<Block*> m_lineOfSight;
	std::unordered_set<Block*> m_establishedAsHavingLineOfSight;

	VisionRequest(Actor& a);

	// Find actors in range.
	void readStep();
	// Run Actor::doVision on actors.
	void writeStep();
	bool hasLineOfSightUsingEstablishedAs(Block* from, Block* to);
	static bool hasLineOfSight(Block* from, Block* to);
};
