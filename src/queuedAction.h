#pragma once

template<class Actor>
class QueuedAction
{
	Actor& m_actor;
	// called by Actor::taskComplete() until QueuedAction::isComplete(). Is expected to create a new callback to call taskComplete, unless it's the station action.
	virtual void execute();
	virtual bool isComplete() const { return true; }
}
