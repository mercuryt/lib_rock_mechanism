#include "grow.h"
#include "actor.h"
#include "config.h"
CanGrow::CanGrow(Actor& a, uint32_t pg) : m_actor(a), m_percentGrown(pg)
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
			uint32_t delay = (m_percentGrown == 0 ?
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
uint32_t CanGrow::growthPercent() const
{
	uint32_t output = m_percentGrown;
	if(m_event.exists())
		output += (m_event.percentComplete() * m_percentGrown) / 100u;
	return output;
}
void CanGrow::update()
{
	uint32_t percent = growthPercent();
	m_actor.m_attributes.updatePercentGrown(percent);
	if(percent != 100)
		m_updateEvent.schedule(Config::statsGrowthUpdateFrequency, *this);
}
