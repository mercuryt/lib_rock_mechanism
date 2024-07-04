#include "actors.h"
#include "../body.h"
bool Actors::body_isInjured(ActorIndex index) const
{
	return m_body.at(index)->isInjured();
}
