#include "actors.h"
#include "types.h"
#include "index.h"
void Actors::vision_do(ActorIndex index, ActorIndices& actors)
{
	//TODO: Since we moved to a homogenious threading model we don't need to double buffer here.
	m_canSee.at(index).swap(actors);
	//TODO: psycology.
	//TODO: fog of war?
}
void Actors::vision_createFacadeIfCanSee(ActorIndex index)
{
	if(vision_canSeeAnything(index))
		m_hasVisionFacade.at(index).create(m_area, index);
}
void Actors::vision_setRange(ActorIndex index, DistanceInBlocks range) 
{
       	m_visionRange.at(index) = range; 
	if(!m_hasVisionFacade.empty())
		m_hasVisionFacade.at(index).updateRange(range);
}
void Actors::vision_clearFacade(ActorIndex index)
{
	m_hasVisionFacade.at(index).clear();
}
void Actors::vision_updateFacadeIndex(ActorIndex index, VisionFacadeIndex visionFacadeIndex)
{
	m_hasVisionFacade.at(index).updateFacadeIndex(visionFacadeIndex);
}
bool Actors::vision_canSeeAnything(ActorIndex index) const
{
	return isAlive(index) && sleep_isAwake(index) && m_location.at(index).exists();
}
bool Actors::vision_canSeeActor(ActorIndex index, ActorIndex actor) const 
{ 
	return std::ranges::find(m_canSee.at(index), actor) != m_canSee.at(index).end(); 
}
VisionFacade& Actors::vision_getFacadeBucket(ActorIndex index)
{
	return m_hasVisionFacade.at(index).getVisionFacade();
}
std::pair<VisionFacade*, VisionFacadeIndex> Actors::vision_getFacadeWithIndex(ActorIndex index) const
{
	auto& hasVisionFacade = m_hasVisionFacade.at(index);
	return std::make_pair(&hasVisionFacade.getVisionFacade(), hasVisionFacade.getIndex());
}
bool Actors::vision_hasFacade(ActorIndex index) const
{
	return !m_hasVisionFacade.at(index).empty();
}
