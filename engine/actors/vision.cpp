#include "actors.h"
void Actors::vision_do(ActorIndex index, std::vector<ActorIndex>& actors)
{
	m_canSee.at(index).swap(actors);
	//TODO: psycology.
	//TODO: fog of war?
}
void Actors::vision_createFacadeIfCanSee(ActorIndex index)
{
	if(vision_canCurrentlySee(index))
		m_hasVisionFacade.at(index).create(index);
}
void Actors::vision_setRange(ActorIndex index, DistanceInBlocks range) 
{
       	m_visionRange.at(index) = range; 
	if(!m_hasVisionFacade.empty())
		m_hasVisionFacade.updateRange(range);
}
bool Actors::vision_canSeeAnything(ActorIndex index) const
{
	return m_actor.isAlive() && m_actor.m_mustSleep.isAwake() && m_actor.m_location;
}
bool Actors::vision_canSeeActor(ActorIndex index, ActorIndex actor) const 
{ 
	return std::ranges::find(m_canSee.at(index), actor) != m_canSee.at(index).end(); 
}
