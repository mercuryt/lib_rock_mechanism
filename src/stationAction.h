#pragma once

#include "queuedAction.h"

template<class Block, class Actor>
class StationAction : QueuedAction<Actor>
{
	Block& m_location;
	StationAction(Actor& a, Block& l) : QueuedAction(a), m_location(l) {}
	void execute()
	{
		if(m_actor.m_location != m_location)
			m_actor.setDestination(m_location);
	}
	// Is never complete, only overwitten / canceled.
	bool isComplete() const { return false; }
}
