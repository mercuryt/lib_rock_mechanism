/*
 * Go To where combat uniform is and equip it. If no uniform exists do nothing.
 */
#pragma once
#include "queuedAction.h"

template<class Actor>
class MusterAction : QueuedAction<Actor>
{
	bool m_isComplete;
	MusterAction(Actor& a) : QueuedAction(a), m_isComplete(false) { }
	void execute()
	{
		if(m_actor.m_combatUniform != nullptr && !m_actor.m_combatUniform->isEquiped())
		{
			if(m_actor.m_location == m_actor.m_combatUniform.m_location)
			{
				m_actor.m_combatUniform.equip(m_actor);
				m_isComplete = true;
			}
			else
				m_actor.setDestination(m_actor.m_combatUniform.m_location);
		}
		else
			m_actor.m_area.m_notifyNoUniform(m_actor);
	}
	bool isComplete() const { return m_isComplete; }
}
