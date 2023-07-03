#include "canGrow.h"
CanGrow::CanGrow(Actor& a, uint32_t pg) : m_actor(a), m_percentGrown(pg)
{ 
	// Note: CanGrow must be initalized after MustEat, MustDrink, and SafeTemperature.
	updateGrowingStatus();
}
void CanGrow::updateGrowingStatus()
{
	if(m_percentGrown != 100 && m_mustEat.ok() && m_mustDrink.ok() && m_safeTemperature.ok())
	{
		if(m_event.empty())
		{
			uint32_t step = ::g_step + (m_percentGrown == 0 ?
					m_actor.m_species.stepsTillFullyGrown :
					((m_actor.m_species.stepsTillFullyGrown * m_percentGrown) / 100u));
			m_event.schedule(step, *this);
			m_updateEvent.schedule(::g_step + Config::statsGrowthUpdateFrequency, *this);
		}
	}
	else if(!m_growthEvent.empty())
	{
		m_percentGrown = growthPercent();
		m_event->unschedule();
		m_updateEvent->unschedule();
	}
}
uint32_t CanGrow::growthPercent() const
{
	uint32_t output = m_percentGrown;
	if(!m_growthEvent.empty())
		output += (m_growthEvent->percentComplete() * m_percentGrown) / 100u;
	return output;
}
void CanGrow::update()
{
	uint32_t percent = growthPercent();
	m_actor.m_attributes.updatePercentGrown(percent);
	if(percent != 100)
		m_updateEvent.schedule(::g_step + Config::statsGrowthUpdateFrequency, *this);
}
