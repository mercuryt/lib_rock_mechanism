#include "actors.h"
#include "types.h"
#include "index.h"
#include "visionFacade.h"
void Actors::vision_do(ActorIndex index, ActorIndices& actors)
{
	//TODO: Since we moved to a homogenious threading model we don't need to double buffer here.
	m_canSee[index].swap(actors);
	//TODO: psycology.
	//TODO: fog of war?
}
void Actors::vision_createFacadeIfCanSee(ActorIndex index)
{
	if(vision_canSeeAnything(index))
		m_hasVisionFacade[index].create(m_area, index);
}
void Actors::vision_setRange(ActorIndex index, DistanceInBlocks range) 
{
       	m_visionRange[index] = range; 
	if(!m_hasVisionFacade.empty())
		m_hasVisionFacade[index].updateRange(range);
}
void Actors::vision_clearFacade(ActorIndex index)
{
	m_hasVisionFacade[index].clear();
}
void Actors::vision_swap(ActorIndex index, ActorIndices& toSwap)
{
	m_canSee[index].swap(toSwap);
}
bool Actors::vision_canSeeAnything(ActorIndex index) const
{
	return isAlive(index) && sleep_isAwake(index) && m_location[index].exists();
}
bool Actors::vision_canSeeActor(ActorIndex index, ActorIndex actor) const 
{ 
	return std::ranges::find(m_canSee[index], actor) != m_canSee[index].end(); 
}
VisionFacade& Actors::vision_getFacadeBucket(ActorIndex index)
{
	return m_hasVisionFacade[index].getVisionFacade();
}
std::pair<VisionFacade*, VisionFacadeIndex> Actors::vision_getFacadeWithIndex(ActorIndex index) const
{
	auto& hasVisionFacade = m_hasVisionFacade[index];
	return std::make_pair(&hasVisionFacade.getVisionFacade(), hasVisionFacade.getIndex());
}
bool Actors::vision_hasFacade(ActorIndex index) const
{
	return !m_hasVisionFacade[index].empty();
}
