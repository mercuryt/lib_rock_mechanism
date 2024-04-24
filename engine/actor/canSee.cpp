#include "canSee.h"
Json ActorCanSee::toJson() const { return {{"range", m_range}}; }
void ActorCanSee::doVision(std::unordered_set<Actor*>& actors)
{
	m_currently.swap(actors);
	//TODO: psycology.
	//TODO: fog of war?
}
