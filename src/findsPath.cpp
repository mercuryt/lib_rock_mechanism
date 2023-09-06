#include "findsPath.h"
#include "path.h"
#include <utility>
void FindsPath::pathToBlock(const Actor& actor, const Block& block, bool detour)
{
	auto pair = path::getForActor(actor, block, detour);
	if(!pair.first.empty())
	{
		m_route = pair.first;
		m_moveCostsToCache = pair.second;
	}
}
void FindsPath::pathToPredicate(const Actor& actor, std::function<bool(const Block&)> predicate, bool detour)
{
	//TODO: detour, move costs cache.
	(void)detour;
	m_route = path::getForActorToPredicate(actor, predicate);
}
void FindsPath::cacheMoveCosts(const Actor& actor)
{
	for(auto& pair : m_moveCostsToCache)
		pair.first->m_hasShapes.tryToCacheMoveCosts(*actor.m_shape, actor.getMoveType(), pair.second);
}
