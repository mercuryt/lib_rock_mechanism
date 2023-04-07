#include "hasScheduledEvents.h"
class Actor;
/*
 *  Try to enter next step on route. 
 */
class MoveEvent : public ScheduledEvent
{
public:
	Actor* m_actor;
	uint32_t m_delayCount;
	MoveEvent(uint32_t s, Actor* a, uint32_t dc = 0);
	void execute();
	~MoveEvent();
};
