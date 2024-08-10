#include "actors.h"
#include "../body.h"
#include "../area.h"
bool Actors::body_isInjured(ActorIndex index) const
{
	return m_body[index]->isInjured();
}
BodyPart& Actors::body_pickABodyPartByVolume(ActorIndex index) const
{
	return m_body[index]->pickABodyPartByVolume(m_area.m_simulation);
}
BodyPart& Actors::body_pickABodyPartByType(ActorIndex index, BodyPartTypeId bodyPartType) const
{
	return m_body[index]->pickABodyPartByType(bodyPartType);
}
Step Actors::body_getStepsTillWoundsClose(ActorIndex index)
{
	return m_body[index]->getStepsTillWoundsClose();
}
Step Actors::body_getStepsTillBleedToDeath(ActorIndex index)
{
	return m_body[index]->getStepsTillBleedToDeath();
}
bool Actors::body_hasBleedEvent(ActorIndex index) const
{
	return m_body[index]->hasBleedEvent();
}
