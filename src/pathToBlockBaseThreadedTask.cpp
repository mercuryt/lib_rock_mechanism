#include "pathToBlockBaseThreadedTask.h"
#include "path.h"
#include <utility>
void PathToBlockBaseThreadedTask::pathTo(const Actor& actor, const Block& block, bool detour)
{
	auto pair = path::getForActor(actor, block, detour);
	if(!pair.first.empty())
	{
		m_route = pair.first;
		m_moveCostsToCache = pair.second;
	}
}
void PathToBlockBaseThreadedTask::cacheMoveCosts(const Actor& actor)
{
	for(auto& pair : m_moveCostsToCache)
		pair.first->m_hasShapes.tryToCacheMoveCosts(*actor.m_shape, actor.getMoveType(), pair.second);
}
