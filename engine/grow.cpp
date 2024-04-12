#include "grow.h"
#include "actor.h"
#include "config.h"
#include "simulation.h"
#include "util.h"
CanGrow::CanGrow(Actor& a, Percent pg) : m_actor(a), m_event(a.getEventSchedule()), m_percentGrown(pg)
{ 
	// Note: CanGrow must be initalized after MustEat, MustDrink, and SafeTemperature.
	updateGrowingStatus();
}
CanGrow::CanGrow(const Json& data, Actor& a) : m_actor(a), m_event(a.getEventSchedule()), m_percentGrown(data["percentGrown"].get<Percent>())
{
	if(data.contains("eventStart"))
		m_event.schedule(data["eventDuration"].get<Step>(), *this, data["eventStart"].get<Step>());
}
Json CanGrow::toJson() const
{
	Json data({{"percentGrown", growthPercent()}});
	if(m_event.exists())
	{
		data["eventStart"] = m_event.getStartStep();
		data["eventDuration"] = m_event.duration();
	}
	return data;
}
void CanGrow::setGrowthPercent(Percent percent)
{
	m_percentGrown = percent;
	// Restart the growth event.
	stop();
	m_event.reset();
	update();
	updateGrowingStatus();
}
void CanGrow::updateGrowingStatus()
{
	if(canGrowCurrently())
	{
		if(m_event.isPaused())
			scheduleEvent();
	}
	else if(!m_event.isPaused())
		m_event.pause();
}
bool CanGrow::canGrowCurrently() const
{
	return m_percentGrown != 100 && m_actor.getAge() < m_actor.m_species.stepsTillFullyGrown && 
		!m_actor.m_mustEat.needsFood() && !m_actor.m_mustDrink.needsFluid() && 
		m_actor.m_needsSafeTemperature.isSafeAtCurrentLocation();
}
Percent CanGrow::growthPercent() const
{
	return m_percentGrown;
}
void CanGrow::scheduleEvent()
{
	Step delay = std::round(m_actor.m_species.stepsTillFullyGrown / 100.f);
	m_event.resume(delay, *this);
}
void CanGrow::update()
{
	m_actor.m_attributes.updatePercentGrown(m_percentGrown);
	const Shape& shape = m_actor.m_species.shapeForPercentGrown(m_percentGrown);
	if(&shape != m_actor.m_shape)
		m_actor.setShape(shape);
	if(m_percentGrown != 100 && m_actor.m_species.stepsTillFullyGrown > m_actor.getAge() && m_event.isPaused())
		scheduleEvent();
}
void CanGrow::stop()
{
	if(!m_event.isPaused())
		m_event.pause();
}
void CanGrow::maybeStart()
{
	if(m_event.isPaused())
		updateGrowingStatus();
}
void CanGrow::increment()
{
	++m_percentGrown;
	m_event.reset();
	update();
}
AnimalGrowthEvent::AnimalGrowthEvent(Step delay, CanGrow& cg, const Step start) : ScheduledEvent(cg.m_actor.getSimulation(), delay, start), m_canGrow(cg) { }
