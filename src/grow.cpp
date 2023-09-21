#include "grow.h"
#include "actor.h"
#include "config.h"
CanGrow::CanGrow(Actor& a, Percent pg) : m_actor(a), m_event(a.getEventSchedule()), m_updateEvent(a.getEventSchedule()), m_percentGrown(pg)
{ 
	// Note: CanGrow must be initalized after MustEat, MustDrink, and SafeTemperature.
	updateGrowingStatus();
}
void CanGrow::updateGrowingStatus()
{
	if(m_percentGrown != 100 && !m_actor.m_mustEat.needsFood() && !m_actor.m_mustDrink.needsFluid() && m_actor.m_needsSafeTemperature.isSafeAtCurrentLocation())
	{
		if(!m_event.exists())
		{
			Percent delay = (m_percentGrown == 0 ?
					m_actor.m_species.stepsTillFullyGrown :
					((m_actor.m_species.stepsTillFullyGrown * m_percentGrown) / 100u));
			m_event.schedule(delay, *this);
			m_updateEvent.schedule(Config::statsGrowthUpdateFrequency, *this);
		}
	}
	else if(m_event.exists())
	{
		m_percentGrown = growthPercent();
		m_event.unschedule();
		m_updateEvent.unschedule();
	}
}
Percent CanGrow::growthPercent() const
{
	Percent output = m_percentGrown;
	if(m_event.exists())
		output += (m_event.percentComplete() * m_percentGrown) / 100u;
	return output;
}
void CanGrow::update()
{
	Percent percent = growthPercent();
	m_actor.m_attributes.updatePercentGrown(percent);
	if(percent != 100)
		m_updateEvent.schedule(Config::statsGrowthUpdateFrequency, *this);
}
void CanGrow::complete()
{
	m_percentGrown = 100;
	m_updateEvent.maybeUnschedule();
	m_event.maybeUnschedule();
}
void CanGrow::stop()
{
	m_event.maybeUnschedule();
	m_updateEvent.maybeUnschedule();
}
void CanGrow::maybeStart()
{
	if(m_event.exists())
		updateGrowingStatus();
}
AnimalGrowthEvent::AnimalGrowthEvent(Step delay, CanGrow& cg) : ScheduledEventWithPercent(cg.m_actor.getSimulation(), delay), m_canGrow(cg) { }
AnimalGrowthUpdateEvent::AnimalGrowthUpdateEvent(Step delay, CanGrow& cg) : ScheduledEventWithPercent(cg.m_actor.getSimulation(), delay), m_canGrow(cg) { }
