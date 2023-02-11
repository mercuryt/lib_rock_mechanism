#pragma once 

#include <vector>
#include <stack>
#include <unordered_set>

class Block;
class Actor;
class VisionRequest
{
public:
	Actor* m_actor;
	std::vector<Block*> m_blocks;
	std::unordered_set<Actor*> m_actors;

	VisionRequest(Actor* a);

	// Find blocks or actors in range.
	void readStep();
	// Cache blocks and Actor::doVision on actors.
	void writeStep();

	std::vector<Block*> getVisibleBlocks(uint32_t range) const;
	std::unordered_set<Actor*> getVisibleActors(uint32_t range, std::vector<Block*>* blocks);
	bool hasLineOfSight(Block* from, Block* to, std::unordered_set<Block*> establishedAsHavingLineOfSight) const;
	std::stack<Block*> getLineOfSight(Block* from, Block* to, std::unordered_set<Block*> establishedAsHavingLineOfSight) const;

	void recordActorsInBlock(Block* block);
};
