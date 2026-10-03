#include "actors.h"
#include "../body.h"
#include "../area/area.h"
bool Actors::body_isInjured(ActorIndex index) const
{
	return m_body[index]->isInjured();
}
bool Actors::body_isSeriouslyInjured(ActorIndex index) const
{
	return m_body[index]->isSeriouslyInjured();
}
BodyPart& Actors::body_pickABodyPartByVolume(ActorIndex index) const
{
	return m_body[index]->pickABodyPartByVolume(m_area.m_simulation);
}
BodyPart& Actors::body_pickABodyPartByType(ActorIndex index, const BodyPartTypeId bodyPartType) const
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
Percent Actors::body_getImpairMovePercent(ActorIndex index)
{
	return m_body[index]->getImpairMovePercent();
}
Percent Actors::body_getImpairManipulationPercent(ActorIndex index)
{
	return m_body[index]->getImpairManipulationPercent();
}
const std::vector<Wound*> Actors::body_getWounds(ActorIndex index) const
{
	return m_body[index]->getAllWounds();
}
const PsycologyWeight Actors::body_getPain(ActorIndex index) const
{
	return m_body[index]->getPain();
}