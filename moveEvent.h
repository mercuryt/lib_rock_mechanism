#include "hasScheduledEvents.h"
class Actor;
/*
 *  Try to enter next step on route. 
 */
class MoveEvent : public ScheduledEvent
{
public:
	Actor* m_actor;
	MoveEvent(uint32_t s, Actor* a);
	void execute();
	~MoveEvent();
};
