#include "grow.h"
#include "actor.h"
#include "config.h"
#include "simulation.h"
CanGrow::CanGrow(Actor& a, Percent pg) : m_actor(a), m_event(a.getEventSchedule()), m_updateEvent(a.getEventSchedule()), m_percentGrown(pg)
{ 
	// Note: CanGrow must be initalized after MustEat, MustDrink, and SafeTemperature.
	updateGrowingStatus();
}
CanGrow::CanGrow(const Json& data, Actor& a) : m_actor(a), m_event(a.getEventSchedule()), m_updateEvent(a.getEventSchedule()), m_percentGrown(data["percentGrown"].get<Percent>())
{
	if(data.contains("eventStart"))
		m_event.schedule(data["eventDuration"].get<Step>(), *this, data["eventStart"].get<Step>());
	if(data.contains("updateEventStart"))
		m_updateEvent.schedule(data["updateEventDuration"].get<Step>(), *this, data["updateEventStart"].get<Step>());
}
Json CanGrow::toJson() const
{
	Json data({{"percentGrown", growthPercent()}});
	if(m_event.exists())
	{
		data["eventStart"] = m_event.getStartStep();
		data["eventDuration"] = m_event.duration();
	}
	if(m_updateEvent.exists())
	{
		data["updateEventStart"] = m_updateEvent.getStartStep();
		data["updateEventDuration"] = m_updateEvent.duration();
	}
	return data;
}
void CanGrow::setGrowthPercent(Percent percent)
{
	m_event.maybeUnschedule();
	m_updateEvent.maybeUnschedule();
	m_percentGrown = percent;
	update();
}
void CanGrow::updateGrowingStatus()
{
	if(
			m_percentGrown != 100 && m_actor.getAge() < m_actor.m_species.stepsTillFullyGrown && 
			!m_actor.m_mustEat.needsFood() && !m_actor.m_mustDrink.needsFluid() && 
			m_actor.m_needsSafeTemperature.isSafeAtCurrentLocation()
	)
	{
		if(!m_event.exists())
		{
			Step delay = (m_percentGrown == 0 ?
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
	const Shape& shape = m_actor.m_species.shapeForPercentGrown(percent);
	if(&shape != m_actor.m_shape)
		m_actor.setShape(shape);
	if(percent != 100 && !m_updateEvent.exists() && m_actor.m_species.stepsTillFullyGrown > m_actor.getSimulation().m_step - m_actor.m_birthStep)
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
AnimalGrowthEvent::AnimalGrowthEvent(Step delay, CanGrow& cg, const Step start) : ScheduledEvent(cg.m_actor.getSimulation(), delay, start), m_canGrow(cg) { }
AnimalGrowthUpdateEvent::AnimalGrowthUpdateEvent(Step delay, CanGrow& cg, const Step start) : ScheduledEvent(cg.m_actor.getSimulation(), delay, start), m_canGrow(cg) { }
