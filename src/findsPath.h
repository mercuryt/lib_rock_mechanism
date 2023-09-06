#pragma once
#include "threadedTask.h"
#include <vector>
#include <unordered_map>
#include <functional>
class Block;
class Actor;
class FindsPath
{
	std::vector<Block*> m_route;
	std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>> m_moveCostsToCache;
public:
	void pathToBlock(const Actor& actor, const Block& block, bool detour = false);
	void pathToPredicate(const Actor& actor, std::function<bool(const Block&)> predicate, bool detour = false);
	void cacheMoveCosts(const Actor& actor);
	std::vector<Block*>& getPath() { return m_route; }
	bool found() const { return !m_route.empty(); }
	// For testing.
	[[maybe_unused]]std::unordered_map<Block*, std::vector<std::pair<Block*, uint32_t>>>& getMoveCostsToCache() { return m_moveCostsToCache; }
};
