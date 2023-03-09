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
	static std::vector<Block*> getVisibleBlocks(Block* block, uint32_t range);
	std::unordered_set<Actor*> getVisibleActors(uint32_t range, std::vector<Block*>* blocks);
	static bool hasLineOfSight(Block* from, Block* to, std::unordered_set<Block*> establishedAsHavingLineOfSight);
	static std::stack<Block*> getLineOfSight(Block* from, Block* to, std::unordered_set<Block*> establishedAsHavingLineOfSight);

	void recordActorsInBlock(Block* block);
};
