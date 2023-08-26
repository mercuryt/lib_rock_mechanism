#pragma once
#include "threadedTask.h"
#include <vector>
#include <unordered_map>
class Block;
class Actor;
class PathToBlockBaseThreadedTask : public ThreadedTask
{
protected:
	std::vector<Block*> m_route;
	std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>> m_moveCostsToCache;
	void pathTo(const Actor& actor, const Block& block, bool detour = false);
	void cacheMoveCosts(const Actor& actor);
	PathToBlockBaseThreadedTask(ThreadedTaskEngine& tte) : ThreadedTask(tte) { }
	// For testing.
public:
	[[maybe_unused]]std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>>& getMoveCostsToCache() { return m_moveCostsToCache; }
	[[maybe_unused]]std::vector<Block*>& getPath() { return m_route; }
};
