#include "actors.h"
#include "types.h"
#include "index.h"
#include "visionFacade.h"
void Actors::vision_do(const ActorIndex& index, ActorIndices& actors)
{
	//TODO: Since we moved to a homogenious threading model we don't need to double buffer here.
	m_canSee[index].swap(actors);
	//TODO: psycology.
	//TODO: fog of war?
}
void Actors::vision_createFacadeIfCanSee(const ActorIndex& index)
{
	if(vision_canSeeAnything(index))
		m_hasVisionFacade[index].create(m_area, index);
}
void Actors::vision_setRange(const ActorIndex& index, const DistanceInBlocks& range) 
{
       	m_visionRange[index] = range; 
	if(!m_hasVisionFacade.empty())
		m_hasVisionFacade[index].updateRange(range);
}
void Actors::vision_clearFacade(const ActorIndex& index)
{
	m_hasVisionFacade[index].clear();
}
void Actors::vision_swap(const ActorIndex& index, ActorIndices& toSwap)
{
	m_canSee[index].swap(toSwap);
}
bool Actors::vision_canSeeAnything(const ActorIndex& index) const
{
	return isAlive(index) && sleep_isAwake(index) && m_location[index].exists();
}
bool Actors::vision_canSeeActor(const ActorIndex& index, const ActorIndex& actor) const 
{ 
	return std::ranges::find(m_canSee[index], actor) != m_canSee[index].end(); 
}
VisionFacade& Actors::vision_getFacadeBucket(const ActorIndex& index)
{
	return m_hasVisionFacade[index].getVisionFacade();
}
std::pair<VisionFacade*, VisionFacadeIndex> Actors::vision_getFacadeWithIndex(const ActorIndex& index) const
{
	auto& hasVisionFacade = m_hasVisionFacade[index];
	return std::make_pair(&hasVisionFacade.getVisionFacade(), hasVisionFacade.getIndex());
}
bool Actors::vision_hasFacade(const ActorIndex& index) const
{
	return !m_hasVisionFacade[index].empty();
}
