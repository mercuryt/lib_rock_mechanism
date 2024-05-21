#include "canSee.h"
#include "../actor.h"
Json ActorCanSee::toJson() const { return {{"range", m_range}}; }
void ActorCanSee::doVision(std::vector<Actor*>& actors)
{
	m_currently.swap(actors);
	//TODO: psycology.
	//TODO: fog of war?
}
void ActorCanSee::createFacadeIfCanSee()
{
	if(canCurrentlySee())
		m_hasVisionFacade.create(m_actor);
}
void ActorCanSee::setVisionRange(DistanceInBlocks range) 
{
       	m_range = range; 
	if(!m_hasVisionFacade.empty())
		m_hasVisionFacade.updateRange(range);
}
bool ActorCanSee::canCurrentlySee() const
{
	return m_actor.isAlive() && m_actor.m_mustSleep.isAwake() && m_actor.m_location;
}
bool ActorCanSee::isCurrentlyVisible(Actor& actor) const 
{ 
	return std::ranges::find(m_currently, &actor) != m_currently.end(); 
}
