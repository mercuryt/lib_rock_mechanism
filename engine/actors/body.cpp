#include "actors.h"
#include "../body.h"
#include "../area.h"
bool Actors::body_isInjured(const ActorIndex& index) const
{
	return m_body[index]->isInjured();
}
BodyPart& Actors::body_pickABodyPartByVolume(const ActorIndex& index) const
{
	return m_body[index]->pickABodyPartByVolume(m_area.m_simulation);
}
BodyPart& Actors::body_pickABodyPartByType(const ActorIndex& index, const BodyPartTypeId& bodyPartType) const
{
	return m_body[index]->pickABodyPartByType(bodyPartType);
}
Step Actors::body_getStepsTillWoundsClose(const ActorIndex& index)
{
	return m_body[index]->getStepsTillWoundsClose();
}
Step Actors::body_getStepsTillBleedToDeath(const ActorIndex& index)
{
	return m_body[index]->getStepsTillBleedToDeath();
}
bool Actors::body_hasBleedEvent(const ActorIndex& index) const
{
	return m_body[index]->hasBleedEvent();
}
Percent Actors::body_getImpairMovePercent(const ActorIndex& index)
{
	return m_body[index]->getImpairMovePercent();
}
Percent Actors::body_getImpairManipulationPercent(const ActorIndex& index)
{
	return m_body[index]->getImpairManipulationPercent();
}
const std::vector<Wound*> Actors::body_getWounds(const ActorIndex& index) const
{
	return m_body[index]->getAllWounds();
}