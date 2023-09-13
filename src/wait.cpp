#include "wait.h"
#include "actor.h"
WaitObjective::WaitObjective(Actor& a, Step duration) : Objective(0), m_actor(a), m_event(a.getEventSchedule())
{
	m_event.schedule(duration, *this);
}
void WaitObjective::execute() { m_actor.m_hasObjectives.objectiveComplete(*this); }
WaitScheduledEvent::WaitScheduledEvent(Step delay, WaitObjective& wo) : ScheduledEventWithPercent(wo.m_actor.getSimulation(), delay), m_objective(wo) { }
