/*
 * A scheduled event, which will move the actor to the next tile in it's route and then schedule the next move.
 * If the tile is permanantly no longer accessable then request a new route from area.
 * If the tile is temporarily no longer accessable then wait Config::moveTryAttemptsBeforeDetour move attempts and then request a new route from area with detour mode turned on.
 */
#pragma once
#include "eventSchedule.h"
class Actor;
/*
 *  Try to enter next step on route. 
 */
class MoveEvent : public ScheduledEvent
{
public:
	Actor& m_actor;
	uint32_t m_delayCount;
	static ScheduledEvent* emplace(EventSchedule& es, uint32_t delay, Actor& a);
	MoveEvent(uint32_t s, Actor& a);
	void execute();
	~MoveEvent();
};
